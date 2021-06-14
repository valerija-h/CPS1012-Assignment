#ifndef OS_THING_HEADER_H
#define OS_THING_HEADER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <memory.h>
#define _GNU_SOURCE
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <fcntl.h>

#define DELIMITERS " \t\r\n"
#define MAX_SIZE 1024

/* --------- FUNCTION DEFINITIONS ------- */

/* Core Functions */
void start();
char *read_line();
char **split_line(char *line);
int execute (char **args);
int execute_redirect(char **left, char **right, int choice);
int execute_pipe(char **left, char **right);

/* Other Functions */
int is_redirect(char **args, char **red1, char **red2);
int is_pipe(char **args, char **pipe1, char **pipe2);
int split_args(char **args, char **first, char **second, char *c);
int get_size_args(char **args);

/* Functions for Variables */
int modify_var(char *name, char *value);
int is_var_assignment(char *arg);
char *return_var_value(char *name);
char *return_env_var(char *name);
char *set_var_value(char *arg);
//Setting Environment Variables
void define_var();
void set_terminal();
void set_exitcode(int status);
void set_cwd();

/* Functions for Commands */
//Internal Commands.
int exit_comm(char **args);
int print_comm(char **args);
int chdir_comm(char **args);
int all_comm(char **args);
int source_comm(char **args);
//External Commands.
int launch (char **args);

/* Functions for Process Management */
//Signalling Functions
void signals (int signal);

/* ---------- GLOBAL VARIABLES ---------- */

/* Definitions for Variables */
typedef struct variable {
    char name[MAX_SIZE];
    char value[MAX_SIZE];
} VARIABLE;

VARIABLE *variables;
int VAR_SIZE = 0; //Number of environment variables.

/* Definitions for Commands */
//An array of pointers to command functions.
int (*commands[]) (char **) = {&exit_comm,&print_comm,&chdir_comm,&all_comm,&source_comm};
//An array of commands names.
char *commands_names[] = {"exit","print","chdir","all","source"};

#endif //OS_THING_HEADER_H