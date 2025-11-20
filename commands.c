//commands.c
#include "commands.h"
#include <errno.h>
#include <string.h>
#include "my_system_call.h"
#include <sys/stat.h>


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

int cmd_showpid(char **args, int argc)
{
    if (argc != 0)
	{
		fprintf(stderr, "smash error: pwd: expected 0 arguments\n");
		return -1;
		}
	
		printf("smash pid is %d\n", getpid());
		return 0;
	    
}


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

int kill(char **args, int argc)
{
	if (argc != 2)
	{
		fprintf(stderr, "smash error: kill: invalid arguments\n");
		return -1;
	}
	
	int pid = atoi(args[1]);
	

}

int diff(char **args, int argc)
{
	if (argc != 2)
	{
		fprintf(stderr, "smash error: diff: invalid arguments\n");
		return -1;
	}

	const char *path1 = args[1];
	const char *path2 = args[2];

	struct stat stat1, stat2;
	//Check that both paths exist
	if (stat(path1, &st1) != 0 || stat(path2, &st2) != 0) 
	{
		fprintf(stderr, "smash error: diff: stat failed\n");
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
		fprintf(stderr, "smash error: expected valid paths for file\n");
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
		long r1  = my_system_call(SYS_READ, fd2, buf2, sizeof(buf2));

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


