
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            global.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#define GLOBAL_VARIABLES_HERE

#include "type.h"
#include "const.h"
#include "protect.h"
#include "tty.h"
#include "proc.h"
#include "global.h"
#include "proto.h"

PUBLIC	PROCESS	proc_table[NR_TASKS + NR_PROCS];

PUBLIC	TASK	task_table[NR_TASKS] = {
	{task_tty, STACK_SIZE_TTY, "tty"}};

PUBLIC  TASK    user_proc_table[NR_PROCS] = {
	{A, STACK_SIZE_TESTA, "A"},
	{B, STACK_SIZE_TESTB, "B"},
	{C, STACK_SIZE_TESTC, "C"},
	{D, STACK_SIZE_TESTD, "D"},
	{E, STACK_SIZE_TESTE, "E"}};

PUBLIC	char		task_stack[STACK_SIZE_TOTAL];

PUBLIC	irq_handler	irq_table[NR_IRQ];

PUBLIC	system_call	sys_call_table[NR_SYS_CALL] = {
	sys_get_ticks, 
	sys_disp_str,
	sys_process_sleep,
	sys_sem_p,
	sys_sem_v
	};


