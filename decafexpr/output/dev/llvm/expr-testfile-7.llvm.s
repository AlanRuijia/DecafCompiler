	.section	__TEXT,__text,regular,pure_instructions
	.macosx_version_min 10, 12
	.globl	_main
	.align	4, 0x90
_main:                                  ## @main
	.cfi_startproc
## BB#0:                                ## %entry
	subq	$24, %rsp
Ltmp0:
	.cfi_def_cfa_offset 32
	movl	$958, 20(%rsp)          ## imm = 0x3BE
	movl	$-28660, 12(%rsp)       ## imm = 0xFFFFFFFFFFFF900C
	movl	$28660, 20(%rsp)        ## imm = 0x6FF4
	movb	$0, 19(%rsp)
	movl	20(%rsp), %edi
	callq	_print_int
	xorl	%eax, %eax
	addq	$24, %rsp
	retq
	.cfi_endproc


.subsections_via_symbols
