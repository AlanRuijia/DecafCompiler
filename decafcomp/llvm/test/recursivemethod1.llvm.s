	.section	__TEXT,__text,regular,pure_instructions
	.macosx_version_min 10, 12
	.globl	_test
	.align	4, 0x90
_test:                                  ## @test
	.cfi_startproc
## BB#0:                                ## %entry
	movl	%edi, -4(%rsp)
	movl	$0, -8(%rsp)
	movl	$10, -8(%rsp)
	movl	-4(%rsp), %eax
	addl	$10, %eax
	retq
	.cfi_endproc

	.globl	_main
	.align	4, 0x90
_main:                                  ## @main
	.cfi_startproc
## BB#0:                                ## %entry
	pushq	%rax
Ltmp0:
	.cfi_def_cfa_offset 16
	movl	$0, 4(%rsp)
	movl	$1, 4(%rsp)
	movl	$1, %edi
	callq	_test
	movl	%eax, %edi
	callq	_print_int
	xorl	%eax, %eax
	popq	%rcx
	retq
	.cfi_endproc


.subsections_via_symbols
