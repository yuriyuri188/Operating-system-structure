#ifndef SIGNALS_H
#define SIGNALS_H

#include <signal.h>

/* install all signal handlers */
void install_signal_handlers(void);

/* compatibility wrapper for your main (it calls MainHandleConfigPack) */
void MainHandleConfigPack(void);

/* signal handlers */
void ctrl_c(int sig);
void ctrl_z(int sig);

#endif /* SIGNALS_H */
