#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#define FG       '1'
#define BG       '2'
#define STOPPED  '3'

#define MAX_ARGS 100   

typedef struct job {
    pid_t pid;
    char command[256];
    time_t time_stamp;
    char status;
    int full;
    char prev_wd[256];
} job;

typedef struct job_arr {
    job jobs[MAX_ARGS + 2];
    int job_counter;
    int smallest_id;
} job_arr;


/*=============================================================================
* init functions 
=============================================================================*/

void init_job(job* j) {
    j->pid = 0;
    j->command[0] = '\0';
    j->time_stamp = 0;
    j->status = 0;
    j->full = 0;
    j->prev_wd[0] = '\0';
}

void init_job_arr(job_arr* arr) {
    arr->job_counter = 0;
    arr->smallest_id = 1;
    for (int i = 0; i < MAX_ARGS + 1; i++) {
        init_job(&arr->jobs[i]);
    }
}


/*=============================================================================
* C equivalents of your class methods
=============================================================================*/

int find_by_id(job_arr* arr, pid_t pid) {
    for (int j = 0; j <= MAX_ARGS + 1; j++) {
        if ((arr->jobs[j].pid == pid) && arr->jobs[j].full) {
            return j;
        }
    }
    return -1;
}

int job_status_change (job_arr* arr, pid_t pid, char cur_status){
    for(int j=0, j<MAX_ARGS+1, j++){
        if(arr->jobs[j].pid == pid){
            arr->jobs[j].status=cur_status;
            return 0;
        }
    }
    printf("status change failed\n")
    return 1;    
}

void print_all_jobs(job_arr* arr) {
    for (int j = 1; j < MAX_ARGS + 1; j++) {
        if (arr->jobs[j].full) {
            printf("[%d] %s: %d %ld secs",
                   j,
                   arr->jobs[j].command,
                   arr->jobs[j].pid,
                   (long)difftime(time(NULL), arr->jobs[j].time_stamp)
            );

            if (arr->jobs[j].status == STOPPED) {
                printf(" (stopped)");
            }
            printf("\n");
        }
    }
}
