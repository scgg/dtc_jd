#ifndef __TTC_PROCTITLE_H
#define __TTC_PROCTITLE_H

extern "C" {
	extern void init_proc_title(int, char **);
	extern void set_proc_title(const char *title);
	extern void set_proc_name(const char *);
	extern void get_proc_name(char *);
}

#endif
