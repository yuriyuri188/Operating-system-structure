//smash.c

/*=============================================================================
* includes, defines, usings
=============================================================================*/
#include <stdlib.h>
#include <stdio.h>
#include "commands.h"
#include "signals.h"
#include "jobs.c"


/*=============================================================================
* classes/structs declarations
=============================================================================*/

/*=============================================================================
* global variables & data structures
=============================================================================*/
char _line[CMD_LENGTH_MAX];

/*=============================================================================
* main function
=============================================================================*/
int main(int argc, char* argv[])
{
	char _cmd[CMD_LENGTH_MAX];

	MainHandleConfigPack(); // initialize signals

	init_job_arr(&job_list); //init jobs array


	while(1) {
		printf("smash > "); //Every shell prints a prompt
		fgets(_line, CMD_LENGTH_MAX, stdin); // reads a full line from the keyboard into line


		size_t len = strlen(_line);
		if (len > 0 && _line[len - 1] == '\n') {
			_line[len - 1] = '\0';
		}

		// skip empty/whitespace-only lines
		int only_ws = 1;
		for (char *p = _line; *p != '\0'; ++p) {
			if (*p != ' ' && *p != '\t') {
				only_ws = 0;
				break;
			}
		}
		if (only_ws) {
			_line[0] = '\0';
			continue;
		}

		 // ===== detect compound command (&&) =====
        if (strstr(_line, "&&") != NULL) {
            // handle chain "cmd1 && cmd2 && cmd3"
            (void)handle_compound_commands(_line);
        } else {
            // ===== simple single command =====
            strcpy(_cmd, _line);     // copy to parse buffer

            int numArgs = parseCommand(_cmd);      // fills g_argv, sets g_is_bg
            (void)command_Manager(numArgs, _line); // execute (we ignore return here)
        }

        // reset buffers for next line
        _line[0] = '\0';
        _cmd[0] = '\0';
    }

    return 0;
}
