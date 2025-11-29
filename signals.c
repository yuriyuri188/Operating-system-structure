#include <stdio.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include "jobs.h"
#include "signals.h"
#include "my_system_call.h"

extern job_arr job_list;

/* ----------------------------------------------------------
   Helpers
   ---------------------------------------------------------- */

// is there a foreground job in slot 0? 
static int is_foreground_job_active() {
    return job_list.jobs[0].full;
}

// get pid of foreground job
static pid_t get_foreground_pid() {
    return job_list.jobs[0].pid;
}

// block delivery of all signals (save previous mask)
static void block_all_signal_delivery(sigset_t* previous_mask) {
    sigset_t all_signals;
    sigfillset(&all_signals);
    sigprocmask(SIG_SETMASK, &all_signals, previous_mask);
}


// restore previous signal mask
static void restore_signal_mask(sigset_t* previous_mask) {
    sigprocmask(SIG_SETMASK, previous_mask, NULL);
}

/* ----------------------------------------------------------
   Install handlers
   ---------------------------------------------------------- */
void install_signal_handlers(void) {

    
    struct sigaction sa_sigint;
    struct sigaction sa_sigtstp;
    
        // CTRL+C → SIGINT
    sigemptyset(&sa_sigint.sa_mask);
    sa_sigint.sa_handler = ctrl_c;
    sa_sigint.sa_flags   = SA_RESTART;
    sigaction(SIGINT, &sa_sigint, NULL);
    
        // CTRL+Z → SIGTSTP
    sigemptyset(&sa_sigtstp.sa_mask);
    sa_sigtstp.sa_handler = ctrl_z;
    sa_sigtstp.sa_flags   = SA_RESTART;
    sigaction(SIGTSTP, &sa_sigtstp, NULL);
}

/* wrapper so smash.c can call MainHandleConfigPack() */
void MainHandleConfigPack(void)
{
    install_signal_handlers();
}


/* ----------------------------------------------------------
   CTRL+C (SIGINT)
   ---------------------------------------------------------- */
   void ctrl_c(int sig)
   {
       (void)sig;  // unused
   
       sigset_t previous_mask;
       block_all_signal_delivery(&previous_mask);
   
       printf("\nsmash: caught CTRL+C\n");
       fflush(stdout);
   
       if (is_foreground_job_active()) {
           pid_t pid = get_foreground_pid();
   
           // send SIGKILL to the foreground process via wrapper
           long r = my_system_call(SYS_KILL, pid, SIGKILL);
           if (r == -1) {
               perror("smash error: kill failed");
           } else {
               printf("smash: process %d was killed\n", pid);
           }
       }
   
       fflush(stdout);
       restore_signal_mask(&previous_mask);
   }

/* ----------------------------------------------------------
   CTRL+Z (SIGTSTP)
   ---------------------------------------------------------- */
   void ctrl_z(int sig)
   {
       (void)sig;  // unused
   
       sigset_t previous_mask;
       block_all_signal_delivery(&previous_mask);
   
       printf("\nsmash: caught CTRL+Z\n");
       fflush(stdout);
   
       if (is_foreground_job_active()) {
           pid_t pid = get_foreground_pid();
   
           // send SIGSTOP to the foreground process via wrapper
           long r = my_system_call(SYS_KILL, pid, SIGSTOP);
           if (r == -1) {
               perror("smash error: kill failed");
           } else {
               printf("smash: process %d was stopped\n", pid);
           }
           // We do NOT modify job_list here:
           // run_external_command + waitpid(WUNTRACED) will see WIFSTOPPED
           // and handle moving the job to background as STOPPED.
       }
   
       fflush(stdout);
       restore_signal_mask(&previous_mask);
   }
