// #include <linux/kernel.h>
// #include <linux/errno.h>
// #include <linux/slab.h>
#include <linux/module.h>
#include <linux/init.h>
// #include <linux/kref.h>
// #include <linux/uaccess.h>
#include <linux/usb.h>
// #include <linux/mutex.h>


#include "pico_osci_ioctl.h"


#define USB_PICO_OSCI_VENDOR_ID		0x0000
#define USB_PICO_OSCI_PRODUCT_ID	0x0001


#define ISO_EP4_IN_ADDR		0x84
#define NUM_ISOC_PACKETS	16
#define ISO_TRANSFER_SIZE	0x3c0
#define SAMPLE_BUFFER_SIZE	0x80000



static const struct usb_device_id pico_osci_table[] = {
	{ USB_DEVICE(USB_PICO_OSCI_VENDOR_ID, USB_PICO_OSCI_PRODUCT_ID) },
	{ }
};
MODULE_DEVICE_TABLE(usb, pico_osci_table);


/* Get a minor range for your devices from the usb maintainer */
#define USB_PICO_OSCI_MINOR_BASE	192

#define WRITES_IN_FLIGHT	8

/* Structure to hold all of our device specific stuff */
struct usb_pico_osci {
	char 			endp0_in_buf[64];
	char 			endp0_out_buf[64];

	struct usb_device	*udev;			/* the usb device for this device */
	struct usb_interface	*interface;		/* the interface for this device */
	// struct semaphore	limit_sem;		/* limiting the number of writes in progress */
	struct usb_anchor	submitted;		/* in case we need to retract our submissions */

	struct urb		*iso_in_urb[2];		/* the urb to read data with */
	unsigned char           *iso_in_buffer[2];	/* the buffer to receive data */
	size_t			iso_in_size;		/* the size of the receive buffer */
	__u8			iso_in_endpointAddr;	/* the address of the bulk in endpoint */

	unsigned char           *sample_buffer;	/* the buffer to receive data */
	size_t			buf_size;
	int			buf_len;
	int           		buf_put_idx;	/* where to put data into sample_buffer */
	int           		buf_get_idx;	/* where to get data from sample_buffer */
	bool 			capturing;

	int			errors;			/* the last request tanked */
	bool			ongoing_read;		/* a read is going on */
	spinlock_t		err_lock;		/* lock for errors */
	struct kref		kref;
	struct mutex		io_mutex;		/* synchronize I/O with disconnect */
	unsigned long		disconnected:1;
	wait_queue_head_t	iso_in_wait;		/* to wait for an ongoing read */
};
#define to_pico_osci_dev(d) container_of(d, struct usb_pico_osci, kref)

typedef enum {
    PICO_GET_OSCI_STATUS = 1,
    PICO_SET_OSCI_STATUS = 2,
    PICO_START_RUNNING_CAPTURE = 3,
    PICO_TEST_ADC = 4,
    PICO_NEW_SIGNAL = 5,
    PICO_GET_TEMPERATURE = 6,
    PICO_STOP_RUNNING_CAPTURE = 7,
} pico_request;



static struct usb_driver pico_osci_driver;


static void pico_osci_delete(struct kref *kref)
{
	pr_info("pico_osci: freeing allocated memory CALLED pico_osci_delete.\n");

	struct usb_pico_osci *dev = to_pico_osci_dev(kref);

	usb_free_urb(dev->iso_in_urb[0]);
	usb_free_urb(dev->iso_in_urb[1]);
	usb_put_intf(dev->interface);
	usb_put_dev(dev->udev);
	kfree(dev->iso_in_buffer[0]);
	kfree(dev->iso_in_buffer[1]);
	kfree(dev->sample_buffer);

	kfree(dev);
}


static int pico_submit_urb(struct usb_pico_osci *dev, struct urb *urb)
{
	int retval;
	int retry_count = 0;

retry_submission:
	// retval = usb_submit_urb(urb, GFP_ATOMIC);
	retval = usb_submit_urb(urb, GFP_KERNEL);
	retry_count++;
	if ((retval == -EAGAIN) && (retry_count < 4))
		goto retry_submission;

	if (retval)
		dev_err(&dev->interface->dev, "Unable to submit URB. (errno: %d)\n", retval);

	return retval;
}


static int pico_osci_open(struct inode *inode, struct file *file)
{
	pr_info("pico CALL: pico_osci_open\n");
	struct usb_pico_osci *dev;
	struct usb_interface *interface;
	int subminor;
	int retval = 0;

	subminor = iminor(inode);

	interface = usb_find_interface(&pico_osci_driver, subminor);
	if (!interface) {
		pr_err("%s - error, can't find device for minor %d\n",
			__func__, subminor);
		retval = -ENODEV;
		goto exit;
	}

	dev = usb_get_intfdata(interface);
	if (!dev) {
		pr_err("pico_osci: Failed to get inteface\n");
		retval = -ENODEV;
		goto exit;
	}

	// retval = usb_autopm_get_interface(interface);
	// if (retval) {
	// 	pr_err("pico_osci: Failed to autopm get interface\n");
	// 	goto exit;
	// }

	/* increment our usage count for the device */
	kref_get(&dev->kref);

	/* save our object in the file's private structure */
	file->private_data = dev;

exit:
	pr_info("pico_osci: OPEN - retval = %d\n", retval);
	return retval;
}

static int pico_osci_release(struct inode *inode, struct file *file)
{
	pr_info("pico CALL: pico_osci_release\n");
	struct usb_pico_osci *dev;

	dev = file->private_data;
	if (dev == NULL)
		return -ENODEV;

	/* allow the device to be autosuspended */
	// usb_autopm_put_interface(dev->interface);

	/* decrement the count on our device */
	kref_put(&dev->kref, pico_osci_delete);
	pr_info("pico_osci: RELEASE\n");
	return 0;
}


static long int pico_osci_ioctl (struct file *file, unsigned int cmd, unsigned long arg)
{
	// pr_info("pico_osci: IOCTL - cmd = %u\n", cmd);
	pr_info("pico CALL: pico_osci_ioctl\n");

	static uint8_t status = 0x12;
	uint16_t chans = (uint16_t)arg;

	struct usb_pico_osci *dev;
	int retval = 0;

	// uint8_t *waveform = (uint8_t *)arg;
	struct buffer waveform = {0};;

	dev = file->private_data;

	switch (cmd) {
		case PICO_IOCTL_START_RUNNING:
			pr_debug("pico_osci: IOCTL - START_RUNNING ( chans = %d )\n", chans);
			if (dev->capturing != 1) {
				dev->capturing = 1;
				retval = pico_submit_urb(dev, dev->iso_in_urb[0]);
				if (retval) {
					pr_err("pico_osci: IOCTL - Error setting pico capture chans. Failed to submit URB_0. (errno %d)\n", retval);
					return -1;
				}
				retval = pico_submit_urb(dev, dev->iso_in_urb[1]);
				if (retval) {
					pr_err("pico_osci: IOCTL - Error setting pico capture chans. Failed to submit URB_1. (errno %d)\n", retval);
					return -1;
				}
			}

			// retval = usb_control_msg(dev->udev, usb_sndctrlpipe(dev->udev, 0), PICO_START_RUNNING_CAPTURE, 0x40, chans, 0, NULL, 0, 100);
			retval = usb_control_msg_send(dev->udev, 0, PICO_START_RUNNING_CAPTURE, 0x40, chans, 0, NULL, 0, 100, GFP_KERNEL);
			if (retval < 0) {
				pr_err("pico_osci: IOCTL - Error setting pico capture chans. (errno %d)\n", retval);
				return -1;
			}

			dev->buf_put_idx = 0;
			dev->buf_get_idx = 0;

			break;

		case PICO_IOCTL_STOP_RUNNING:
			pr_debug("pico_osci: IOCTL - STOP_RUNNING\n");
			retval = usb_control_msg_send(dev->udev, 0, PICO_STOP_RUNNING_CAPTURE, 0x40, 0, 0, NULL, 0, 100, GFP_KERNEL);
			if (retval < 0) {
				pr_err("pico_osci: IOCTL - Error stopping pico capture. (errno %d)\n", retval);
				return -1;
			}

			dev->capturing = 0;
			usb_kill_urb(dev->iso_in_urb[0]);
			usb_kill_urb(dev->iso_in_urb[1]);
			// usb_kill_anchored_urbs(&dev->submitted);
			pr_info("pico_osci: IOCTL - Stopped capturing\n");
			break;

		case PICO_IOCTL_SET_NEW_SIGNAL:
			pr_info("pico_osci: IOCTL - NEW_SIGNAL\n");
			if (copy_from_user(&waveform, (struct buffer *)arg, sizeof(waveform))) {
				pr_err("pico_osci: IOCTL - Error copying data from user.\n");
				return -EFAULT;
			}

			uint8_t *data = kmalloc(waveform.len, GFP_KERNEL);
			if (!data) {
				pr_err("pico_osci: IOCTL - Error allocating data buffer.\n");
				return -ENOMEM;
			}

			if (copy_from_user(data, waveform.buf, waveform.len)) {
				pr_err("pico_osci: IOCTL - Error copying waveform data from user.\n");
				return -EFAULT;
			}

			int num_pkts = ((waveform.len + 0x3F) / 0x40);
			int remaining = waveform.len;
			pr_info("pico_osci: IOCTL :: NEW_SIGNAL : num_pkts = %d, len = %d\n", num_pkts, (int)waveform.len);

			for (int i = 0; i < num_pkts; i++) {
				int len = min(remaining, 0x40);
				remaining -= len;

				uint16_t offset = 0x40 * i;
				uint16_t wValue = 0;
				// wValue |= ((i == (num_pkts-1)) && switch_buffers) ? (1) : (0);
				wValue |= (i == (num_pkts-1)) ? 1 : 0;
				wValue |= (i == 0) ? 2 : 0;
				uint16_t wIndex = (i == 0) ? waveform.len : offset;

				retval = usb_control_msg_send(dev->udev, 0, PICO_NEW_SIGNAL, 0x40, wValue, wIndex, &data[offset], len, 100, GFP_KERNEL);
				if (retval < 0) {
					pr_err("pico_osci: IOCTL - Error sending new waveform. (errno %d)\n", retval);
					return -1;
				}
			}

			pr_info("pico_osci: IOCTL - Sent new waveform.\n");

			kfree(data);

			break;

		case PICO_IOCTL_SET_STATUS:
			pr_debug("pico_osci: IOCTL - SET_STATUS\n");
			if (copy_from_user(&status, (uint16_t *)arg, sizeof(status))) {
				pr_err("pico_osci: IOCTL - Error copying data from user.\n");
				return -EFAULT;
			}

			// retval = usb_control_msg(dev->udev, usb_sndctrlpipe(dev->udev, 0), PICO_SET_OSCI_STATUS, 0x40, status, 0, NULL, 0, 100);
			retval = usb_control_msg_send(dev->udev, 0, PICO_SET_OSCI_STATUS, 0x40, status, 0, NULL, 0, 100, GFP_KERNEL);
			if (retval < 0) {
				pr_err("pico_osci: IOCTL - Error setting pico status. (errno %d)\n", retval);
				return -1;
			}

			pr_info("pico_osci: IOCTL - Update the status to %d\n", status);
			break;

		case PICO_IOCTL_GET_STATUS:
			pr_debug("pico_osci: IOCTL - GET_STATUS\n");
			retval = usb_control_msg_recv(dev->udev, 0, PICO_GET_OSCI_STATUS, 0xC0, 0, 0, &status, 1, 100, GFP_KERNEL);
			if (retval < 0) {
				pr_err("pico_osci: IOCTL - Error reading pico status. (errno %d)\n", retval);
				return -1;
			}

			if (copy_to_user((uint8_t *)arg, &status, sizeof(status))) {
				pr_err("pico_osci: IOCTL - Error copying data to user. (errno %d)\n", retval);
				return -EFAULT;
			}

			pr_info("pico_osci: IOCTL - The status was copyied\n");
			break;

		default:
			// return -ENOTTY;
			return -ENOIOCTLCMD;
			// return -EOPNOTSUPP;
			break;
	}
	return 0;
}


static void pico_osci_read_iso_callback(struct urb *urb)
{
	struct usb_pico_osci *dev;
	// unsigned long flags;
	int retval;

	dev = urb->context;

	// spin_lock_irqsave(&dev->err_lock, flags);
	// sync/async unlink faults aren't errors
	if (urb->status && !((urb->status == -ENOENT) || (urb->status == -ECONNRESET))) {
		dev_err(&dev->interface->dev,
			"%s - nonzero write iso status received: %d\n",
			__func__, urb->status);
		dev->errors = urb->status;
	} else {
		for (int i = 0; i < NUM_ISOC_PACKETS; i++) {
			if (urb->iso_frame_desc[i].actual_length != 0) {
				int new_buf_put_idx = dev->buf_put_idx + urb->iso_frame_desc[i].actual_length;

				// int start = dev->buf_put_idx;

				unsigned char *dest = &dev->sample_buffer[dev->buf_put_idx];
				unsigned char *src  = &((unsigned char *)urb->transfer_buffer)[urb->iso_frame_desc[i].offset];

				if (new_buf_put_idx <= dev->buf_len) {
					memcpy(dest, src, urb->iso_frame_desc[i].actual_length);
				} else {
					new_buf_put_idx -= dev->buf_len;

					size_t len_first  = dev->buf_len - dev->buf_put_idx;
					size_t len_second = urb->iso_frame_desc[i].actual_length - len_first;

					memcpy(src, dest, len_first);
					dest = dev->sample_buffer;
					src  = &((unsigned char *)urb->transfer_buffer)[urb->iso_frame_desc[i].offset + len_first];
					memcpy(src, urb->transfer_buffer + urb->iso_frame_desc[i].offset + len_first, len_second);
				}
				dev->buf_put_idx = new_buf_put_idx;

				int idxp = dev->buf_put_idx;
				int idxg = dev->buf_get_idx;
				int len  = dev->buf_len;
				// If the callback is called durning the execution of pico_osci_read() 
				// there should be enough space after the buf_put_idx, so that the data
				// does not change while it's being read.
				int max_available = len - 2*NUM_ISOC_PACKETS*1024;

				size_t available = (idxp > idxg) ? (idxp - idxg) : (len + idxp - idxg);
				// We push forward the buf_get_idx if there is too much unread data in the buffer.
				if (available > max_available) {
					int new_idxg = idxg + available - max_available;
					if ( new_idxg > len)
						new_idxg -=len;
					dev->buf_get_idx = new_idxg;
				}
			}
		}
	}
	// dev->ongoing_read = 0;
	// spin_unlock_irqrestore(&dev->err_lock, flags);

	// wake_up_interruptible(&dev->iso_in_wait);

	if ((urb->status == 0) && dev->capturing) {
		retval = usb_submit_urb(urb, GFP_ATOMIC);
		if (retval < 0) {
			dev_err(&dev->interface->dev,
				"%s - failed submitting read urb, error %d\n",
				__func__, retval);
		}
	}
}


static ssize_t pico_osci_read(struct file *file, char *buffer, size_t count,
			 loff_t *ppos)
{
	struct usb_pico_osci *dev;
	int rv;

	dev = file->private_data;

	if (!count)
		return 0;

	/* no concurrent readers */
	rv = mutex_lock_interruptible(&dev->io_mutex);
	if (rv < 0)
		return rv;

	if (dev->disconnected) {
		rv = -ENODEV;
		goto exit;
	}

	/* errors must be reported */
	rv = dev->errors;
	if (rv < 0) {
		/* any error is reported once */
		dev_err(&dev->interface->dev, "error reading dev->error = %d\n", rv);
		dev->errors = 0;
		/* to preserve notifications about reset */
		rv = (rv == -EPIPE) ? rv : -EIO;
		/* report it */
		goto exit;
	}

	if (1) {
		int idxp = dev->buf_put_idx;
		int idxg = dev->buf_get_idx;
		int len  = dev->buf_len;

		size_t available = (idxp >= idxg) ? (idxp - idxg) : (len + idxp - idxg);
		available = min(available, (size_t)len - 2*NUM_ISOC_PACKETS*1024);
		size_t chunk = min(available, count);

		if (idxg + chunk <= len) {
			if (copy_to_user(buffer, &dev->sample_buffer[idxg], chunk)) {
				rv = -EFAULT;
				pr_err("ERR_pico_osci WHOLE: copy_to_user (%p, %x, %x)\n", buffer, idxg, (int)chunk);
			} else {
				rv = chunk;
				dev->buf_get_idx += chunk;
			}
		} else {
			size_t first_chunk = dev->buf_len - idxg;
			size_t second_chunk = chunk - first_chunk;

			if (copy_to_user(buffer, &dev->sample_buffer[idxg], first_chunk)) {
				rv = -EFAULT;
				pr_err("ERR_pico_osci FIRST: copy_to_user (%p, %x, %x)\n", buffer, idxg, (int)first_chunk);
				pr_err("ERR_pico_osci FIRST: idxp = %x, idxg = %x, len = %x\n", idxp, idxg, len);
			} else {
				// rv = first_chunk;
				if (copy_to_user(&buffer[first_chunk], &dev->sample_buffer[0], second_chunk)) {
					rv = -EFAULT;
					pr_err("ERR_pico_osci SECOND: copy_to_user (%p, %x, %x)\n", buffer, idxg + (int)first_chunk, (int)second_chunk);
				} else {
					rv = chunk;
					dev->buf_get_idx = second_chunk;
				}
			}
		}
	}
exit:
	mutex_unlock(&dev->io_mutex);
	return rv;
}


static const struct file_operations pico_osci_fops = {
	.owner =	THIS_MODULE,
	.read =		pico_osci_read,
	.unlocked_ioctl = pico_osci_ioctl,
	.open =		pico_osci_open,
	.release =	pico_osci_release,
	.llseek =	noop_llseek,
};


/*
 * usb class driver info in order to get a minor number from the usb core,
 * and to have the device registered with the driver core
 */
static struct usb_class_driver pico_osci_class = {
	.name =		"pico_osci%d",
	.fops =		&pico_osci_fops,
	.minor_base =	USB_PICO_OSCI_MINOR_BASE,
};


static int pico_osci_probe(struct usb_interface *interface,
		      const struct usb_device_id *id)
{
	pr_info("pico_osci: Probing for Pico-Oscilloscope.\n");

	struct usb_pico_osci *dev;
	// struct usb_endpoint_descriptor *bulk_in, *bulk_out;
	/*struct usb_endpoint_descriptor *iso_in;*/
	int retval;

	/* allocate memory for our device state and initialize it */
	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	kref_init(&dev->kref);
	// sema_init(&dev->limit_sem, WRITES_IN_FLIGHT);
	mutex_init(&dev->io_mutex);
	spin_lock_init(&dev->err_lock);
	// init_usb_anchor(&dev->submitted);
	init_waitqueue_head(&dev->iso_in_wait);

	dev->udev = usb_get_dev(interface_to_usbdev(interface));
	dev->interface = usb_get_intf(interface);

	// /* set up the endpoint information */
	// /* use only the first bulk-in and bulk-out endpoints */
	// retval = usb_find_common_endpoints(interface->cur_altsetting,
	// 		&bulk_in, &bulk_out, NULL, NULL);
	// if (retval) {
	// 	dev_err(&interface->dev,
	// 		"Could not find both bulk-in and bulk-out endpoints\n");
	// 	goto error;
	// }

	dev->iso_in_size = NUM_ISOC_PACKETS * ISO_TRANSFER_SIZE;
	dev->buf_size = SAMPLE_BUFFER_SIZE;

	// dev->iso_in_endpointAddr = bulk_in->bEndpointAddress;
	dev->iso_in_endpointAddr = ISO_EP4_IN_ADDR;


	dev->iso_in_buffer[0] = kmalloc(dev->iso_in_size, GFP_KERNEL);
	if (!dev->iso_in_buffer[0]) {
		retval = -ENOMEM;
		goto error;
	}
	dev->iso_in_buffer[1] = kmalloc(dev->iso_in_size, GFP_KERNEL);
	if (!dev->iso_in_buffer[1]) {
		retval = -ENOMEM;
		goto error;
	}

	dev->sample_buffer = kmalloc(SAMPLE_BUFFER_SIZE, GFP_KERNEL);
	if (!dev->sample_buffer) {
		retval = -ENOMEM;
		goto error;
	}
	dev->buf_len = SAMPLE_BUFFER_SIZE;
	dev->buf_put_idx = 0;
	dev->buf_get_idx = 0;

	dev->iso_in_urb[0] = usb_alloc_urb(NUM_ISOC_PACKETS, GFP_KERNEL);
	if (!dev->iso_in_urb[0]) {
		retval = -ENOMEM;
		goto error;
	}

	dev->iso_in_urb[1] = usb_alloc_urb(NUM_ISOC_PACKETS, GFP_KERNEL);
	if (!dev->iso_in_urb[1]) {
		retval = -ENOMEM;
		goto error;
	}

	/* save our data pointer in this interface device */
	usb_set_intfdata(interface, dev);

	/* we can register the device now, as it is ready */
	retval = usb_register_dev(interface, &pico_osci_class);
	if (retval) {
		dev_err(&interface->dev,
			"Not able to get a minor for this device.\n");
		usb_set_intfdata(interface, NULL);
		goto error;
	}

	/* let the user know what node this device is now attached to */
	dev_info(&interface->dev,
		 "USB Pico_osci device now attached to USBPico_osci-%d",
		 interface->minor);

	usb_fill_int_urb(dev->iso_in_urb[0],
			dev->udev,
			usb_rcvisocpipe(dev->udev, dev->iso_in_endpointAddr),
			dev->iso_in_buffer[0],
			dev->iso_in_size,
			pico_osci_read_iso_callback,
			dev,
			1);
	usb_fill_int_urb(dev->iso_in_urb[1],
			dev->udev,
			usb_rcvisocpipe(dev->udev, dev->iso_in_endpointAddr),
			dev->iso_in_buffer[1],
			dev->iso_in_size,
			pico_osci_read_iso_callback,
			dev,
			1);

	dev->iso_in_urb[0]->number_of_packets = NUM_ISOC_PACKETS;
	dev->iso_in_urb[1]->number_of_packets = NUM_ISOC_PACKETS;
	for (int i = 0; i < NUM_ISOC_PACKETS; i++) {
		dev->iso_in_urb[0]->iso_frame_desc[i].length = ISO_TRANSFER_SIZE;
		dev->iso_in_urb[0]->iso_frame_desc[i].offset = i * ISO_TRANSFER_SIZE;
		dev->iso_in_urb[1]->iso_frame_desc[i].length = ISO_TRANSFER_SIZE;
		dev->iso_in_urb[1]->iso_frame_desc[i].offset = i * ISO_TRANSFER_SIZE;
	}
	dev->iso_in_urb[0]->transfer_flags = URB_ISO_ASAP;
	dev->iso_in_urb[1]->transfer_flags = URB_ISO_ASAP;
	// usb_anchor_urb(dev->iso_in_urb[0], &dev->submitted);
	// usb_anchor_urb(dev->iso_in_urb[1], &dev->submitted);

	return 0;

error:
	/* this frees allocated memory */
	kref_put(&dev->kref, pico_osci_delete);

	pr_info("pico_osci: FAILED to init module.\n");

	return retval;
}


static void pico_osci_disconnect(struct usb_interface *interface)
{
	pr_info("pico_osci: Disconnected Pico-Oscilloscope.\n");

	struct usb_pico_osci *dev;
	int minor = interface->minor;

	dev = usb_get_intfdata(interface);

	/* give back our minor */
	usb_deregister_dev(interface, &pico_osci_class);

	/* prevent more I/O from starting */
	mutex_lock(&dev->io_mutex);
	dev->disconnected = 1;
	mutex_unlock(&dev->io_mutex);

	usb_kill_urb(dev->iso_in_urb[0]);
	usb_kill_urb(dev->iso_in_urb[1]);
	// usb_kill_anchored_urbs(&dev->submitted);

	/* decrement our usage count */
	kref_put(&dev->kref, pico_osci_delete);

	dev_info(&interface->dev, "USB Pico_osci #%d now disconnected", minor);
}



static struct usb_driver pico_osci_driver = {
	.name =		"pico_osci",
	.probe =	pico_osci_probe,
	.disconnect =	pico_osci_disconnect,
	// .suspend =	pico_osci_suspend,
	// .resume =	pico_osci_resume,
	// .pre_reset =	pico_osci_pre_reset,
	// .post_reset =	pico_osci_post_reset,
	.id_table =	pico_osci_table,
	// .supports_autosuspend = 1,
};

#if 0

module_usb_driver(pico_osci_driver);

#else

static int __init pico_osci_init(void)
{
	int result;
	pr_info("pico_osci: Module init\n");
	result = usb_register(&pico_osci_driver);
	if (result) {
		pr_err("pico_osci: Failed to register pico_osci driver.\n");
	}
	return result;
}

static void __exit pico_osci_exit(void)
{
	pr_info("pico_osci: Module exit\n");
	usb_deregister(&pico_osci_driver);
}

#endif

module_init(pico_osci_init);
module_exit(pico_osci_exit);

MODULE_LICENSE("GPL v2");
