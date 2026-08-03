/* Minimum Arm IMAGE_DEF for RP2350 (datasheet §5.9.5). MIT — Klin machine_rp. */
.section .picobin_block, "a"

.word 0xffffded3 /* PICOBIN_BLOCK_MARKER_START */

/* IMAGE_TYPE: EXE + Secure + ARM + RP2350 */
.byte 0x42
.byte 0x1
.hword 0x1021

/* LAST */
.byte 0xff
.hword 0x0001
.byte 0

.word 0 /* next block = self / none */

.word 0xab123579 /* PICOBIN_BLOCK_MARKER_END */
