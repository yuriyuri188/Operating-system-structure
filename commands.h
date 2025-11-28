#ifndef COMMANDS_H
#define COMMANDS_H
/*=============================================================================
* includes, defines, usings
=============================================================================*/
#include <stdlib.h>
#include <stdio.h>


#define CMD_LENGTH_MAX 80
#define ARGS_NUM_MAX 20
#define JOBS_NUM_MAX 100
#define MAX_JOBS JOBS_NUM_MAX
#define BUF_SIZE 4096



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
* forward declarations
=============================================================================*/
typedef struct job_arr job_arr;

/*=============================================================================
* global functions

=============================================================================*/
int parseCommandExample(char* line);

int showpid(char **args, int argc);

int pwd(char **args, int argc);

int cd(char **args, int argc);

int quit(char **args, int argc, job_arr *jobs);

int jobs(char **args, int argc, job_arr *arr);

int fg(char **args, int argc, job_arr *jobs);

int bg(char **args, int argc, job_arr *jobs);

int kill(char **args, int argc, job_arr *jobs);

int cmd_diff(char **args, int argc);

int parseCommand(char *cmd);

int command_Manager(int numArgs, char *original_line);

int run_external_command(char **argv, const char *original_line);

int handle_compound_commands(char *line);




#endif //COMMANDS_H