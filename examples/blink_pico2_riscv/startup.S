/* Minimal RV32 crt0 for RP2350: .data copy, .bss zero, call main. */
.section .text
.globl _reset
.extern main
.extern _sidata
.extern _sdata
.extern _edata
.extern _sbss
.extern _ebss

_reset:
    /* copy .data */
    la   a0, _sdata
    la   a1, _edata
    la   a2, _sidata
1:
    bgeu a0, a1, 2f
    lw   t0, 0(a2)
    sw   t0, 0(a0)
    addi a2, a2, 4
    addi a0, a0, 4
    j    1b
2:
    /* zero .bss */
    la   a0, _sbss
    la   a1, _ebss
3:
    bgeu a0, a1, 4f
    sw   zero, 0(a0)
    addi a0, a0, 4
    j    3b
4:
    call main
5:
    j    5b
