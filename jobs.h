#ifndef JOBS_H
#define JOBS_H

#include <time.h>
#include <sys/types.h>
#include <stdbool.h>

/*=============================================================================
* defines
=============================================================================*/
#define FG       '1'
#define BG       '2'
#define STOPPED  '3'

#define MAX_LINE_SIZE 80
#define MAX_ARGS 100

/*=============================================================================
* structs
=============================================================================*/
typedef struct job {
    pid_t pid;
    char command[MAX_LINE_SIZE];
    time_t time_stamp;
    char status;
    bool full;
    char prev_wd[MAX_LINE_SIZE];
    bool is_external;
} job;

typedef struct job_arr {
    job jobs[MAX_ARGS + 1];   /* index 0 = FG , 1..MAX_ARGS = BG */
    int job_counter;
    int smallest_free_id;
} job_arr;

/*=============================================================================
* init
=============================================================================*/
void init_job(job* j);
void init_job_arr(job_arr* arr);

/*=============================================================================
* helpers
=============================================================================*/
int find_by_id(job_arr* arr, pid_t pid);
int job_status_change(job_arr* arr, pid_t pid, char cur_status);

/*=============================================================================
* printing
=============================================================================*/
void print_all_bg_jobs(job_arr* arr);
void print_fg_job(job_arr* arr);

/*=============================================================================
* job manipulation
=============================================================================*/
int add_job(job_arr* arr,
            pid_t pid,
            char status,
            const char* command);

int move_job_to_fg(job_arr* arr, pid_t pid);
void remove_job_from_fg(job_arr* arr);

/*=============================================================================
* delete
=============================================================================*/
void delete_job(job_arr* arr, pid_t pid);

#endif /* JOBS_H */
