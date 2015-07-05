
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            main.c
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
#include "proto.h"

void print_wait();
void add_ID();

/*======================================================================*
                            kernel_main
 *======================================================================*/
PUBLIC int kernel_main()
{
	disp_str("-----\"kernel_main\" begins-----\n");

	TASK*		p_task		= task_table;
	PROCESS*	p_proc		= proc_table;
	char*		p_task_stack	= task_stack + STACK_SIZE_TOTAL;
	u16		selector_ldt	= SELECTOR_LDT_FIRST;
	int i;
    u8              privilege;
    u8              rpl;
    int             eflags;
	for (i = 0; i < NR_TASKS+NR_PROCS; i++) {
                if (i < NR_TASKS) {     /* 任务 */
                        p_task    = task_table + i;
                        privilege = PRIVILEGE_TASK;
                        rpl       = RPL_TASK;
                        eflags    = 0x1202; /* IF=1, IOPL=1, bit 2 is always 1 */
                }
                else {                  /* 用户进程 */
                        p_task    = user_proc_table + (i - NR_TASKS);
                        privilege = PRIVILEGE_USER;
                        rpl       = RPL_USER;
                        eflags    = 0x202; /* IF=1, bit 2 is always 1 */
                }

		strcpy(p_proc->p_name, p_task->name);	// name of the process
		p_proc->pid = i;			// pid

		p_proc->ldt_sel = selector_ldt;

		memcpy(&p_proc->ldts[0], &gdt[SELECTOR_KERNEL_CS >> 3],
		       sizeof(DESCRIPTOR));
		p_proc->ldts[0].attr1 = DA_C | privilege << 5;
		memcpy(&p_proc->ldts[1], &gdt[SELECTOR_KERNEL_DS >> 3],
		       sizeof(DESCRIPTOR));
		p_proc->ldts[1].attr1 = DA_DRW | privilege << 5;
		p_proc->regs.cs	= (0 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
		p_proc->regs.ds	= (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
		p_proc->regs.es	= (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
		p_proc->regs.fs	= (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
		p_proc->regs.ss	= (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
		p_proc->regs.gs	= (SELECTOR_KERNEL_GS & SA_RPL_MASK) | rpl;

		p_proc->regs.eip = (u32)p_task->initial_eip;
		p_proc->regs.esp = (u32)p_task_stack;
		p_proc->regs.eflags = eflags;

		p_proc->nr_tty = 0;
		p_proc->sleep = 0;
		p_proc->wait = 0;
		
		p_task_stack -= p_task->stacksize;
		p_proc++;
		p_task++;
		selector_ldt += 1 << 3;
	}

	proc_table[0].ticks = proc_table[0].priority = 	6;
	proc_table[1].ticks = proc_table[1].priority =  6;
	proc_table[2].ticks = proc_table[2].priority =  6;
	proc_table[3].ticks = proc_table[3].priority =  6;
	proc_table[4].ticks = proc_table[4].priority =  6;
	proc_table[5].ticks = proc_table[5].priority =  6;
	
	proc_table[1].type = OTHER;
	proc_table[2].type = BARBER;
	proc_table[3].type = CUSTOMER_A;
	proc_table[4].type = CUSTOMER_B;
	proc_table[5].type = CUSTOMER_C;
	
	k_reenter = 0;
	ticks = 0;

	waiting = 0;
	c_id = 0;
	
	//信号量
	mutex.value=1;
	mutex.head=0;
	mutex.tail=0;
	mutex.count=0;
	mutex.name="m";
	barbers.value=0;
	barbers.head=0;
	barbers.tail=0;
	barbers.count=0;
	barbers.name="b";
	customers.value=0;
	customers.head=0;
	customers.tail=0;
	customers.count=0;
	customers.name="c";
	
	p_proc_ready	= proc_table;

	init_clock();
    init_keyboard();
	clear_screen();
	restart();

	while(1){}
}

/*======================================================================*
                               TestA
 *======================================================================*/
void A()
{	
	sleep(2000);
	while (1) {
		//show_str("A is running...\n");
		milli_delay(2000);
	}
}

/*======================================================================*
                               TestB
 *======================================================================*/
void B()
{
	show_str("Barber is sleeping.\n");
	while(1){
		//sleep(10000);
		P(&customers);
		P(&mutex);
		waiting--;
		V(&barbers);
		V(&mutex);
		show_str("Barber is cutting hair.\n");
		milli_delay(10000);
	}
}

/*======================================================================*
                               TestC
 *======================================================================*/
void C()
{
	while(1){
		P(&mutex);
		add_ID();
		if(waiting<CHAIRS){
			waiting++;
			//print_wait();
			show_str("C wakes the barber up.\n");
			V(&customers);
			V(&mutex);
			P(&barbers);
			show_str("C getting haircut\n");
		}else{
			//print_wait();
			show_str("No chair, C leave.\n");
			V(&mutex);
		}
		milli_delay(5000);
	}
}


/*======================================================================*
                               TestD
 *======================================================================*/
void D()
{
	while(1){
		
		P(&mutex);
		add_ID();
		if(waiting<CHAIRS){
			waiting++;
			show_str("D wakes the barber up.\n");
			V(&customers);
			V(&mutex);
			P(&barbers);
			show_str("D getting haircut\n");
		}else{
			//print_wait();
			show_str("No chair, D leave.\n");
			V(&mutex);
		}
		milli_delay(5000);
	}
}


/*======================================================================*
                               TestE
 *======================================================================*/
void E()
{
	while(1){
		P(&mutex);
		add_ID();
		if(waiting<CHAIRS){
			waiting++;
			show_str("E wakes the barber up.\n");
			V(&customers);
			V(&mutex);
			P(&barbers);
			show_str("E getting haircut.\n");
		}else{
			//print_wait();
			show_str("No chair, E leave.\n");
			V(&mutex);
		}
		milli_delay(5000);
	}
}


void print_wait(){
	show_str("Waiting Num: ");
	disp_int(waiting);
	show_str("\n");
}

void add_ID(){
	c_id++;
	show_str("#NO. ");
	disp_int(c_id);
	show_str("    ");
}
