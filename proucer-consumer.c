#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <ctype.h>
#include <stdlib.h>

#define command_line 1024 // The maximum length command

int should_run = 1;  // when to exit program
int should_wait = 1; // if process should run in the background

//This code for redirection in 
//create the file and then create a copy of the new file into the old one
void InDirect(char *fileName)
{
    int file_in = open(fileName, O_RDONLY);
    dup2(file_in, 0);
    close(file_in);
}

//This code for redirect out
void OutDirect(char *fileName)
{
    int out = open(fileName, O_WRONLY | O_TRUNC | O_CREAT, 0600);
    dup2(out, 1);
    close(out);
}


void project_cd(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "project: cd: missing argument\n");
    } else {
        if (chdir(args[1]) != 0) {
            perror("project: cd");
        }
	}
}

void project_exit(char **args) {
    exit(0);
}

void project_help(char **args) {
    char *helptext =
        "Project 2- Building terminal "
        "The following commands are built in:\n"
        "  cd       Change the working directory.\n"
        "  exit     Exit the shell.\n"
        "  help     Print this help text.\n"
        ;
    printf("%s", helptext);
}

struct builtin {
    char *name;
    void (*func)(char **args);
};

// Array of built in commands.
struct builtin builtins[] = {
    {"help", project_help},
    {"exit", project_exit},
    {"cd", project_cd},
};

// Returns the number of registered commands.
int project_num_builtins() {
    return sizeof(builtins) / sizeof(struct builtin);
}
// Runs a command.
// the args to run

void run(char *args[])
{
	for (int i = 0; i < project_num_builtins(); i++) {
        if (strcmp(args[0], builtins[i].name) == 0) {
            builtins[i].func(args);
            return;
        }
    }
    pid_t pid;
    if (strcmp(args[0], "exit") != 0)
        {
            pid = fork();
            if (pid < 0) { 
                fprintf(stderr, "Fork Failed");
            }
            else if ( pid == 0) { /* child process */
                execvp(args[0], args);
            }
            else { /* parent process */
                if (should_wait) {
                    waitpid(pid, NULL, 0);
                } else {
                    should_wait = 0;
                }
            }
            InDirect("/dev/tty");
            OutDirect("/dev/tty");
        }
    else {
        should_run = 0;
    }
}

//This is for pipe
void createPipe(char *args[])
{
    int fd[2];
    pipe(fd);

    dup2(fd[1], 1);
    close(fd[1]);

    printf("args = %s\n", *args);

    run(args);

    dup2(fd[0], 0);
    close(fd[0]);
}

/**
 * Creates a tokenized form of the input with spaces to separate words.
 * Token to take care of the spaces to separate the words from the input
 * @return tokenized the tokenized stirng
 */
char * tokenize(char *input)
{
    int i;
    int j = 0;
    char *tokenized = (char *)malloc((command_line * 2) * sizeof(char));

    // add spaces around special characters
    for (i = 0; i < strlen(input); i++) {
        if (input[i] != '>' && input[i] != '<' && input[i] != '|') {
            tokenized[j++] = input[i];
        } else {
            tokenized[j++] = ' ';
            tokenized[j++] = input[i];
            tokenized[j++] = ' ';
        }
    }
    tokenized[j++] = '\0';

    // add null to the end
    char *end;
    end = tokenized + strlen(tokenized) - 1;
    end--;
    *(end + 1) = '\0';

    return tokenized;
}

/**
 * Runs a basic shell.
 * 
 * @return 0 upon completion
 */
int main(void)
{
    char *args[command_line]; // command line arguments
    printf("This is  shell. Working on x86 intel Mac \n");
    while (should_run) {
        printf("Project$ ");
        fflush(stdout);

        char input[command_line];
        fgets(input, command_line, stdin);

        char *tokens;
        tokens = tokenize(input);

        if (tokens[strlen(tokens) - 1] == '&') {
            should_wait = 0;
            tokens[strlen(tokens) - 1] = '\0';
        }

        char *arg = strtok(tokens, " ");
        int i = 0;
        while (arg) {
            if (*arg == '<') {
               InDirect(strtok(NULL, " "));
            } else if (*arg == '>') {
                OutDirect(strtok(NULL, " "));
            } else if (*arg == '|') {
                args[i] = NULL;
                createPipe(args);
                i = 0;
            } else {
                args[i] = arg;
                i++;
            }
            arg = strtok(NULL, " ");
        }
        args[i] = NULL;
  
        run(args);
    }
    return 0;
}
