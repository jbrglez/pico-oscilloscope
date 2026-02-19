#ifndef PICO_OSCI_IOCTL_H
#define PICO_OSCI_IOCTL_H


struct buffer {
	uint8_t  *buf;
	uint16_t len;
};


// PICO_TEST_ADC = 4,
// PICO_NEW_SIGNAL = 5,
// PICO_GET_TEMPERATURE = 6,
// PICO_STOP_RUNNING_CAPTURE = 7,

#define PICO_IOCTL_SET_STATUS 		_IOW('a', 'a', uint8_t *)
#define PICO_IOCTL_GET_STATUS 		_IOR('a', 'b', uint8_t *)
#define PICO_IOCTL_START_RUNNING 	_IOW('a', 'c', uint16_t)
#define PICO_IOCTL_STOP_RUNNING 	_IOW('a', 'd', uint16_t)
#define PICO_IOCTL_SET_NEW_SIGNAL 	_IOW('a', 'e', struct buffer *)
/*#define WR_VALUE _IOW('a', 'a', int32_t *)*/
/*#define RD_VALUE _IOR('a', 'b', int32_t *)*/
#define GREETER  _IOW('a', 'c', struct test_struct *)

#endif 
