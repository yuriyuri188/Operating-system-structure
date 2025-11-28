//commands.c
#include "commands.h"
#include <errno.h>
#include <string.h>
#include "my_system_call.h"
#include <sys/stat.h>
#include <unistd.h>     // sleep
#include <sys/wait.h>


extern job_arr job_list;



//example function for printing errors from internal commands
void perrorSmash(const char* cmd, const char* msg)
{
    fprintf(stderr, "smash error:%s%s%s\n",
        cmd ? cmd : "",
        cmd ? ": " : "",
        msg);
}

//example function for parsing commands
int parseCmdExample(char* line)
{
	char* delimiters = " \t\n"; //parsing should be done by spaces, tabs or newlines
	char* cmd = strtok(line, delimiters); //read strtok documentation - parses string by delimiters
	if(!cmd)
		return INVALID_COMMAND; //this means no tokens were found, most like since command is invalid
	
	char* args[MAX_ARGS];
	int nargs = 0;
	args[0] = cmd; //first token before spaces/tabs/newlines should be command name
	for(int i = 1; i < MAX_ARGS; i++)
	{
		args[i] = strtok(NULL, delimiters); //first arg NULL -> keep tokenizing from previous call
		if(!args[i])
			break;
		nargs++;
	}
	/*
	At this point cmd contains the command string and the args array contains
	the arguments. You can return them via struct/class, for example in C:
		typedef struct {
			char* cmd;
			char* args[MAX_ARGS];
		} Command;
	Or maybe something more like this:
		typedef struct {
			bool bg;
			char** args;
			int nargs;
		} CmdArgs;
	*/
}



//#################################################
int showpid(char **args, int argc)
{
    if (argc != 0)
	{
		fprintf(stderr, "smash error: pwd: expected 0 arguments\n");
		return -1;
		}
	
		printf("smash pid is %d\n", getpid());
		return 0;
	    
}

//###########################################################
int pwd(char **args, int argc)
{
	if (argc != 0)
	{
		fprintf(stderr, "smash error: pwd: expected 0 arguments\n");
		return -1;
		}

	
		char path[CMD_LENGTH_MAX];
		if (getcwd(path, CMD_LENGTH_MAX) == NULL)
		{
			fprintf(stderr, "smash error: pwd: getcwd failed\n");
			return -1;
		}

		printf("%s\n", path);
		return 0;
}


// #########################################################################################
int cmd_diff(char **args, int argc)
{
	if (argc != 2)
	{
		fprintf(stderr, "smash error: diff: expected 2 arguments\n");
		return -1;
	}

	const char *path1 = args[1];
	const char *path2 = args[2];

	struct stat st1, st2;
	//Check that both paths exist
	if (stat(path1, &st1) != 0 || stat(path2, &st2) != 0) 
	{
		fprintf(stderr, "smash error: diff: expected valid paths for files\n");
		return -1;
	}

	//  Check that both are regular files
	if (!S_ISREG(st1.st_mode) || !S_ISREG(st2.st_mode))
	{
		fprintf(stderr, "smash error: diff: paths are not files\n");
		return -1;
	}

	int fd1 = (int)my_system_call(SYS_OPEN, path1, O_RDONLY, 0);
	if (fd1 == -1)
	{
		fprintf(stderr, "smash error: diff: expected valid paths for files\n");
		return -1;
	}

	int fd2 = (int)my_system_call(SYS_OPEN, path2, O_RDONLY, 0);
	if (fd2 == -1)
	{
		my_system_call(SYS_CLOSE, fd1);
		fprintf(stderr, "smash error: diff: expected valid paths for files\n");
		return -1;
    }

	unsigned char buf1[BUF_SIZE], buf2[BUF_SIZE];
	int diff_count = 0;
	while (1)
	{
		long r1  = my_system_call(SYS_READ, fd1, buf1, sizeof(buf1));
		long r2  = my_system_call(SYS_READ, fd2, buf2, sizeof(buf2));

		if (r1 < 0 || r2 < 0)
		{
			// real read error
			diff_count = 1;
			break;
		}

		if (r1 == 0 && r2 == 0)
		{
		 // both EOF which means equal so far 
			break;
		}

		if (r1 != r2)
		{
			 // different lengths
			diff_count = 1;
			break;
		}

		if (memcmp(buf1, buf2, r1) != 0) 
		{ 
		// same length, different bytes
			diff_count = 1;
			break;
		}
	}

	

	my_system_call(SYS_CLOSE, fd1);
	my_system_call(SYS_CLOSE, fd2);


//  Print result: 0 = same, 1 = different
	printf("diff: %d\n", diff_count);
	return 0;
}

// #########################################################################################


static char old_pwd[CMD_LENGTH_MAX] = {0}; // Persistent state
static int old_pwd_set = 0; //  Flag for first use

int cd(char **args, int argc)
{
	if (argc != 1)
	{
		// argc = number of arguments AFTER "cd"
		// so if argc = 1, there is no path to change to
		fprintf(stderr, "smash error: cd: expected 1 arguments\n");
		return -1;
	}
	
	const char *target = args[1]; //
	
	 // 1) Handle "cd -" -> go back to previous directory
	if (strcmp(target, "-") == 0)
	{
		if (!old_pwd_set) {
            fprintf(stderr, "smash error: cd: old pwd not set\n");
            return -1;
        }
        target = old_pwd;  // go to previous directory
	}


// 2) Save current directory before changing
    char current_pwd[CMD_LENGTH_MAX];
    if (getcwd(current_pwd, sizeof(current_pwd)) == NULL) {
	// If this fails, something is very wrong – but just report and stop
	      perror("smash error:: getcwd failed");
	      return -1;
        }

// 3) Check that target exists and is a directory
    struct stat st;
    if (stat(target, &st) != 0) {
	      fprintf(stderr, "smash error: cd: target directory does not exist\n");
	      return -1;
        }

    if (!S_ISDIR(st.st_mode)) {
	     fprintf(stderr, "smash error: cd: %s: not a directory\n", target);
	     return -1;
        }

// 4) Actually change directory
    if (chdir(target) != 0) {
        perror("smash error:: chdir failed");
        return -1;
        }

// 5) Update old_pwd only AFTER successful cd
    strncpy(old_pwd, current_pwd, CMD_LENGTH_MAX - 1);
    old_pwd[CMD_LENGTH_MAX - 1] = '\0';
    old_pwd_set = 1;

    return 0;
}

// #########################################################################################

int jobs(char **args, int argc, job_arr *arr)
{
    if(argc != 0){
        fprintf(stderr, "smash error: jobs: expected 0 arguments\n");
        return -1;
    }

    print_all_jobs(arr);
    return 0;
}

int kill(char **args, int argc, job_arr *jobs)
{
	    // 1. Check argument count: must be exactly 2
		if (argc != 2) {
			fprintf(stderr, "smash error: kill: invalid arguments\n");
			return -1;
		}

		char *endptr1;
		long signum_long = strtol(args[1], &endptr1, 10);

		    // invalid if: no digits, extra chars, non-positive, or too big
			if (*args[1] == '\0' || *endptr1 != '\0' ||
				signum_long <= 0 || signum_long > INT_MAX) {
				fprintf(stderr, "smash error: kill: invalid arguments\n");
				return -1;
			}
			int signum = (int)signum_long;

		// Parse job ID (args[2])
		char *endptr2;
		long job_id_long = strtol(args[2], &endptr2, 10);
		
		if (*args[2] == '\0' || *endptr2 != '\0' ||
				job_id_long <= 0 || job_id_long > MAX_JOBS) {
				fprintf(stderr, "smash error: kill: invalid arguments\n");
				return -1;
			}
		int job_id = (int)job_id_long;

		int index = find_by_job_id(jobs, job_id);   //  Find the job by job_id
        if (index == -1) {
                fprintf(stderr, "smash error: kill: job id %d does not exist\n", job_id);
                return -1;
             }

		// ---------- send the signal via wrapper ----------
		pid_t pid = jobs->jobs[index].pid;
		long ret = my_system_call(SYS_KILL, pid, signum);
		if (ret == -1) {
			// System-level error (very rare); assignment doesn’t specify text,
			// so a generic perror is fine.
			perror("smash error: kill failed");
			return -1;
		}
        // 7. Print success message 
		printf("signal %d was sent to pid %d\n", signum, (int)pid);
		return 0;
		
			
}


//###########################################################################
int fg(char **args, int argc, job_arr *jobs)
{
	int job_id = -1;
    int index = -1;

    /* ---------- argument parsing ---------- */

    if (argc > 1) {
        // too many args
        fprintf(stderr, "smash error: fg: invalid arguments\n");
        return -1;
    }

    if (argc == 1) {
        // parse job id from args[1] since the input ;ooks like fg <num>
        char *endptr;
        long id_long = strtol(args[1], &endptr, 10); // strtol() Converts a string to a long integer

        if (*args[1] == '\0' || *endptr != '\0' ||
            id_long <= 0 || id_long > MAX_JOBS) {
            fprintf(stderr, "smash error: fg: invalid arguments\n"); //We want only a positive number, no extra junk
            return -1;
        }

        job_id = (int)id_long;
        index = find_by_job_id(jobs, job_id);
        if (index == -1) {
            fprintf(stderr, "smash error: fg: job id %d does not exist\n", job_id);
            return -1;
        }
    } else { 
        /* argc == 0: no job id given → use job with highest id */
        if (jobs->job_counter == 0) {
            fprintf(stderr, "smash error: fg: jobs list is empty\n");
            return -1;
        }

        // find highest existing job id
        for (int j = MAX_JOBS; j >= 1; --j) {
            if (jobs->jobs[j].full) {
                job_id = j;
                index  = j;
                break;
            }
        }
    }

    /* at this point: job_id and index refer to a valid job */

    job *j = &jobs->jobs[index];
    pid_t pid = j->pid;

    // ---------- print job header, like "[1] /bin/sleep 30 &" ----------
    printf("[%d] %s\n", job_id, j->command);

    // ---------- if job is stopped, resume it with SIGCONT ----------
    if (j->status == STOPPED) {
        long r = my_system_call(SYS_KILL, pid, SIGCONT);
        if (r == -1) {
            perror("smash error: fg: SIGCONT failed");
            return -1;
        }
    }

    // mark as foreground job
    j->status = FG;

    // ---------- wait for job to finish ----------
    int status = 0;
	long w = my_system_call(SYS_WAITPID, pid, &status, WUNTRACED); //Also return if the process STOPPED (not only if it finished).

	if (w == -1) {
		perror("smash error: fg: waitpid failed");
		delete_job(jobs, job_id);
		return -1;
	}
	/* ========== CHECK HOW PROCESS STOPPED ========== */
	if (WIFSTOPPED(status)) {
		//  Job was stopped (Ctrl+Z or SIGSTOP) → KEEP in list as STOPPED
		j->status = STOPPED;
		printf("\n");  // Clean newline
		return 0;      // Don't delete!
	} else {
		// Job finished (normally or killed) → REMOVE from list
		delete_job(jobs, job_id);
		return 0;
}

}


// ##########################################

int bg(char **args, int argc, job_arr *jobs)
{
    int job_id = -1;
    int index = -1;

    if (argc > 1) {
        // Too many arguments
        fprintf(stderr, "smash error: bg: invalid arguments\n");
        return -1;
    }

    if (argc == 1) {
        //  Parse job_id from args[1]
        char *endptr;
        long job_id_long = strtol(args[1], &endptr, 10); 

        if (*args[1] == '\0' || *endptr != '\0' ||
            job_id_long <= 0 || job_id_long > MAX_JOBS) {
            fprintf(stderr, "smash error: bg: invalid arguments\n");
            return -1;
        }

        job_id = (int)job_id_long;
        
        // Find the job
        index = find_by_job_id(jobs, job_id);
        if (index == -1) {
            fprintf(stderr, "smash error: bg: job id %d does not exist\n", job_id);
            return -1;
        }
    } else {
        // argc == 0: No argument given so use job with highest ID
        
        if (jobs->job_counter == 0) {
            fprintf(stderr, "smash error: bg: there is no stopped job to resume\n");
            return -1;
        }

        // Find the highest job_id that is STOPPED
        int found = 0;
        for (int j = MAX_JOBS; j >= 1; --j) {
            if (jobs->jobs[j].full && jobs->jobs[j].status == STOPPED) {
                job_id = j;
                index = j;
                found = 1;
                break;
            }
        }

        if (!found) {
            fprintf(stderr, "smash error: bg: there is no stopped job to resume\n");
            return -1;
        }
    }

    // At this point job_id and index refer to a valid job 

    job *j = &jobs->jobs[index];

    //  if argc==1, we need to check if stopped or not
    
	if (j->status != STOPPED) {
		//  the job is already running
		fprintf(stderr, "smash error: bg: job id %d is already running in the background\n", job_id);
		return -1;
	}

    // SEND SIGCONT 
    long ret = my_system_call(SYS_KILL, j->pid, SIGCONT);
    if (ret == -1) {
        perror("smash error: bg: SIGCONT failed");
        return -1;
    }

    //  UPDATE STATUS 
    j->status = BG;

    
    printf("%s : %d\n", j->command, j->pid);
    
    return 0;
}

//####################################

int quit(char **args, int argc, job_arr *jobs)
{
	if (argc > 1) {
        fprintf(stderr, "smash error: quit: expected 0 or 1 arguments\n");
        return -1;
    }

	if (argc == 1 && strcmp(args[1], "kill") != 0) {
        fprintf(stderr, "smash error: quit: unexpected arguments\n");
        return -1;
    }

	 /* ---------- case: plain 'quit' ---------- */
	if (argc == 0) {
			_exit(0);   // terminate smash immediately
	}

	/* ---------- case: 'quit kill' ---------- */
	// scan all job slots marks whether this slot is active only active jobs are processed.
	for (int id = 1; id <= MAX_JOBS; ++id) {
        if (!jobs->jobs[id].full)
            continue;

        job  *j   = &jobs->jobs[id];
        pid_t pid = j->pid;

        int status = 0;
        long rv;

        /* skip jobs that already finished */
        rv = my_system_call(SYS_WAITPID, pid, &status, WNOHANG);
        if (rv == -1) {
            // child is probably gone remove from table and skip
            delete_job(jobs, id);
            continue;
        }
        if (rv > 0) {
            // already terminated Remove it from jobs list
            delete_job(jobs, id);
            continue;
        }

        /* 1) header: "[id] command - "  (spaces around '-') */
        printf("[%d] %s - ", id, j->command);

        /* 2) send SIGTERM and print message */
        printf("sending SIGTERM... ");
        fflush(stdout);

        long kr = my_system_call(SYS_KILL, pid, SIGTERM);
        if (kr == -1) {
            perror("smash error: quit: SIGTERM failed");
            printf("done\n");
            delete_job(jobs, id);
            continue;
        }

        /* 3) wait up to 5 seconds after SIGTERM */
        sleep(5);

        rv = my_system_call(SYS_WAITPID, pid, &status, WNOHANG);
        if (rv == -1) {
            perror("smash error: quit: waitpid failed");
            printf("done\n");
            delete_job(jobs, id);
            continue;
        }

        if (rv > 0) {
            /* process exited within those 5 seconds */
            printf("done\n");
            delete_job(jobs, id);
            continue;
        }

        /* 4) still alive after 5 seconds → SIGKILL */
        printf("sending SIGKILL... ");
        fflush(stdout);

        kr = my_system_call(SYS_KILL, pid, SIGKILL);
        if (kr == -1) {
            perror("smash error: quit: SIGKILL failed");
            printf("done\n");
        } else {
            // reap it (blocking is fine, we're quitting anyway)
            my_system_call(SYS_WAITPID, pid, &status, 0);
            printf("done\n");
        }

        delete_job(jobs, id);
    }

    _exit(0);   // after handling all jobs, terminate smash
}

//########################################p


// global argv-like array that parseCommand fills:
char *g_argv[ARGS_NUM_MAX + 1];
int g_is_bg = 0;  // 0 = foreground, 1 = background

int parseCommand(char *cmd)
{
    int argc = 0;
    g_is_bg = 0;   // reset background flag for this command
    char *tok = strtok(cmd, " \t\n");  // handles any number of spaces & tabs

    while (tok && argc < ARGS_NUM_MAX) {
        // If token is "&", mark as background and DO NOT add it to argv
        if (strcmp(tok, "&") == 0) {
            g_is_bg = 1;
            // We *don't* put "&" in g_argv[]
            // According to the assignment, "&" appears only at the end,
            // so we can just keep parsing in case there is weird input,
            // but normally there should be no more tokens.
        } else {
            g_argv[argc++] = tok;
        }

        tok = strtok(NULL, " \t\n");
    }

    g_argv[argc] = NULL;  // terminate argv-style array

    return argc;          // number of *real* args (without "&")
}

//###############################################pragma endregion

int command_Manager(int numArgs, char *original_line)
{
    if (numArgs == 0)
        return 0;  // nothing to do = "success"

    char *cmd = g_argv[0];

    // built-ins
    if (strcmp(cmd, "showpid") == 0) {
        return showpid(g_argv, numArgs - 1);

    } else if (strcmp(cmd, "pwd") == 0) {
        return pwd(g_argv, numArgs - 1);

    } else if (strcmp(cmd, "cd") == 0) {
        return cd(g_argv, numArgs - 1);

    } else if (strcmp(cmd, "diff") == 0) {
        return diff_cmd(g_argv, numArgs - 1);

    } else if (strcmp(cmd, "jobs") == 0) {
        return jobs_cmd(g_argv, numArgs - 1, &job_list);

    } else if (strcmp(cmd, "kill") == 0) {
        return kill_cmd(g_argv, numArgs - 1, &job_list);

    } else if (strcmp(cmd, "fg") == 0) {
        return fg_cmd(g_argv, numArgs - 1, &job_list);

    } else if (strcmp(cmd, "bg") == 0) {
        return bg_cmd(g_argv, numArgs - 1, &job_list);

    } else if (strcmp(cmd, "quit") == 0) {
        return quit_cmd(g_argv, numArgs - 1, &job_list);  // may _exit(0) inside
    }

    // external command
    return run_external_command(g_argv, original_line);
}



//###################################################################

int run_external_command(char **argv,  const char *original_line)
{
    

    // ---------- fork a child process ----------
    pid_t pid = (pid_t)my_system_call(SYS_FORK);
    if (pid < 0) {
        perror("smash error: fork failed");
        return 1; // failure
    }

    /* ================= CHILD PROCESS ================= */
    if (pid == 0) {
        // Put the child in a new process group (required for job control)
        setpgrp();

        // Try to exec the external program.
        // If execvp succeeds, it never returns.
        long rv = my_system_call(SYS_EXECVP, argv[0], argv);

        // If we got here, exec failed.
        // Check errno to decide which error message to print.
        if (rv == -1) {
            if (errno == ENOENT) {
                // Program file not found
                fprintf(stderr, "smash error: external: cannot find program\n");
            } else {
                // Some other exec error
                fprintf(stderr, "smash error: external: invalid command\n");
            }
        } else {
            // Very defensive, normally exec failure should set rv == -1
            fprintf(stderr, "smash error: external: invalid command\n");
        }

        _exit(1);  // Child must exit on failure
    }

    /* ================= PARENT PROCESS (smash) ================= */

    if (g_is_bg) {
        /* ---------- background command ----------
         * Do NOT wait for it.
         * Just add to jobs list with status BG.
         */
        int job_id = add_job(&job_list, pid, original_line, BG);
        if (job_id == -1) {
            fprintf(stderr, "smash error: jobs list is full\n");
            // We don't kill the process, just can't track it as a job.
        }
        // Return immediately to main loop (next prompt).
        return 0;
    } 
     /* ---------- foreground command ---------- */
     int job_id = add_job(&job_list, pid, original_line, FG);
     int status = 0;
 
     long wr = my_system_call(SYS_WAITPID, pid, &status, WUNTRACED); //Waits until the child exits or is stopped
     if (wr == -1) {
         perror("smash error: waitpid failed");
         if (job_id != -1)
             delete_job(&job_list, job_id);
         return 1;
     }
 
     // If the process was stopped (Ctrl+Z), keep it as STOPPED job
     // and treat as "not successful" for &&.
     if (WIFSTOPPED(status)) {
         if (job_id != -1)
             job_list.jobs[job_id].status = STOPPED;
         return 1;   // it did not "complete successfully"
     }
 
     // Process finished (normal or killed)
     int exit_code = 1;   // default: failure
 
     if (WIFEXITED(status)) {
         exit_code = WEXITSTATUS(status);   // program's real return value
     } else if (WIFSIGNALED(status)) {
         exit_code = 1;   // killed by signal -> treat as failure
     }
 
     if (job_id != -1)
         delete_job(&job_list, job_id);
 
     // success if child returned 0, otherwise failure
     return (exit_code == 0) ? 0 : 1;
 }

//###################################

// Remove leading and trailing spaces/tabs in-place.
// Returns pointer to the first non-space character.
static char* trim_spaces(char *s)
{
    while (*s == ' ' || *s == '\t')
        s++;

    char *end = s + strlen(s);
    while (end > s && (end[-1] == ' ' || end[-1] == '\t'))
        end--;

    *end = '\0';
    return s;
}

//#############################################################

int handle_compound_commands(char *line)
{
    char buffer[CMD_LENGTH_MAX];
    strcpy(buffer, line); //copy the line

    char *nextPart = buffer; //points to the part of the string we are currently processing.
    int final_status = 0;  //will remember the result of the last executed command.

    while (1) {
        // Look for &&
        char *andPos = strstr(nextPart, "&&");

        if (!andPos) {
            // There is no more &&
            // so nextPart is the final command
            char *cmd = trim_spaces(nextPart);

            // Execute the command
            int argc = parseCommand(cmd);
            final_status = command_Manager(argc, cmd);

            // return the last status (success/failure)
            return final_status;
        }

        // Cut the line at &&
        *andPos = '\0';

        // Get the left command
        char *cmd = trim_spaces(nextPart);

        // Execute it
        int argc = parseCommand(cmd);
        int status = command_Manager(argc, cmd);

        if (status != 0) {
            // it failed → do NOT execute the rest
            return status;
        }

        // Continue with the right side after &&
        nextPart = andPos + 2;
    }
}





