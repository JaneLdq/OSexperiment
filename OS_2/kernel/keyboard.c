/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            keyboard.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "type.h"
#include "const.h"
#include "protect.h"
#include "proto.h"
#include "string.h"
#include "proc.h"
#include "global.h"
#include "keyboard.h"
#include "keymap.h"

PRIVATE KB_INPUT	kb_in;

PRIVATE	int	code_with_E0;
PRIVATE	int	shift_l;	/* l shift state */
PRIVATE	int	shift_r;	/* r shift state */
PRIVATE	int	alt_l;		/* l alt state	 */
PRIVATE	int	alt_r;		/* r left state	 */
PRIVATE	int	ctrl_l;		/* l ctrl state	 */
PRIVATE	int	ctrl_r;		/* l ctrl state	 */
PRIVATE int tab;		/* tab state */
PRIVATE	int	caps_lock;	/* Caps Lock	 */
PRIVATE	int	num_lock;	/* Num Lock	 */
PRIVATE	int	scroll_lock;	/* Scroll Lock	 */
PRIVATE	int	column;


PRIVATE u8	get_byte_from_kbuf();
PRIVATE int is_number_or_mark(u8 scan_code);
PRIVATE u32 handle_special_group(u8 scan_code);

/*======================================================================*
                            keyboard_handler
 *======================================================================*/
PUBLIC void keyboard_handler(int irq)
{
	u8 scan_code = in_byte(KB_DATA);

	if (kb_in.count < KB_IN_BYTES) {
		*(kb_in.p_head) = scan_code;
		kb_in.p_head++;
		if (kb_in.p_head == kb_in.buf + KB_IN_BYTES) {
			kb_in.p_head = kb_in.buf;
		}
		kb_in.count++;
	}
}


/*======================================================================*
                           init_keyboard
*======================================================================*/
PUBLIC void init_keyboard()
{
	kb_in.count = 0;
	kb_in.p_head = kb_in.p_tail = kb_in.buf;

	shift_l	= shift_r = 0;
	alt_l	= alt_r   = 0;
	ctrl_l	= ctrl_r  = 0;
	tab = 0;
	caps_lock = 0;

    put_irq_handler(KEYBOARD_IRQ, keyboard_handler);/*设定键盘中断处理程序*/
    enable_irq(KEYBOARD_IRQ);                       /*开键盘中断*/
}


/*======================================================================*
                           keyboard_read
*======================================================================*/
PUBLIC void keyboard_read()
{
	u8	scan_code;
	char	output[2];
	int	make;	/* 1: make;  0: break. */

	u32	key = 0;/* 用一个整型来表示一个键。比如，如果 Home 被按下，
			 * 则 key 值将为定义在 keyboard.h 中的 'HOME'。
			 */
	u32*	keyrow;	/* 指向 keymap[] 的某一行 */

	if(kb_in.count > 0){
		code_with_E0 = 0;

		scan_code = get_byte_from_kbuf();

		/* 下面开始解析扫描码 */
		if (scan_code == 0xE1) {
			int i;
			u8 pausebrk_scode[] = {0xE1, 0x1D, 0x45,
					       0xE1, 0x9D, 0xC5};
			int is_pausebreak = 1;
			for(i=1;i<6;i++){
				if (get_byte_from_kbuf() != pausebrk_scode[i]) {
					is_pausebreak = 0;
					break;
				}
			}
			if (is_pausebreak) {
				key = PAUSEBREAK;
			}
		}
		else if (scan_code == 0xE0) {
			scan_code = get_byte_from_kbuf();

			/* PrintScreen 被按下 */
			if (scan_code == 0x2A) {
				if (get_byte_from_kbuf() == 0xE0) {
					if (get_byte_from_kbuf() == 0x37) {
						key = PRINTSCREEN;
						make = 1;
					}
				}
			}
			/* PrintScreen 被释放 */
			if (scan_code == 0xB7) {
				if (get_byte_from_kbuf() == 0xE0) {
					if (get_byte_from_kbuf() == 0xAA) {
						key = PRINTSCREEN;
						make = 0;
					}
				}
			}
			/* 不是PrintScreen, 此时scan_code为0xE0紧跟的那个值. */
			if (key == 0) {
				code_with_E0 = 1;
			}
		}
		/* 大写锁定按下 */
		else if(scan_code == 0x3A){
			key = CAPS_LOCK;
			caps_lock = !caps_lock;
			make = 1;
		}
		else if(scan_code == 0xBA){
			key = CAPS_LOCK;
			make = 0;
		}
		if ((key != PAUSEBREAK) && (key != PRINTSCREEN) && (key != CAPS_LOCK)) {
			/* 首先判断Make Code 还是 Break Code */
			make = (scan_code & FLAG_BREAK ? 0 : 1);

			/* 先定位到 keymap 中的行 */
			keyrow = &keymap[(scan_code & 0x7F) * MAP_COLS];
			
			column = 0;
			/* 根据shift判定大小写或上下字符*/
			if (shift_l || shift_r ) {
					column = 1;
					/* 对于a-z，shift + caps_lock 仍然为小写*/
					if((is_number_or_mark(scan_code)==0) && caps_lock)
						column = 0;
			}
			/* 不按shift，但大写锁打开，字母取column1，其他取colomn0*/
			if((!(shift_l||shift_r)) && caps_lock && (!is_number_or_mark(scan_code))){
				column = 1;
			}
			if (code_with_E0) {
				column = 2; 
				code_with_E0 = 0;
			}
			
			/* shift + tab组合键*/
			if((shift_l||shift_r)&&tab){
				key = handle_special_group(scan_code);
				if(key==-1)
				key = keyrow[column];
			}else{
				/* 如果不是六个特殊组合键则按普通取值*/
				key = keyrow[column];
			}
			
			switch(key) {
			case SHIFT_L:
				shift_l = make;
				break;
			case SHIFT_R:
				shift_r = make;
				break;
			case CTRL_L:
				ctrl_l = make;
				break;
			case CTRL_R:
				ctrl_r = make;
				break;
			case ALT_L:
				alt_l = make;
				break;
			case ALT_R:
				alt_l = make;
				break;
			case TAB:
				tab = make;
				break;
			default:
				break;
			}

			if (make) { /* 忽略 Break Code */
				key |= shift_l	? FLAG_SHIFT_L	: 0;
				key |= shift_r	? FLAG_SHIFT_R	: 0;
				key |= ctrl_l	? FLAG_CTRL_L	: 0;
				key |= ctrl_r	? FLAG_CTRL_R	: 0;
				key |= alt_l	? FLAG_ALT_L	: 0;
				key |= alt_r	? FLAG_ALT_R	: 0;
				in_process(key);
			}
		}
	}
}

/*======================================================================*
			    get_byte_from_kbuf
 *======================================================================*/
PRIVATE u8 get_byte_from_kbuf()       /* 从键盘缓冲区中读取下一个字节 */
{
        u8 scan_code;

        while (kb_in.count <= 0) {}   /* 等待下一个字节到来 */

        disable_int();
        scan_code = *(kb_in.p_tail);
        kb_in.p_tail++;
        if (kb_in.p_tail == kb_in.buf + KB_IN_BYTES) {
                kb_in.p_tail = kb_in.buf;
        }
        kb_in.count--;
        enable_int();

        return scan_code;
}


/*======================================================================*
			    is_number_or_mark
 *======================================================================*/
PRIVATE int is_number_or_mark(u8 scan_code){
	u8 num_mark[] = 
	{	0x0B, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
		0x08, 0x09, 0x0A, 0x29, 0x0C, 0x0D, 0x2B, 
		0x1B, 0x27, 0x28, 0x33, 0x34, 0x35, 0x1A
		};
	int i;
	for(i=0; i<21;++i){
		if(scan_code == num_mark[i])
			return 1;
	}
	return 0;
}

/*======================================================================*
			    handle_special_group
 *======================================================================*/
PRIVATE u32 handle_special_group(u8 scan_code){
	/* key = {W,S,X,E,D,C} */
	u8 key_ch[] = {0x11,0x1F,0x2D,0x12,0x20,0x2E};
	/* value = { E,D,C,R,F,V }*/
	u8 value_ch[] = {'E','D','C','R','F','V'};
	int i=0;
	for(i=0;i<6;++i){
		if(scan_code == key_ch[i])
			break;
	}
	if(i<=5){
		return value_ch[i];
	}
	return -1;
}