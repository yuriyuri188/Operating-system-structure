#include <stdio.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include "jobs.h"
#include "signals.h"

extern job_arr job_list;

/* ----------------------------------------------------------
   Helpers
   ---------------------------------------------------------- */

static int is_foreground_job_active() {
    return job_list.jobs[0].full;
}

static pid_t get_foreground_pid() {
    return job_list.jobs[0].pid;
}

static void block_all_signal_delivery(sigset_t* previous_mask) {
    sigset_t all_signals;
    sigfillset(&all_signals);
    sigprocmask(SIG_SETMASK, &all_signals, previous_mask);
}

static void restore_signal_mask(sigset_t* previous_mask) {
    sigprocmask(SIG_SETMASK, previous_mask, NULL);
}

/* ----------------------------------------------------------
   Install handlers
   ---------------------------------------------------------- */
void install_signal_handlers() {

    struct sigaction sa_sigint = {0};
    sa_sigint.sa_handler = ctrl_c;
    sa_sigint.sa_flags = SA_RESTART;
    sigemptyset(&sa_sigint.sa_mask);
    sigaction(SIGINT, &sa_sigint, NULL);

    struct sigaction sa_sigtstp = {0};
    sa_sigtstp.sa_handler = ctrl_z;
    sa_sigtstp.sa_flags = SA_RESTART;
    sigemptyset(&sa_sigtstp.sa_mask);
    sigaction(SIGTSTP, &sa_sigtstp, NULL);
}

/* ----------------------------------------------------------
   CTRL+C (SIGINT)
   ---------------------------------------------------------- */
void ctrl_c(int sig) {
    sigset_t previous_mask;
    block_all_signal_delivery(&previous_mask);

    printf("\nsmash: caught CTRL+C\n");
    fflush(stdout);

    if (is_foreground_job_active()) {
        pid_t pid = get_foreground_pid();
        if (kill(pid, SIGKILL) == 0) {
            printf("smash: process %d was killed\n", pid);
            int cur_status = 0; 
            remove_job_from_fg(&job_list, pid, cur_status);
        } else {
            perror("smash error: kill failed");
        }
    }

    fflush(stdout);
    restore_signal_mask(&previous_mask);
}

/* ----------------------------------------------------------
   CTRL+Z (SIGTSTP)
   ---------------------------------------------------------- */
void ctrl_z(int sig) {
    sigset_t previous_mask;
    block_all_signal_delivery(&previous_mask);

    printf("\nsmash: caught CTRL+Z\n");
    fflush(stdout);

    if (is_foreground_job_active()) {
        pid_t pid = get_foreground_pid();
        if (kill(pid, SIGSTOP) == 0) {
            printf("smash: process %d was stopped\n", pid);  
            int cur_status = (1 << 8);        
            remove_job_from_fg(&job_list, pid, cur_status);
        } else {
            perror("smash error: kill failed");
        }
    }

    fflush(stdout);
    restore_signal_mask(&previous_mask);
}
