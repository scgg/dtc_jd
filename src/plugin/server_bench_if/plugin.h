#include "benchapi.h"

#ifndef __plugin_http
#define __plugin_http
#ifdef __cplusplus
extern "C" {
#endif
int handle_init(int argc, char **argv, int threadtype);
int handle_input(const char* recv_buf, int recv_len, const skinfo_t* skinfo_t);
int handle_process(char *recv_buf, int real_len, char **send_buf, int *send_len, const skinfo_t* skinfo_t);
int handle_open(char **, int *, const skinfo_t*);
int handle_close(const skinfo_t*);
void handle_fini(int threadtype);
int handle_timer_notify(int plugin_timer_interval, void** name);

#ifdef __cplusplus
}
#endif
#endif
