[section .data]
dircolor db 27, '[49;36m', 0 		;27，转义字符
    .len equ $ - dircolor
defaultcolor db 27, '[0;0m', 0 
    .len equ $ - defaultcolor

[section .text]

global change_color
global def_color
global my_print

my_print:
	mov	edx,[esp+8]		;取出字符串长度
	mov	ecx,[esp+4]		;取出字符串首地址
	mov	ebx,1			;系统调用
	mov	eax,4 
	int	0x80
	ret

change_color:
    mov ecx, dircolor
    mov edx, dircolor.len
	mov eax, 4
    mov ebx, 1
    int 0x80
    ret

def_color:
    mov ecx, defaultcolor
    mov edx, defaultcolor.len
	mov eax, 4
    mov ebx, 1
    int 0x80

    ret
