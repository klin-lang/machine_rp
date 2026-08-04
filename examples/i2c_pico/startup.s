.syntax unified
.cpu cortex-m0plus
.thumb

.global Reset_Handler
.extern main

.section .isr_vector, "a", %progbits
.word _estack
.word Reset_Handler
.word NMI_Handler
.word HardFault_Handler
.word 0
.word 0
.word 0
.word 0
.word 0
.word 0
.word 0
.word SVC_Handler
.word 0
.word 0
.word PendSV_Handler
.word SysTick_Handler

.section .text.Reset_Handler, "ax", %progbits
.thumb_func
Reset_Handler:
  ldr r0, =_sdata
  ldr r1, =_edata
  ldr r2, =_sidata
1:
  cmp r0, r1
  bcs 2f
  ldr r3, [r2]
  str r3, [r0]
  adds r2, #4
  adds r0, #4
  b 1b
2:
  ldr r0, =_sbss
  ldr r1, =_ebss
  movs r2, #0
3:
  cmp r0, r1
  bcs 4f
  str r2, [r0]
  adds r0, #4
  b 3b
4:
  bl main
5:
  b 5b

.section .text.Default_Handler, "ax", %progbits
.thumb_func
Default_Handler:
  b .

.weak NMI_Handler
.thumb_set NMI_Handler, Default_Handler
.weak HardFault_Handler
.thumb_set HardFault_Handler, Default_Handler
.weak SVC_Handler
.thumb_set SVC_Handler, Default_Handler
.weak PendSV_Handler
.thumb_set PendSV_Handler, Default_Handler
.weak SysTick_Handler
.thumb_set SysTick_Handler, Default_Handler
