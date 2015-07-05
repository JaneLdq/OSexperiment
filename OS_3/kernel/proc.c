
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                               proc.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "type.h"
#include "const.h"
#include "protect.h"
#include "tty.h"
#include "string.h"
#include "proc.h"
#include "global.h"
#include "proto.h"

PRIVATE int sleep_sq(SEM* s, PROCESS* p);
PRIVATE int wakeup_sq(SEM* s);
/*======================================================================*
                              schedule
 *======================================================================*/
PUBLIC void schedule()
{
	PROCESS* p;
	int	 greatest_ticks = 0;

	
	while (!greatest_ticks) {
		for (p = proc_table; p < proc_table+NR_TASKS+NR_PROCS; p++) {
			if((!p->sleep) && (!p->wait)){
				if (p->ticks > greatest_ticks) {
					greatest_ticks = p->ticks;
					p_proc_ready = p;
				}
			}
		}
		
		if (!greatest_ticks) {
			for(p=proc_table;p<proc_table+NR_TASKS+NR_PROCS;p++) {
				if(!p->sleep&&(!p->wait)){
					p->ticks = p->priority;
				}
			}
		}	
	}
}

/*======================================================================*
                           sys_get_ticks
 *======================================================================*/
PUBLIC int sys_get_ticks()
{
	return ticks;
}

PUBLIC void sys_process_sleep(int seconds, PROCESS* p)
{
	p->sleep = seconds/10;
}

PUBLIC int sys_sem_p(SEM* s, PROCESS* p)
{	

/*	disp_str("P->");
	disp_str(s->name);
	disp_str(" ->");
	disp_str(p->p_name);
	disp_str("    ");
*/	
	s->value--;
	if(s->value<0){
		sleep_sq(s,p);
		schedule();
	}
	
}

PUBLIC int sys_sem_v(SEM* s)
{	
/*	disp_str("V->");
	disp_str(s->name);
	disp_str("    ");
*/	
	s->value++;
	if(s->value <= 0){
		wakeup_sq(s);
	}
	
}

PRIVATE int sleep_sq(SEM* s, PROCESS* p){
/*	disp_str("sleep->");
	disp_str(p->p_name);
	disp_str("     ");
*/
//	if((s->name)[0]=='c'){
//		disp_color_str("Barber is waiting for customers.\n",BARBER_COLOR);
//	}else if((s->name)[0]=='b'){
		disp_color_str(p->p_name,DEFAULT_CHAR_COLOR);
		disp_color_str(" is wait for source.\n",DEFAULT_CHAR_COLOR);
//	}
	if(s->count>=MAX_QUE_LEN)
		return -1;
	p->wait = 1;
	s->list[s->tail] = p;
	s->tail = (s->tail + 1) % MAX_QUE_LEN;
	s->count++;
	return 0;
}

PRIVATE int wakeup_sq(SEM* s){
	if(s->count==0)
		return 0;
	PROCESS* p = s->list[s->head];
	p->wait=0;
/*	disp_str("wakeup->");
	disp_str(p->p_name);
	disp_str("    ");
*/	
	s->head = (s->head + 1) % MAX_QUE_LEN;
	s->count--;
	return 0;
}

/*======================================================================*
                              sys_disp_str
*======================================================================*/
PUBLIC int sys_disp_str(char* buf, PROCESS* p_proc)
{		int len = strlen(buf);
		int color = DEFAULT_CHAR_COLOR;
		if(p_proc->type == BARBER){
			color = BARBER_COLOR;
		}else if(p_proc->type == CUSTOMER_A){
			color = CUSTOMER_COLOR;
		}else if(p_proc->type == CUSTOMER_B){
			color = CUSTOMER_COLOR_B;
		}else if(p_proc->type == CUSTOMER_C){
			color = CUSTOMER_COLOR_C;
		}
		disp_color_str(buf,color);
		disp_color_str("",color);
        return 0;
}
