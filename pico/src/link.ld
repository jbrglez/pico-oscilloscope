MEMORY
{
	BOOT_FLASH(r) : ORIGIN = 0x10000000, LENGTH = 256
	BOOT_SRAM(rwx) : ORIGIN = 0x20041f00, LENGTH = 256

	FLASH(r) : ORIGIN = 0x10000100, LENGTH = 2M - 256
	SRAM(rwx) : ORIGIN = 0x20000000, LENGTH = 256K
	SCRATCH_X(rwx) : ORIGIN = 0x20040000, LENGTH = 4K
	SCRATCH_Y(rwx) : ORIGIN = 0x20041000, LENGTH = 4K

	XIP_CACHE(rwx) : ORIGIN = 0x15000000, LENGTH = 16K
}

ENTRY(_entry_point)

SECTIONS
{

	. = ORIGIN(BOOT_SRAM);
	.boot2 ALIGN(0x100): {
		__pico_boot2_start__ = .;
		KEEP (*(.boot2))
		__pico_boot2_code_end__ = .;
		. = ALIGN(0x100);
		__pico_boot2_end__ = .;
	} > BOOT_SRAM AT> BOOT_FLASH

	ASSERT(__pico_boot2_code_end__ - __pico_boot2_start__ <= 252, "ERROR: boot2 code larger than 252B")
	ASSERT(__pico_boot2_end__ - __pico_boot2_start__ == 256, "ERROR: boot2 larger than 256B")

	. = ORIGIN(SRAM);

	.NVIC_reset ALIGN(0x100): {
		__pico_nvic_start__ = .;
		KEEP (*(.vectors))
		KEEP (*(.reset))
		. = ALIGN(16);
		__pico_nvic_end__ = .;
	} > SRAM AT> FLASH
	__pico_nvic_source__ = LOADADDR(.NVIC_reset);

	.text ALIGN(16): {
		__pico_text_start__ = .;
		*(.text*)
		. = ALIGN(16);
		__pico_text_end__ = .;
	} > SRAM AT> FLASH
	__pico_text_source__ = LOADADDR(.text);

	.rodata ALIGN(16): {
		__pico_rodata_start__ = .;
		*(.rodata)
		*(.rodata.*)
		. = ALIGN(16);
		__pico_rodata_end__ = .;
	} > SRAM AT> FLASH
	__pico_rodata_source__ = LOADADDR(.rodata);

	.data ALIGN(16): {
		__pico_data_start__ = .;
		*(.data*)
		. = ALIGN(16);
		__pico_data_end__ = .;
	} > SRAM AT> FLASH
	__pico_data_source__ = LOADADDR(.data);


	.bss ALIGN(16): {
		*(.bss*)
		. = ALIGN(16);
	} > SRAM
	__pico_bss_start__ = LOADADDR(.bss);
	__pico_bss_end__   = LOADADDR(.bss) + SIZEOF(.bss);

	__pico_dbg_str_section_start__ = .;
}
