#ifndef JOBS_H
#define JOBS_H

#include <time.h>
#include <sys/types.h>
#include <stdbool.h>
#include "commands.h"   // for CMD_LENGTH_MAX and JOBS_NUM_MAX

/*=============================================================================
* flags
=============================================================================*/
#define FG       '1'
#define BG       '2'
#define STOPPED  '3'


/*=============================================================================
* structs
=============================================================================*/
typedef struct job {
    pid_t pid;
    char command[CMD_LENGTH_MAX];
    time_t time_stamp;
    char status;
    bool full;
    //char prev_wd[CMD_LENGTH_MAX];
    //bool is_external;
} job;

typedef struct job_arr {
    job jobs[JOBS_NUM_MAX + 1];   /* index 0 = FG , 1..MAX_ARGS = BG */
    int job_counter;     // how many BG/STOPPED jobs (not counting fg)
    int smallest_free_id;  // smallest free index in 1..JOBS_NUM_MAX
} job_arr;

/*=============================================================================
* init
=============================================================================*/
void init_job(job* j);

void init_job_arr(job_arr* arr);

/*=============================================================================
* helpers
=============================================================================*/
// find background job index by pid, returns job_id in [1..JOBS_NUM_MAX] or -1
int find_by_pid(job_arr* arr, pid_t pid);

// change status of job by pid, returns 0 on success, -1 if not found
int job_status_change(job_arr* arr, pid_t pid, char cur_status);

/*=============================================================================
* printing
=============================================================================*/
void print_all_bg_jobs(job_arr* arr);

void print_fg_job(job_arr* arr);

/*=============================================================================
* job manipulation
=============================================================================*/
// add job, status should be FG or BG (STOPPED is set later when signal happens)
int add_job(job_arr* arr, pid_t pid, const char* command, char status);

// move BG/STOPPED job [job_id] (1..JOBS_NUM_MAX) into foreground slot (jobs[0])
int move_job_to_fg(job_arr* arr, int job_id);

// clear fg slot after job finished
void clear_fg_job(job_arr *arr);

void update_jobs(job_arr *arr);




/*=============================================================================
* delete
=============================================================================*/
// delete BG/STOPPED job by job_id in [1..JOBS_NUM_MAX]
void delete_job(job_arr* arr, int job_id);

//void delete_complex_job(job_arr* arr, pid_t complex_pid, int complex_i, int   smallest_free_id);

#endif /* JOBS_H */
