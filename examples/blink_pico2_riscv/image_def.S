/* Minimum RISC-V IMAGE_DEF for RP2350 (datasheet §5.9.5). MIT — Klin machine_rp. */
.section .embedded_block, "a"
.p2align 2
embedded_block:
    .word   0xffffded3
    .byte   0x42
    .byte   0x01
    .hword  0x1101              /* EXE | CPU=RISC-V | CHIP=RP2350 */
    .byte   0x44
    .byte   0x03
    .hword  0x0000
    .word   _reset              /* entry */
    .word   0x20080000          /* initial SP — top of 512K SRAM */
    .byte   0xff
    .hword  (embedded_block_end - embedded_block - 16) / 4
    .byte   0x00
    .word   0
    .word   0xab123579
embedded_block_end:
