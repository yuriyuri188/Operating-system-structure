#include "jobs.h"
#include <string.h>
#include <stdio.h>
#include "my_system_call.h"
#include <sys/wait.h>



 




/*=============================================================================
 * Initialize a single job slot to "empty"
 *===========================================================================*/

 void init_job(job* j)
 {
     j->pid        = 0;
     j->command[0] = '\0';
     j->time_stamp = 0;
     j->status     = 0;
     j->full       = false;
 }


/*==========================================================
 * Initialize the entire jobs array
 *  - job_counter = 0 (no background jobs yet)
 *  - smallest_free_id = 1 (first free job ID)
 *  - clear all slots jobs[0..JOBS_NUM_MAX]
 *========================================================*/

 void init_job_arr(job_arr* arr)
 {
     arr->job_counter      = 0;
     arr->smallest_free_id = 1;
 
     for (int i = 0; i <= JOBS_NUM_MAX; ++i) {
         init_job(&arr->jobs[i]);
     }
 }

/*================================================================
 * Find background job by PID
 *  - searches jobs[1..JOBS_NUM_MAX]
 *  - returns job_id if found, -1 otherwise
 *===================================================================*/
int find_by_pid(job_arr* arr, pid_t pid)
{
    for (int j = 1; j <= JOBS_NUM_MAX; ++j) {
        if (arr->jobs[j].full && arr->jobs[j].pid == pid) {
            return j;   // job_id
        }
    }
    return -1;
}

/*==============================================================
 * Change job status by PID
 *  - e.g. from BG to STOPPED when receiving SIGTSTP
 *  - returns 0 if changed, -1 if no such pid in the jobs table
 *==================================================================*/
 int job_status_change(job_arr* arr, pid_t pid, char new_status)
 {
     int idx = find_by_pid(arr, pid);
     if (idx == -1) {
         return -1;
     }
     arr->jobs[idx].status = new_status;
     return 0;
 }

/*
 * update_jobs:
 * Reap all finished child processes (background jobs) using waitpid(WNOHANG)
 * and remove them from the jobs list.
 *
 * This keeps the job list up-to-date, as required:
 * - before running commands
 * - before printing "jobs"
 * - before adding new jobs
 */
 void update_jobs(job_arr *arr)
 {
     int status;
     while (1) {
         // -1 = any child, WNOHANG = don't block if none finished
         pid_t pid = (pid_t)my_system_call(SYS_WAITPID, -1, &status, WNOHANG);
 
         if (pid <= 0) {
             // pid == 0  -> no child has finished
             // pid == -1 -> no children, or error (we ignore here)
             break;
         }
 
         // Find this pid in our jobs array
         int idx = find_by_pid(arr, pid);
 
         if (idx > 0) {
             // Background or stopped job slot
             delete_job(arr, idx);
         } else if (idx == 0) {
             // Very defensive: if somehow fg job got reaped here
             clear_fg_job(arr);
         } else {
             // pid not found in our table – ignore
         }
     }
 }


//###########################################################################


/*=============================================================================
* printing
=============================================================================*/


/*===========================================================
 * Print all background / stopped jobs
 *  - job IDs are indexes 1..JOBS_NUM_MAX
 *  - format: "[<id>] <command> : <pid> <secs> secs (Stopped)"
 *============================================================*/
 void print_all_bg_jobs(job_arr* arr)
 {
     for (int j = 1; j <= JOBS_NUM_MAX; ++j) {
         if (!arr->jobs[j].full)
             continue;
 
         printf("[%d] %s : %d %ld secs",
                j,
                arr->jobs[j].command,
                (int)arr->jobs[j].pid,
                (long)difftime(time(NULL), arr->jobs[j].time_stamp));
 
         if (arr->jobs[j].status == STOPPED) {
             printf(" (Stopped)");
         }
         printf("\n");
     }
 }




/*==================================================
 * Print current foreground job (slot 0), if any
 *  - used if you want to show fg info
 *=================================================*/
void print_fg_job(job_arr* arr)
{
    if (!arr->jobs[0].full)
        return;

    printf("%s : %d %ld secs\n",
           arr->jobs[0].command,
           (int)arr->jobs[0].pid,
           (long)difftime(time(NULL), arr->jobs[0].time_stamp));
}


//################################################################################################

/*=============================================================================
* job manipulation
=============================================================================*/



/*=============================================
 * Move a background/stopped job into foreground
 *  - job_id is in [1..JOBS_NUM_MAX]
 *  - copies job[job_id] into jobs[0] (FG slot)
 *  - frees job[job_id] and updates smallest_free_id
 *=============================================*/


 int move_job_to_fg(job_arr* arr, int job_id)
 {
     if (job_id < 1 || job_id > JOBS_NUM_MAX || !arr->jobs[job_id].full) {
         fprintf(stderr, "smash error: fg %d - job does not exist\n", job_id);
         return -1;
     }
 
     // Copy job data into fg slot 0
     arr->jobs[0].pid        = arr->jobs[job_id].pid;
     arr->jobs[0].status     = FG;
     arr->jobs[0].time_stamp = arr->jobs[job_id].time_stamp;
 
     strncpy(arr->jobs[0].command, arr->jobs[job_id].command, CMD_LENGTH_MAX - 1);
     arr->jobs[0].command[CMD_LENGTH_MAX - 1] = '\0';
 
     arr->jobs[0].full = true;
 
     // Clear background slot
     arr->jobs[job_id].full       = false;
     arr->jobs[job_id].pid        = 0;
     arr->jobs[job_id].command[0] = '\0';
     arr->jobs[job_id].status     = 0;
     arr->jobs[job_id].time_stamp = 0;
     arr->job_counter--;
 
     // Update smallest_free_id if this index is now the first free one
     if (job_id < arr->smallest_free_id) {
         arr->smallest_free_id = job_id;
     }
 
     return 0;
 }




/*=============================================================================
 * Add a job:
 *  - if status == FG: put it in jobs[0]
 *  - else (BG or STOPPED): find a free slot 1..JOBS_NUM_MAX using smallest_free_id
 *===========================================================================*/


int add_job(job_arr* arr, pid_t pid, const char* command, char status)
{
    if (status == FG) {
        // Foreground job goes into slot 0
        arr->jobs[0].pid        = pid;
        arr->jobs[0].status     = FG;
        arr->jobs[0].time_stamp = time(NULL);

        strncpy(arr->jobs[0].command, command, CMD_LENGTH_MAX - 1);
        arr->jobs[0].command[CMD_LENGTH_MAX - 1] = '\0';

        arr->jobs[0].full = true;
        return 0;
    }

    // Background / stopped job: use smallest_free_id for job ID in [1..JOBS_NUM_MAX]
    int id = arr->smallest_free_id;
    if (id < 1 || id > JOBS_NUM_MAX) {
        fprintf(stderr, "smash error: jobs list is full\n");
        return -1;
    }

    arr->jobs[id].pid        = pid;
    arr->jobs[id].status     = status;   // BG or STOPPED
    arr->jobs[id].time_stamp = time(NULL);

    strncpy(arr->jobs[id].command, command, CMD_LENGTH_MAX - 1);
    arr->jobs[id].command[CMD_LENGTH_MAX - 1] = '\0';

    arr->jobs[id].full = true;
    arr->job_counter++;

    // Recompute smallest_free_id (first free slot in 1..JOBS_NUM_MAX)
    int new_free = JOBS_NUM_MAX + 1;
    for (int i = 1; i <= JOBS_NUM_MAX; ++i) {
        if (!arr->jobs[i].full) {
            new_free = i;
            break;
        }
    }
    arr->smallest_free_id = new_free;

    return 0;
}




 // Clear foreground job slot (used after fg job finishes)
 
void clear_fg_job(job_arr* arr)
{
    if (!arr->jobs[0].full)
        return;

    arr->jobs[0].full       = false;
    arr->jobs[0].pid        = 0;
    arr->jobs[0].command[0] = '\0';
    arr->jobs[0].status     = 0;
    arr->jobs[0].time_stamp = 0;
}



/*=============================================================================
 * Delete a background / stopped job by job_id
 *  - job_id in [1..JOBS_NUM_MAX]
 *===========================================================================*/

void delete_job(job_arr* arr, int job_id)
{
    if (job_id < 1 || job_id > JOBS_NUM_MAX)
        return;

    if (!arr->jobs[job_id].full)
        return;

    arr->jobs[job_id].full       = false;
    arr->jobs[job_id].pid        = 0;
    arr->jobs[job_id].command[0] = '\0';
    arr->jobs[job_id].status     = 0;
    arr->jobs[job_id].time_stamp = 0;
    arr->job_counter--;

    if (job_id < arr->smallest_free_id) {
        arr->smallest_free_id = job_id;
    }
}
