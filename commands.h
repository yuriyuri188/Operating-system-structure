#ifndef COMMANDS_H
#define COMMANDS_H
/*=============================================================================
* includes, defines, usings
=============================================================================*/
#include <stdlib.h>
#include <stdio.h>
#include "jobs.h"



#define CMD_LENGTH_MAX 80
#define ARGS_NUM_MAX 20
#define JOBS_NUM_MAX 100
#define MAX_JOBS 100
#define BUF_SIZE 4096
#define PATH_MAX 4096


extern char *g_argv[ARGS_NUM_MAX + 1];
extern int   g_is_bg;   // 1 if command ended with &

/*=============================================================================
* error handling - some useful macros and examples of error handling,
* feel free to not use any of this
=============================================================================*/
#define ERROR_EXIT(msg) \
    do { \
        fprintf(stderr, "%s: %d\n%s", __FILE__, __LINE__, msg); \
        exit(1); \
    } while(0);

static inline void* _validatedMalloc(size_t size)
{
    void* ptr = malloc(size);
    if(!ptr) ERROR_EXIT("malloc");
    return ptr;
}

// example usage:
// char* bufffer = MALLOC_VALIDATED(char, MAX_LINE_SIZE);
// which automatically includes error handling
#define MALLOC_VALIDATED(type, size) \
    ((type*)_validatedMalloc((size)))


/*=============================================================================
* error definitions
=============================================================================*/
typedef enum  {
	INVALID_COMMAND = 0,
	//feel free to add more values here or delete this
} ParsingError;

typedef enum {
	SMASH_SUCCESS = 0,
	SMASH_QUIT,
	SMASH_FAIL
	//feel free to add more values here or delete this
} CommandResult;



/*=============================================================================
* global functions

=============================================================================*/

int showpid(char **args, int argc);

int pwd(char **args, int argc);

int cd(char **args, int argc);

int quit(char **args, int argc, job_arr *jobs);

int jobs(char **args, int argc, job_arr *arr);

int fg(char **args, int argc, job_arr *jobs);

int bg(char **args, int argc, job_arr *jobs);

int kill(char **args, int argc, job_arr *jobs);

int cmd_diff(char **args, int argc);

/* parsing & dispatch */

int parseCommand(char *cmd);

// dispatcher: choose built-in vs external
// numArgs is argc from parseCommand; original_line is the full, unmodified line.

int command_Manager(int numArgs, char *original_line);

/* external commands */

int run_external_command(char **argv, const char *original_line);

/* compound commands with && */

int handle_compound_commands(char *line);




#endif //COMMANDS_H