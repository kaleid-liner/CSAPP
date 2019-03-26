.intel_syntax noprefix
.quad 0, 0, 0, 0 ,0
.quad 0x5561DCA8

movq rdi, 0x5561DD30
mov qword ptr [rsp], 0x4018FA
ret

.quad 0, 0, 0, 0, 0
.quad 0, 0, 0, 0, 0
.quad 0, 0, 0, 0, 0

.ascii "59b997fa\0"

