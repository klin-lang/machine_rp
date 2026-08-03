OUTPUT_FORMAT("elf32-littleriscv")
ENTRY(_reset)

MEMORY
{
  FLASH (rx)  : ORIGIN = 0x10000000, LENGTH = 4M
  RAM (rwx)   : ORIGIN = 0x20000000, LENGTH = 512K
}

SECTIONS
{
  .text :
  {
    KEEP(*(.embedded_block))
    *(.text .text.*)
    *(.rodata .rodata.*)
  } > FLASH

  .data :
  {
    _sdata = .;
    *(.data .data.*)
    _edata = .;
  } > RAM AT> FLASH

  _sidata = LOADADDR(.data);

  .bss (NOLOAD) :
  {
    _sbss = .;
    *(.bss .bss.*)
    *(COMMON)
    _ebss = .;
  } > RAM
}
