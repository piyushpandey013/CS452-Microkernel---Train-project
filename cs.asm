.global __interrupt_type

.align	2
__interrupt_type: 	.word 0
link_register: 		.word 0
spsr_register: 		.word 0

@ supervisor 0xd3 = 0b11010011
@ supervisor/IE 0x53 = 0b01010011
@
@ system 0xdf = 0b11011111
@ system/IE 0x5f = 0b01011111
@
@ FIQ 0xd1 = 0b11010001
@ FIQ/IE 0x51 = 0b01010001
@
@ IRQ 0xd2 = 0b11010010
@ IRQ/IE 0x52 = 0b01010010

	.text
	.align	2
	.global	swi
	.type	swi, %function
swi:
	@ we are in supervisor mode with the user's registers!
	@ at this point, swi overwrote the lr_svc as the user programs's pc
	@ we need to go to the point whence the kernel left off

	@ also, due to the HWI, we need to set the type of interrupt
	@ into the memory location r8 holds. r9 will hold the value
	@ for use in the user_mode_resume, but we need it to be
	@ in a global memory location as well

	@ disable interrupts
	msr	CPSR_cx, #0xd3

	@ save r0 and r1 temporarily on the stack
	stmdb sp!, {r0, r1}
	
	@ put 0 into interrupt_type, meaning this is a system call
	mov r1, #0
	str r1, __interrupt_type
	
	@ save lr into link_register
	str lr, link_register
	
	@ save the spsr into spsr_register
	mrs r1, spsr
	str r1, spsr_register

	@ pop the r0 and r1 back
	ldmia sp!, {r0, r1}
	
	b	user_mode_resume
	.size	swi, .-swi

	.align 2
	.global hwi
	.type hwi, %function
hwi:	
	@ we are in interrupt mode and we need to tell the kernel this fact
	@ so we jump to FIQ mode, use the banked registers to write a
	@ value to a global at the address of r8
	
	@ disable interrupts
	msr	CPSR_cx, #0xd2

	@ save r0 and r1 temporarily on the stack
	stmdb sp!, {r0, r1}
	
	@ put 1 into interrupt_type, meaning this is a hardware interrupt
	mov r1, #1
	str r1, __interrupt_type
	
	@ save lr into link_register
	sub lr, lr, #4
	str lr, link_register
	
	@ save the spsr into spsr_register
	mrs r1, spsr
	str r1, spsr_register

	@ pop the r0 and r1 back
	ldmia sp!, {r0, r1}
	
	b	user_mode_resume
	.size	hwi, .-hwi

	.align	2
	.global	user_mode
	.type	user_mode, %function
user_mode:
	@ args = 0, pretend = 0, frame = 4
	@ frame_needed = 1, uses_anonymous_args = 0
	mov	ip, sp
	stmfd	sp!, {fp, ip, lr, pc}
	sub	fp, ip, #4
	sub	sp, sp, #4
	str	r0, [fp, #-16]
	@ Save kernel registers onto the kernel stack
	stmdb sp!, {r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, lr}
	@ Make sure tid is on the top of the stack so we can read it first
	stmdb sp!, {r0}
	@ Load the cpsr_usr from tid into r2
	@ Move the cpsr_usr (stored in r1) into the saved spsr (so that it can be swapped when we do `movs pc, lr`)
	ldr r2, [r0, #4]
	msr spsr_cxsf, r2
	@ lr right now is the next instruction of the user_mode() caller (in kernel mode), and we've saved it
	@ so we can override it with the user program's pc
	ldr lr, [r0, #12]

	@ Load the user's stack pointer into r2
	@ Restore user registers
	ldr r5, [r0, #8]

	ldmdb r5, {r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, sp, lr}^
	@ jump to user mode/program by moving user program pc (stored in kernel's lr) into pc,
	@ and copy the spsr into cpsr
	movs pc, lr
	
	@ --------------------------------------------
	@ Return point of kernel
user_mode_resume:

	@ Entering System Mode (to access user registers)
	msr CPSR_cx, #0xdf
	@ save umakser registers onto user stack
	stmdb sp, {r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, sp, lr}
	@ Move the user stack pointer into r1 so we can see it in SVC mode & save it in our task descriptor
	mov r1, sp
	@ Exiting system mode back to supervisor mode
	msr CPSR_cx, #0xd3
	
	@ Get task descriptor structure into r1 from the kernel stack
	ldmia sp!, {r0}
	
	@ Save the user's stack pointer into the task descriptor
	str r1, [r0, #8]
	
	@ get link_register (which is user's pc) into task structure
	ldr r1, link_register
	str r1, [r0, #12]
	
	@ get spsr_register into task structure
	ldr r1, spsr_register
	str r1, [r0, #4]

	@ restore kernel registers from the stack (except the lr_svc, which is userprog's pc)
	ldmia sp!, {r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, lr}

	ldmfd	sp, {r3, fp, sp, pc}
	.size	user_mode, .-user_mode
	.ident	"GCC: (GNU) 4.0.2"
