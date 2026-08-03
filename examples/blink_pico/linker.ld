ENTRY(Reset_Handler)

MEMORY
{
  BOOT2 (rx) : ORIGIN = 0x10000000, LENGTH = 0x100
  FLASH (rx) : ORIGIN = 0x10000100, LENGTH = 2048K - 0x100
  RAM (rwx)  : ORIGIN = 0x20000000, LENGTH = 256K
}

_estack = ORIGIN(RAM) + LENGTH(RAM);

SECTIONS
{
  .boot2 :
  {
    KEEP(*(.boot2))
  } > BOOT2

  .isr_vector :
  {
    KEEP(*(.isr_vector))
  } > FLASH

  .text :
  {
    *(.text*)
    *(.rodata*)
  } > FLASH

  .data :
  {
    _sdata = .;
    *(.data*)
    _edata = .;
  } > RAM AT> FLASH

  _sidata = LOADADDR(.data);

  .bss (NOLOAD) :
  {
    _sbss = .;
    *(.bss*)
    *(COMMON)
    _ebss = .;
  } > RAM
}
