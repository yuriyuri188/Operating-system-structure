#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>


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
    job jobs[MAX_ARGS+1];
    int job_counter;
    int smallest_free_id;
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
    arr->smallest_free_id = 1;
    for (int i = 0; i < MAX_ARGS + 1; i++) {
        init_job(&arr->jobs[i]);
    }
}


/*=============================================================================
* 
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
    for(int j=1; j<MAX_ARGS+1; j++){
        if ((arr->jobs[j].pid == pid)){
            arr->jobs[j].status=cur_status;
            return 0;
        }
    }
    printf("status change failed\n")
    return 1;    
}

void print_all_bg_jobs(job_arr* arr) {
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

void print_fg_job(job_arr* arr) {
     printf("[%d] %s: %d %ld secs",
                   0,
                   arr->jobs[0].command,
                   arr->jobs[0].pid,
                   (long)difftime(time(NULL), arr->jobs[0].time_stamp)
            );
}


int move_job_to_fg(job_arr* arr, pid_t pid){
    for (int j = 1; j < MAX_ARGS + 1; j++) {
        if (arr->jobs[j].pid == pid) {
            arr->jobs[0].pid = pid;
			strcpy(arr->jobs[0].command,arr->jobs[j].command);
            arr->jobs[0].time_stamp = arr->jobs[j].time_stamp;
            arr->jobs[0].status = FG;
            arr->jobs[j].full = false;
            arr->jobs[0].full = true;
            if (smallest_free_id > j) {
                smallest_free_id = j;
            }
            return 0;
        }
    }
    printf("move_job_to_fg failed\n")
    return 1;
}

void remove_job_from_fg(job_arr* arr){
    
    if(){ //job stopped because cntrl z
        arr->jobs[0].status = STOPPED;
        add_job(arr, 0, STOPPED);  //not complete
    }
    
    else if(pid == jobs[0].pid)							//fg reaped
		arr->jobs[0].full = false;
        arr->jobs[0].pid=getpid();
	return;
}


void delete_complex_job(job_arr* arr, pid_t complex_pid, int complex_i, int smallest_free_id) {

    if (complex_i < 1 || complex_i > MAX_ARGS) {
        printf("smash error: invalid job index\n");
        return;
    }

    if (arr->jobs[complex_i].pid != complex_pid || !arr->jobs[complex_i].full) {
        printf("smash error: job not found\n");
        return;
    }


    if (smallest_free_id > complex_i) {
        smallest_free_id = complex_i;
    }

    arr->jobs[complex_i].full = 0;
   // init_job(&arr->jobs[complex_i]);

    return;
}

int add_job(job_arr* arr, pid_t pid, char cur_status){
    
    int id = arr->smallest_free_id;

    if (id < 1 || id > MAX_ARGS) {
        printf("smash error: jobs list full\n");
        return -1;
    }

    if (cur_status == FG) {
    strcpy(arr->jobs[0].command, command);
    arr->jobs[0].pid = pid;
    arr->jobs[0].status = cur_status;
    arr->jobs[0].time_stamp = time(NULL);
    arr->jobs[0].full = 1;
    return 0;
    }

    else{
        arr->jobs[id].pid = pid;
        arr->jobs[id].status = cur_status;
        arr->jobs[id].time_stamp = time(NULL);
        strcpy(arr->jobs[id].command, command);
        arr->jobs[id].full = 1;
    }

    for (int i = id + 1; i <= MAX_ARGS; i++) {
        if (!arr->jobs[i].full) {
            arr->smallest_free_id = i;
            break;
        }
    }

    arr->smallest_free_id = MAX_ARGS + 1;

    return 0;
}




void delete_job(){
    for (int j = 1; j <= MAX_ARGS + 1; j++) {
        if (arr->jobs[j].full) {
            
            if (smallest_free_id > j) {
                smallest_free_id = j;
            }    
            arr->jobs[j].full = false;
            return ;
        }
    }

}
