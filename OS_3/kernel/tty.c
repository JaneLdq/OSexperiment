
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                               tty.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "type.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "proc.h"
#include "tty.h"
#include "global.h"
#include "keyboard.h"
#include "proto.h"

/* 字体颜色：黑底白字、黑底蓝字、黑底红字，黑底黄字、黑底青字*/
PRIVATE int current_color = 0x07;
PRIVATE int clear_color = 0x0F;
PRIVATE int tab_color = 0x08;
PRIVATE void print_output(char* output, int color);

/*======================================================================*
                           task_tty
 *======================================================================*/
PUBLIC void task_tty()
{
	while (1) {
		keyboard_read();
	}
}

/*======================================================================*
				in_process
 *======================================================================*/
PUBLIC void in_process(u32 key)
{
        char output[2] = {'\0', '\0'};

        if (!(key & FLAG_EXT)) {
            output[0] = key & 0xFF;
			print_output(output,current_color);
        }
        else {
            int raw_code = key & MASK_RAW;
            switch(raw_code) {
			/* 回车键换行*/
			case ENTER:
				output[0] = '\n';
				print_output(output,clear_color);
				break;
			/* TAB键4个空格*/
			case TAB:
				output[0] = ' ';
				int i=0;
				for(i=0;i<4;++i)
					print_output(output,tab_color);
				break;
			/* 退格键  */
			case BACKSPACE:
				if(disp_pos>=2){
					//check_space();
					//充值光标 
					out_byte(CRTC_ADDR_REG, CURSOR_H);
					out_byte(CRTC_DATA_REG, ((disp_pos/2)>>8)&0xFF);
					out_byte(CRTC_ADDR_REG, CURSOR_L);
					out_byte(CRTC_DATA_REG, (disp_pos/2)&0xFF);
				}
				break;
            default:
                break;
            }
        }
}

/*======================================================================*
				clear_screen
 *======================================================================*/
PUBLIC void clear_screen(){
	disp_pos = 0;
	/* 通过打印空格的方式清空屏幕25行，并把 disp_pos 清零 */
	int i;
	for(i=0;i<80*25;i++){
		disp_color_str(" ",clear_color);
	}
	disp_pos = 0;
	disable_int();
	out_byte(CRTC_ADDR_REG, START_ADDR_H);
    out_byte(CRTC_DATA_REG, ((0) >> 8) & 0xFF);
    out_byte(CRTC_ADDR_REG, START_ADDR_L);
    out_byte(CRTC_DATA_REG, (0) & 0xFF);
	out_byte(CRTC_ADDR_REG, CURSOR_H);
	out_byte(CRTC_DATA_REG, ((0)>>8)&0xFF);
	out_byte(CRTC_ADDR_REG, CURSOR_L);
	out_byte(CRTC_DATA_REG, (0)&0xFF);
	enable_int();
}

/*======================================================================*
				*print_output
 *======================================================================*/
PRIVATE void print_output(char* output, int color){
    disp_color_str(output, color);
	disable_int();
	out_byte(CRTC_ADDR_REG, CURSOR_H);
	out_byte(CRTC_DATA_REG, ((disp_pos/2)>>8)&0xFF);
	out_byte(CRTC_ADDR_REG, CURSOR_L);
	out_byte(CRTC_DATA_REG, (disp_pos/2)&0xFF);
	enable_int();	 
	 
 }

