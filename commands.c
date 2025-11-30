//commands.c
#include "commands.h"
#include <errno.h>
#include <string.h>
#include "my_system_call.h"
#include <sys/stat.h>
#include <fcntl.h>      // O_RDONLY
#include <unistd.h>     // sleep
#include <sys/wait.h>
#include <limits.h>     // PATH_MAX
#include "jobs.h"
#include <stdlib.h>   // malloc, free



// job table is actually defined in smash.c
extern job_arr job_list;

// global argv buffer for the current simple command
char *g_argv[ARGS_NUM_MAX + 1];

// 1 if the current command should run in background (ends with '&')
int g_is_bg = 0;




/* ================= ALIAS DATA STRUCTURE ================= */

typedef struct Alias {
    char name[CMD_LENGTH_MAX];   // alias name
    char value[CMD_LENGTH_MAX];  // expanded command line
    struct Alias *next;
} Alias;

static Alias *alias_head = NULL;

/* protect against infinite recursion: a->b, b->a, etc. */
#define MAX_ALIAS_EXPANSION_DEPTH 10
static int alias_expansion_depth = 0;

/* find alias node by name (internal helper) */
static Alias* find_alias_node(const char *name)
{
    for (Alias *cur = alias_head; cur != NULL; cur = cur->next) {
        if (strcmp(cur->name, name) == 0)
            return cur;
    }
    return NULL;
}

/* get alias value or NULL if not found */
static const char* find_alias_value(const char *name)
{
    Alias *node = find_alias_node(name);
    return node ? node->value : NULL;
}

/* create or update alias */
static void set_alias(const char *name, const char *value)
{
    Alias *node = find_alias_node(name);
    if (!node) {
        node = (Alias*)malloc(sizeof(Alias));
        if (!node) {
            fprintf(stderr, "smash error: alias: malloc failed\n");
            return;
        }
        strncpy(node->name, name, CMD_LENGTH_MAX - 1);
        node->name[CMD_LENGTH_MAX - 1] = '\0';
        node->next = alias_head;
        alias_head = node;
    }

    strncpy(node->value, value, CMD_LENGTH_MAX - 1);
    node->value[CMD_LENGTH_MAX - 1] = '\0';
}

/* remove alias by name, return 1 if removed, 0 if not found */
static int remove_alias(const char *name)
{
    Alias *prev = NULL;
    Alias *cur  = alias_head;

    while (cur) {
        if (strcmp(cur->name, name) == 0) {
            if (prev)
                prev->next = cur->next;
            else
                alias_head = cur->next;
            free(cur);
            return 1;
        }
        prev = cur;
        cur  = cur->next;
    }
    return 0;
}

/* ================= END OF ALIAS DATA STRUCTURE ================= */


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
		fprintf(stderr, "smash error: showpid: expected 0 arguments\n");
		return 1;
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
		return 1;
		}

	
		char path[CMD_LENGTH_MAX];
		if (getcwd(path, CMD_LENGTH_MAX) == NULL)
		{
			fprintf(stderr, "smash error: pwd: getcwd failed\n");
			return 1;
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
		return 1;
	}

	const char *path1 = args[1];
	const char *path2 = args[2];

	struct stat st1, st2;
	//Check that both paths exist
	if (stat(path1, &st1) != 0 || stat(path2, &st2) != 0) 
	{
		fprintf(stderr, "smash error: diff: expected valid paths for files\n");
		return 1;
	}

	//  Check that both are regular files
	if (!S_ISREG(st1.st_mode) || !S_ISREG(st2.st_mode))
	{
		fprintf(stderr, "smash error: diff: paths are not files\n");
		return 1;
	}

	int fd1 = (int)my_system_call(SYS_OPEN, path1, O_RDONLY, 0);
	if (fd1 == -1)
	{
		fprintf(stderr, "smash error: diff: expected valid paths for files\n");
		return 1;
	}

	int fd2 = (int)my_system_call(SYS_OPEN, path2, O_RDONLY, 0);
	if (fd2 == -1)
	{
		my_system_call(SYS_CLOSE, fd1);
		fprintf(stderr, "smash error: diff: expected valid paths for files\n");
		return 1;
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


static char old_pwd[PATH_MAX] = {0}; // Persistent state
static int old_pwd_set = 0; //  Flag for first use

int cd(char **args, int argc)
{
	if (argc != 1)
	{
		// argc = number of arguments AFTER "cd"
		// so if argc = 1, there is no path to change to
		fprintf(stderr, "smash error: cd: expected 1 arguments\n");
		return 1;
	}
	
	const char *target = args[1]; //
	
	 // 1) Handle "cd -" -> go back to previous directory
	if (strcmp(target, "-") == 0)
	{
		if (!old_pwd_set) {
            fprintf(stderr, "smash error: cd: old pwd not set\n");
            return 1;
        }
        target = old_pwd;  // go to previous directory
	}


// 2) Save current directory before changing
    char current_pwd[PATH_MAX];
    if (getcwd(current_pwd, sizeof(current_pwd)) == NULL) {
	// If this fails, something is very wrong – but just report and stop
	      perror("smash error: cd: getcwd failed");
	      return 1;
        }

// 3) Check that target exists and is a directory
    struct stat st;
    if (stat(target, &st) != 0) {
	      fprintf(stderr, "smash error: cd: target directory does not exist\n");
	      return 1;
        }

    if (!S_ISDIR(st.st_mode)) {
	     fprintf(stderr, "smash error: cd: %s: not a directory\n", target);
	     return 1;
        }

// 4) Actually change directory
    if (chdir(target) != 0) {
        perror("smash error: cd: chdir failed");
        return 1;
        }

// 5) Update old_pwd only AFTER successful cd
    strncpy(old_pwd, current_pwd, PATH_MAX - 1);
    old_pwd[PATH_MAX - 1] = '\0';
    old_pwd_set = 1;

    return 0;
}

// #########################################################################################

int jobs(char **args, int argc, job_arr *arr)
{
    if(argc != 0){
        fprintf(stderr, "smash error: jobs: expected 0 arguments\n");
        return 1;
    }

    update_jobs(arr);

    print_all_bg_jobs(arr);
    return 0;
}



//####################################################################################

int kill(char **args, int argc, job_arr *jobs)
{
	    //  Check argument count: must be exactly 2
		if (argc != 2) {
			fprintf(stderr, "smash error: kill: invalid arguments\n");
			return 1;
		}

		char *endptr1;
		long signum_long = strtol(args[1], &endptr1, 10);

		    // invalid if: no digits, extra chars, non-positive, or too big
			if (*args[1] == '\0' || *endptr1 != '\0' ||
				signum_long <= 0 || signum_long > INT_MAX) {
				fprintf(stderr, "smash error: kill: invalid arguments\n");
				return 1;
			}
			int signum = (int)signum_long;

		// Parse job ID (args[2])
		char *endptr2;
		long job_id_long = strtol(args[2], &endptr2, 10);
		
		if (*args[2] == '\0' || *endptr2 != '\0' ||
				job_id_long <= 0 || job_id_long > MAX_JOBS) {
				fprintf(stderr, "smash error: kill: invalid arguments\n");
				return 1;
			}
		int job_id = (int)job_id_long;

		if (job_id < 1 || job_id > MAX_JOBS || !jobs->jobs[job_id].full) {
			fprintf(stderr, "smash error: kill: job id %d does not exist\n", job_id);
			return 1;
		}

		// ---------- send the signal via wrapper ----------
		pid_t pid = jobs->jobs[job_id].pid;
		long ret = my_system_call(SYS_KILL, pid, signum);
		if (ret == -1) {
			// System-level error (very rare); assignment doesn't specify text,
			// so a generic perror is fine.
			perror("smash error: kill failed");
			return 1;
		}
        // 7. Print success message 
		printf("signal %d was sent to pid %d\n", signum, (int)pid);
		return 0;
		
			
}


//###########################################################################
int fg(char **args, int argc, job_arr *jobs)
{
    int job_id = -1;   // will hold the chosen job index (1..MAX_JOBS)

    /* ---------- argument parsing ---------- */

    if (argc > 1) {
        // too many args
        fprintf(stderr, "smash error: fg: invalid arguments\n");
        return 1;
    }

    if (argc == 1) {
        // parse job id from args[1], command looks like: fg <num>
        char *endptr;
        long id_long = strtol(args[1], &endptr, 10);

        // must be: pure number, >0, within range
        if (*args[1] == '\0' || *endptr != '\0' ||
            id_long <= 0 || id_long > MAX_JOBS) {
            fprintf(stderr, "smash error: fg: invalid arguments\n");
            return 1;
        }

        job_id = (int)id_long;

        // check that this job id actually exists
        if (!jobs->jobs[job_id].full) {
            fprintf(stderr, "smash error: fg: job id %d does not exist\n", job_id);
            return 1;
        }

    } else {
        /* argc == 0: no job id given → use job with highest id */

        if (jobs->job_counter == 0) {
            fprintf(stderr, "smash error: fg: jobs list is empty\n");
            return 1;
        }

        // find highest existing job id
        for (int j = MAX_JOBS; j >= 1; --j) {
            if (jobs->jobs[j].full) {
                job_id = j;
                break;
            }
        }

        // very defensive: in case job_counter is out of sync
        if (job_id == -1) {
            fprintf(stderr, "smash error: fg: jobs list is empty\n");
            return 1;
        }
    }

    /* ---------- at this point: job_id refers to a valid job ---------- */

    job  *j   = &jobs->jobs[job_id];
    pid_t pid = j->pid;

    // print job info (adjust format to what the assignment wants, if needed)
    printf("[%d] %s\n", job_id, j->command);

    // if job is stopped, resume it with SIGCONT
    if (j->status == STOPPED) {
        long r = my_system_call(SYS_KILL, pid, SIGCONT);
        if (r == -1) {
            perror("smash error: fg: SIGCONT failed");
            return 1;
        }
    }

    // mark as foreground (in your status logic)
    j->status = FG;

    // ---------- wait for job to finish or stop again ----------
    int  status = 0;
    long w      = my_system_call(SYS_WAITPID, pid, &status, WUNTRACED);

    if (w == -1) {
        perror("smash error: fg: waitpid failed");
        delete_job(jobs, job_id);
        return 1;
    }

    if (WIFSTOPPED(status)) {
        // job was stopped again (Ctrl+Z / SIGSTOP) → keep it in job list as STOPPED
        j->status = STOPPED;
        printf("\n");
        return 0;
    } else {
        // job finished (normal exit or killed) → remove from jobs list
        delete_job(jobs, job_id);
        return 0;
    }
}



// ##########################################

int bg(char **args, int argc, job_arr *jobs)
{
    int job_id = -1;

    /* ---------- argument parsing ---------- */

    if (argc > 1) {
        fprintf(stderr, "smash error: bg: invalid arguments\n");
        return 1;
    }

    if (argc == 1) {
        // parse job id from args[1]
        char *endptr;
        long id_long = strtol(args[1], &endptr, 10);

        if (*args[1] == '\0' || *endptr != '\0' ||
            id_long <= 0 || id_long > MAX_JOBS) {
            fprintf(stderr, "smash error: bg: invalid arguments\n");
            return 1;
        }

        job_id = (int)id_long;

        // ensure this job exists
        if (!jobs->jobs[job_id].full) {
            fprintf(stderr, "smash error: bg: job id %d does not exist\n", job_id);
            return 1;
        }

    } else {
        // argc == 0: No argument given → pick highest STOPPED job

        if (jobs->job_counter == 0) {
            fprintf(stderr, "smash error: bg: there is no stopped job to resume\n");
            return 1;
        }

        int found = 0;
        for (int j = MAX_JOBS; j >= 1; --j) {
            if (jobs->jobs[j].full && jobs->jobs[j].status == STOPPED) {
                job_id = j;
                found  = 1;
                break;
            }
        }

        if (!found) {
            fprintf(stderr, "smash error: bg: there is no stopped job to resume\n");
            return 1;
        }
    }

    /* ---------- at this point job_id is valid ---------- */

    job *j = &jobs->jobs[job_id];

    // must be STOPPED to resume with bg
    if (j->status != STOPPED) {
        fprintf(stderr,
                "smash error: bg: job id %d is already running in the background\n",
                job_id);
        return 1;
    }

    // Send SIGCONT to resume the job
    long ret = my_system_call(SYS_KILL, j->pid, SIGCONT);
    if (ret == -1) {
        perror("smash error: bg: SIGCONT failed");
        return 1;
    }

    // update status to background
    j->status = BG;

    // print info
    printf("%s : %d\n", j->command, j->pid);

    return 0;
}


//####################################

int quit(char **args, int argc, job_arr *jobs)
{
	if (argc > 1) {
        fprintf(stderr, "smash error: quit: expected 0 or 1 arguments\n");
        return 1;
    }

	if (argc == 1 && strcmp(args[1], "kill") != 0) {
        fprintf(stderr, "smash error: quit: unexpected arguments\n");
        return 1;
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
            break;

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

    /* ---------- alias & unalias builtins (not alias-expanded) ---------- */
    if (strcmp(cmd, "alias") == 0) {
        return alias_cmd(g_argv, numArgs - 1, original_line);
    }

    if (strcmp(cmd, "unalias") == 0) {
        return unalias_cmd(g_argv, numArgs - 1);
    }

    /* ---------- alias EXPANSION for other commands ---------- */
    const char *expansion = find_alias_value(cmd);
    if (expansion != NULL) {
        if (alias_expansion_depth >= MAX_ALIAS_EXPANSION_DEPTH) {
            fprintf(stderr, "smash error: alias: expansion too deep (possible recursion)\n");
            return 1;
        }

        alias_expansion_depth++;

        char buf[CMD_LENGTH_MAX];
        strncpy(buf, expansion, CMD_LENGTH_MAX - 1);
        buf[CMD_LENGTH_MAX - 1] = '\0';

        int ret;
        if (strstr(buf, "&&") != NULL) {
            // expansion itself may contain '&&'
            ret = handle_compound_commands(buf);
        } else {
            char tmp[CMD_LENGTH_MAX];
            strcpy(tmp, buf);
            int argc2 = parseCommand(tmp);
            ret = command_Manager(argc2, buf);
        }

        alias_expansion_depth--;
        return ret;
    }


/* ---------- built-ins commands --------------------------------------- */
    
    if (strcmp(cmd, "showpid") == 0) {
        // argc here = number of arguments *after* the command name
        return showpid(g_argv, numArgs - 1);

    } else if (strcmp(cmd, "pwd") == 0) {
        return pwd(g_argv, numArgs - 1);

    } else if (strcmp(cmd, "cd") == 0) {
        return cd(g_argv, numArgs - 1);

    } else if (strcmp(cmd, "diff") == 0) {
        return cmd_diff(g_argv, numArgs - 1);

    } else if (strcmp(cmd, "jobs") == 0) {
        return jobs(g_argv, numArgs - 1, &job_list);

    } else if (strcmp(cmd, "kill") == 0) {
        return kill(g_argv, numArgs - 1, &job_list);

    } else if (strcmp(cmd, "fg") == 0) {
        return fg(g_argv, numArgs - 1, &job_list);

    } else if (strcmp(cmd, "bg") == 0) {
        return bg(g_argv, numArgs - 1, &job_list);

    } else if (strcmp(cmd, "quit") == 0) {
        return quit(g_argv, numArgs - 1, &job_list);  // may _exit(0) inside
    }

    // not a built-in -> external command
    return run_external_command(g_argv, original_line);
}




//###################################################################

int run_external_command(char **argv, const char *original_line)
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

        long rv = my_system_call(SYS_EXECVP, argv[0], argv);

        // If we got here, exec failed.
        if (rv == -1) {
            if (errno == ENOENT) {
                fprintf(stderr, "smash error: external: cannot find program\n");
            } else {
                fprintf(stderr, "smash error: external: invalid command\n");
            }
        } else {
            fprintf(stderr, "smash error: external: invalid command\n");
        }

        _exit(1);  // Child must exit on failure
    }

    /* ================= PARENT PROCESS (smash) ================= */

    if (g_is_bg) {
        // ---------- background command ----------
        int job_id = add_job(&job_list, pid, original_line, BG);
        if (job_id == -1) {
            fprintf(stderr, "smash error: jobs list is full\n");
            // process still runs, we just don't track it
        }
        return 0;  // bg command itself "succeeds"
    }

    /* ---------- foreground command ---------- */

    // Put FG job into slot 0
    int job_id = add_job(&job_list, pid, original_line, FG);
    int status = 0;

    long wr = my_system_call(SYS_WAITPID, pid, &status, WUNTRACED);
    if (wr == -1) {
        perror("smash error: waitpid failed");
        clear_fg_job(&job_list);   // make sure fg slot is not left dirty
        return 1;
    }

    // If the process was stopped (Ctrl+Z), move it into jobs[1..] as STOPPED
    if (WIFSTOPPED(status)) {
        job *fgj = &job_list.jobs[0];

        // Try to add it as a STOPPED BG job with a real job id
        if (fgj->full) {
            if (add_job(&job_list, fgj->pid, fgj->command, STOPPED) == -1) {
                fprintf(stderr, "smash error: jobs list is full\n");
                // If this fails, we at least clear the fg slot; process is still stopped in OS
            }
        }

        clear_fg_job(&job_list);
        return 1;   // did not complete successfully → fail for &&
    }

    // Process finished (normal exit or killed)
    int exit_code = 1;   // default: failure

    if (WIFEXITED(status)) {
        exit_code = WEXITSTATUS(status);   // program's return value
    } else if (WIFSIGNALED(status)) {
        exit_code = 1;                     // killed by signal → treat as failure
    }

    clear_fg_job(&job_list);

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
    char buffer[CMD_LENGTH_MAX + 1];
    strncpy(buffer, line, CMD_LENGTH_MAX);
    buffer[CMD_LENGTH_MAX] = '\0';

    char *nextPart = buffer;  // part we are currently processing
    int final_status = 0;     // result of last executed command

    while (1) {
        // Look for "&&" in the remaining string
        char *andPos = strstr(nextPart, "&&");

        // This will hold the current simple command (trimmed)
        char *cmd_segment = NULL;

        if (!andPos) {
            // No more "&&" -> nextPart is the final command
            cmd_segment = trim_spaces(nextPart);

            // If empty after trimming, nothing to do
            if (cmd_segment[0] == '\0') {
                return final_status;  // whatever happened before
            }

            // Make a copy for parseCommand (because it uses strtok and modifies string)
            char cmd_for_parse[CMD_LENGTH_MAX + 1];
            strncpy(cmd_for_parse, cmd_segment, CMD_LENGTH_MAX);
            cmd_for_parse[CMD_LENGTH_MAX] = '\0';

            int argc = parseCommand(cmd_for_parse);
            if (argc == 0) {
                return final_status;
            }

            final_status = command_Manager(argc, cmd_segment);
            return final_status;
        }

        // Cut the buffer at "&&"
        *andPos = '\0';

        // Left part is the next command
        cmd_segment = trim_spaces(nextPart);

        // Skip empty left segment
        if (cmd_segment[0] != '\0') {
            // Make a copy for parsing
            char cmd_for_parse[CMD_LENGTH_MAX + 1];
            strncpy(cmd_for_parse, cmd_segment, CMD_LENGTH_MAX);
            cmd_for_parse[CMD_LENGTH_MAX] = '\0';

            int argc = parseCommand(cmd_for_parse);
            if (argc > 0) {
                int status = command_Manager(argc, cmd_segment);
                if (status != 0) {
                    // This command failed -> stop executing further
                    return status;
                }
                final_status = status;
            }
        }

        // Continue with the right side, after the "&&"
        nextPart = andPos + 2;
    }
}

//#########################################################################################

/* alias: alias name="some commands" */
int alias_cmd(char **args, int argc, const char *original_line)
{
    (void)args;  // we ignore tokenized args; we re-parse from original_line

    // must have at least a name after "alias"
    if (argc < 1) {
        fprintf(stderr, "smash error: alias: invalid arguments\n");
        return 1;
    }

    const char *p = original_line;

    // skip leading spaces
    while (*p == ' ' || *p == '\t') p++;

    // must start with "alias"
    if (strncmp(p, "alias", 5) != 0) {
        fprintf(stderr, "smash error: alias: invalid arguments\n");
        return 1;
    }
    p += 5;

    // skip spaces after 'alias'
    while (*p == ' ' || *p == '\t') p++;

    // parse alias name: up to '=' or whitespace
    char name_buf[CMD_LENGTH_MAX];
    int ni = 0;
    while (*p && *p != '=' && *p != ' ' && *p != '\t' && ni < CMD_LENGTH_MAX - 1) {
        name_buf[ni++] = *p++;
    }
    name_buf[ni] = '\0';

    if (ni == 0) {
        fprintf(stderr, "smash error: alias: invalid name\n");
        return 1;
    }

    // skip spaces before '='
    while (*p == ' ' || *p == '\t') p++;

    if (*p != '=') {
        fprintf(stderr, "smash error: alias: invalid arguments\n");
        return 1;
    }
    p++; // skip '='

    // skip spaces before value
    while (*p == ' ' || *p == '\t') p++;

    // parse value
    char value_buf[CMD_LENGTH_MAX];

    if (*p == '"') {
        // quoted value: alias x="......"
        p++;  // skip opening quote
        int vi = 0;
        while (*p && *p != '"' && vi < CMD_LENGTH_MAX - 1) {
            value_buf[vi++] = *p++;
        }
        value_buf[vi] = '\0';
        // if closing quote is missing, we just stop at end-of-line
    } else {
        // unquoted: copy rest of line and trim spaces using your existing trim_spaces
        char tmp[CMD_LENGTH_MAX];
        strncpy(tmp, p, CMD_LENGTH_MAX - 1);
        tmp[CMD_LENGTH_MAX - 1] = '\0';

        char *trimmed = trim_spaces(tmp);   // your existing function
        strncpy(value_buf, trimmed, CMD_LENGTH_MAX - 1);
        value_buf[CMD_LENGTH_MAX - 1] = '\0';
    }

    // store or update alias
    set_alias(name_buf, value_buf);
    return 0;
}


//###############################################################################

int unalias_cmd(char **args, int argc)
{
    if (argc != 1) {
        fprintf(stderr, "smash error: unalias: invalid arguments\n");
        return 1;
    }

    const char *name = args[1];
    // removing a non-existing alias is just a no-op
    (void)remove_alias(name);
    return 0;
}




