
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            proto.h
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/* klib.asm */
PUBLIC void	out_byte(u16 port, u8 value);
PUBLIC u8	in_byte(u16 port);
PUBLIC void	disp_str(char * info);
PUBLIC void	disp_color_str(char * info, int color);

/*string.asm*/
PUBLIC int strlen(char* str);

/* protect.c */
PUBLIC void	init_prot();
PUBLIC u32	seg2phys(u16 seg);

/* klib.c */
PUBLIC void	delay(int time);
PUBLIC void disp_int(int i);
/* kliba.asm */
void disable_int();
void enable_int();

/* kernel.asm */
void restart();

/* main.c */
void A();
void B();
void C();
void D();
void E();

/* i8259.c */
PUBLIC void put_irq_handler(int irq, irq_handler handler);
PUBLIC void spurious_irq(int irq);

/* clock.c */
PUBLIC void clock_handler(int irq);
PUBLIC void init_clock();

/* keyboard.c */
PUBLIC void init_keyboard();

/* tty.c */
PUBLIC void task_tty();
PUBLIC void clear_screen();

/* 以下是系统调用相关 */

/* 系统调用 - 系统级 */
/* proc.c */
PUBLIC  int     sys_get_ticks();
PUBLIC 	void 	sys_process_sleep(int seconds, PROCESS* p_proc);
PUBLIC 	int 	sys_disp_str(char* buf, PROCESS* p_proc);
PUBLIC 	int 	sys_sem_p(SEM* sem, PROCESS* p_proc);
PUBLIC 	int 	sys_sem_v(SEM* sem);
/* syscall.asm */
PUBLIC  void    sys_call();             /* int_handler */

/* 系统调用 - 用户级 */
PUBLIC  int     get_ticks();
PUBLIC  void 	sleep(int seconds);
PUBLIC  int	    show_str(char* str);
PUBLIC  int	P(SEM* sem);
PUBLIC  int	V(SEM* sem);

