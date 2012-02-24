	.file	"syscall.c"
	.text
	.align	2
	.global	MyTid
	.type	MyTid, %function
MyTid:
	@ args = 0, pretend = 0, frame = 4
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	sub	sp, sp, #4
	@ lr needed for prologue
	swi #2
	add	sp, sp, #4
	bx	lr
	.size	MyTid, .-MyTid
	.align	2
	.global	Create
	.type	Create, %function
Create:
	@ args = 0, pretend = 0, frame = 4
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	sub	sp, sp, #4
	@ lr needed for prologue
	swi #1
	add	sp, sp, #4
	bx	lr
	.size	Create, .-Create
	.align	2
	.global	MyParentTid
	.type	MyParentTid, %function
MyParentTid:
	@ args = 0, pretend = 0, frame = 4
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	sub	sp, sp, #4
	@ lr needed for prologue
	swi #3
	add	sp, sp, #4
	bx	lr
	.size	MyParentTid, .-MyParentTid
	.align	2
	.global	Pass
	.type	Pass, %function
Pass:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	@ lr needed for prologue
	swi #4
	bx	lr
	.size	Pass, .-Pass
	.align	2
	.global	Exit
	.type	Exit, %function
Exit:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	@ lr needed for prologue
	swi #5
	bx	lr
	.size	Exit, .-Exit
	.align	2
	.global	Send
	.type	Send, %function
Send:
	@ args = 4, pretend = 0, frame = 4
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	sub	sp, sp, #4
	@ lr needed for prologue
	stmdb sp!, {r4}
	ldr r4, [sp, #8]
	swi #7
	ldmia sp!, {r4}
	add	sp, sp, #4
	bx	lr
	.size	Send, .-Send
	.align	2
	.global	Receive
	.type	Receive, %function
Receive:
	@ args = 0, pretend = 0, frame = 4
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	sub	sp, sp, #4
	@ lr needed for prologue
	swi #8
	add	sp, sp, #4
	bx	lr
	.size	Receive, .-Receive
	.align	2
	.global	Reply
	.type	Reply, %function
Reply:
	@ args = 0, pretend = 0, frame = 4
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	sub	sp, sp, #4
	@ lr needed for prologue
	swi #9
	add	sp, sp, #4
	bx	lr
	.size	Reply, .-Reply
	.align	2
	.global	AwaitEvent
	.type	AwaitEvent, %function
AwaitEvent:
	@ args = 0, pretend = 0, frame = 4
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	sub	sp, sp, #4
	@ lr needed for prologue
	swi #11
	add	sp, sp, #4
	bx	lr
	.size	AwaitEvent, .-AwaitEvent
	.align	2
	.global	Shutdown
	.type	Shutdown, %function
Shutdown:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	@ lr needed for prologue
	swi #13
	bx	lr
	.size	Shutdown, .-Shutdown
	.align	2
	.global	Assert
	.type	Assert, %function
Assert:
	@ args = 0, pretend = 0, frame = 4
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	sub	sp, sp, #4
	@ lr needed for prologue
	swi #12
	add	sp, sp, #4
	bx	lr
	.size	Assert, .-Assert
	.align	2
	.global	AssertF
	.type	AssertF, %function
AssertF:
	@ args = 4, pretend = 12, frame = 104
	@ frame_needed = 0, uses_anonymous_args = 1
	stmfd	sp!, {r1, r2, r3}
	stmfd	sp!, {r4, lr}
	cmp	r0, #0
	sub	sp, sp, #104
	add	ip, sp, #104
	mov	r4, sp
	mov	r1, sp
	add	r3, sp, #116
	mov	r2, #0
	bne	.L26
	str	r0, [ip, #-4]!
	ldr	r2, [sp, #112]
	mov	r0, ip
	bl	sformat(PLT)
	mov	r0, sp
	bl	Assert(PLT)
	mov	r2, r0
.L26:
	mov	r0, r2
	add	sp, sp, #104
	ldmfd	sp!, {r4, lr}
	add	sp, sp, #12
	bx	lr
	.size	AssertF, .-AssertF
	.ident	"GCC: (GNU) 4.0.2"
