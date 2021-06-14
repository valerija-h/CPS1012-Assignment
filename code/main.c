//Includes all functions, libraries and global variables.
#include "header.h"

int main() {
    define_var(); //Sets up the environment variables.
    start(); //Starts the terminal.
}

void start(){
    char *line, **args;
    int status;
    //Loops until the exit command is typed into shell terminal (returning 0).
    do {
        //Prints the command prompt.
        printf("%s",return_var_value("PROMPT"));
        //Reads line.
        line = read_line();
        //Tokenizes line.
        args = split_line(line);
        //Executes line.
        status = execute(args);
    } while (status != 0);
}

//Executes a list of arguments.
int execute(char **args){
    char *first[MAX_SIZE], *second[MAX_SIZE];
    int choice;
    //Executes pipe commands.
    if(is_pipe(args,first,second) != 0) {
        return execute_pipe(first, second);
    }
    //Executes redirection commands.
    if((choice = is_redirect(args,first,second)) != 0){
        return execute_redirect(first,second,choice);
    }
    //Executes variable assignment.
    if(is_var_assignment(args[0]) != 0){
        return 1;
    }
    //Executes internal commands.
    for (int i = 0; i < sizeof(commands_names) / sizeof(char *); i++) {
        //If the name is one of the in-built function names, executes that function.
        if (strcmp(args[0], commands_names[i]) == 0)
            return (*commands[i])(args);
    }
    //Executes external commands.
    launch(args);
    return 1;
}

//Signal Handling function.
void signals (int signal){
    switch(signal) {
        //If CTR-C is caught, terminate process.
        case SIGINT:
            printf("Signal %d caught.\n", signal);
            exit(0); //Exits process.
        //If CRT-Z is caught.
        case SIGSTOP:
            printf("Signal %d caught.\n", signal);
            break;
        default:
            printf("Error - caught wrong signal.\n");
    }
}

//Returns the number of arguments inputted.
int get_size_args(char **args){
    int i = 0;
    //Loops until the current argument is NULL, signifying the last argument.
    while(args[i]!= NULL){
        i++;
    }
    return i;
}

//Splits the arguments array from a character.
int split_args(char **args, char **first, char **second, char *c){
    int n = get_size_args(args); //Determines size of arguments in args.
    //Goes through every argument.
    for(int i=0;i<n;i++){
        //If the current argument is the character, split the 'args' array from this index.
        if(strcmp(args[i],c) == 0){
            //Copies the left half of the argument array.
            memcpy(first, args, (i+1) * sizeof(char *));
            first[i] = NULL;
            //Copies the right half of the argument array.
            memcpy(second, args+(i+1), ((n-i)+1) * sizeof(char *));
            second[n] = NULL;
            return 1;
        } else
            continue;
    }
    return 0; //If the character wasn't one of the arguments returns 0.
}

//Checks if there is redirection and returns argument split by redirect operator.
int is_redirect(char **args, char **red1, char **red2){
    //If a redirection operator is found, returns a value other than 0.
    if (split_args(args,red1,red2, ">") != 0) {
        return 1;
    } else if (split_args(args,red1,red2, ">>") != 0) {
        return 2;
    } else if (split_args(args,red1,red2, "<") != 0) {
        return 3;
    } else if (split_args(args,red1,red2, "<<<") != 0) {
        return 4;
    } else
        return 0; //Return 0 if none of the operators were found.
}

//Checks if there is a pipe and returns argument split into pipe sections.
int is_pipe(char **args, char **pipe1, char **pipe2){
    //If '|' is found, returns the left and right side of pipe arguments.
    if(split_args(args,pipe1,pipe2,"|") != 0){
        return 1;
    } else
        return 0; //Return 0 if '|' not found.
}

//Executes the pipe commands.
int execute_pipe(char **left, char **right){
    int mypipe[2];
    pid_t pid1, pid2;
    //Creating the pipe.
    if(pipe(mypipe) < 0){
        perror("Error -- pipe()");
    }
    //Creating a process.
    pid1 = fork();
    //If the fork failed.
    if(pid1 == -1){
        perror("Error -- fork()");
    } else if (pid1 == 0) {
        //Executes the left hand side of the commands.
        //Sets stdout to the output side of the pipe (the writing end).
        dup2(mypipe[1], STDOUT_FILENO);
        //Closes input side of the pipe.
        close(mypipe[0]);
        //Executes the arguments.
        execute(left);
        exit(0);
    } else {
        //Creating another process.
        pid2 = fork();
        if(pid2 == -1){
            perror("Error -- fork()");
        } else if (pid2 == 0) {
            //Executes the right hand side of the commands.
            //Sets stdin to the input side of the pipe (the reading end).
            dup2(mypipe[0], STDIN_FILENO);
            //Closes output side of the pipe.
            close(mypipe[1]);
            //Executes the arguments.
            execute(right);
            exit(0);
        } else {
            //Closes input side of the pipe.
            close(mypipe[0]);
            //Closes output side of the pipe.
            close(mypipe[1]);
            //Waits for processes to finish.
            wait(NULL);
            wait(NULL);
        }
    }
    return 1;
}

int execute_redirect(char **left, char **right, int choice){
    int status;
    int out_redirection = 0, in_redirection = 0;
    //Determines whether the arguments is an output redirection.
    if(choice == 1 || choice == 2) {
        out_redirection = 1;
    }
    //Determines whether the arguments is an input redirection.
    if (choice == 3 || choice == 4){
        in_redirection = 1;
    }
    //Creating a process.
    pid_t pid = fork();
    if(pid == -1) {
        perror("Error -- fork()");
    } else if (pid == 0) {
        //For output redirection.
        if (out_redirection == 1) {
            FILE *f;
            //Opening an appropriate file depending on whether operator is '>' or '>>'.
            if (choice == 1) {
                if ((f = fopen(right[0], "w")) == NULL)
                    perror("Error -- fopen()");
            }
            if (choice == 2){
                if ((f = fopen(right[0], "a")) == NULL)
                    perror("Error -- fopen()");
            }
            //Setting the stdout to the file.
            dup2(fileno(f), STDOUT_FILENO);
            //Closing the file.
            fclose(f);
            //Executing the arguments.
            execute(left);
            exit(1);
        }
        //For input redirection.
        if (in_redirection == 1){
            FILE *f;
            //Opening an appropriate file depending on whether operator is '<' or '<<<'.
            if(choice == 3){
                if ((f = fopen(right[0], "r")) == NULL)
                    perror("Error -- fopen()");
            }
            if(choice == 4){
                int index = 0;
                //Opening a temporary file ('HERE file') for the input.
                if((f = tmpfile()) == NULL)
                    perror("Error -- fopen()");
                //Copies the right hand arguments into the file.
                while(right[index]!= NULL){
                    fputs(right[index], f);
                    fputs(" ", f);
                    index++;
                }
                fputs("\n", f);
                rewind(f); //Returns to the start of the file.
            }
            //Setting stdin to the file.
            dup2(fileno(f),STDIN_FILENO);
            //Closing the file.
            fclose(f);
            //Executing the arguments.
            execute(left);
            exit(1);
        }
    //Parent process.
    } else {
        //Waits for the child process and returns exit code if waitpid() is successful.
        if(waitpid(pid, &status, WUNTRACED) == -1)
            perror("Error - waitpid()");
        else
            set_exitcode(status); //Sets the exitcode environment variable.
    }
    return 1;
}

//Reads a line from the console and returns it.
char *read_line(){
    char *line = NULL;
    size_t size = 0;
    getline(&line, &size, stdin);
    return line;
}

//Tokenizes the line and returns an array of arguments.
char **split_line(char *line){
    int size = MAX_SIZE, index = 0;
    //Allocates space for the array of arguments.
    char **tokens = malloc(size * sizeof(char *));
    char *token;
    //Acquires the first token.
    token = strtok(line, DELIMITERS);
    //Splits string into tokens.
    while(token != NULL) {
        tokens[index] = token;
        index++;
        //Checks if more space needs to be allocated for the array of arguments.
        if (index >= size) {
            size += MAX_SIZE;
            tokens = realloc(tokens, size * sizeof(char *));
        }
        //Acquires the next tokens.
        token = strtok(NULL, DELIMITERS);
    }
    tokens[index] = NULL;
    //Returns an array of tokens.
    return tokens;
}

/* --------------------- COMMANDS --------------------- */

//The 'exit' internal command - Tells shell to exit.
int exit_comm(char **args){
    return 0;
}

//The 'print' internal command - Displays variable values and strings.
int print_comm(char **args){
    //Executes if no arguments were inputted after 'print'.
    if(args[1] == NULL){
        fprintf(stderr,"Error -- No arguments inputted after the command \'print\'.\n");
    } else {
        int index = 1;
        int n = get_size_args(args); //The number of arguments.
        //If it starts and ends with " - variable insensitive.
        if(args[1][0] == '\"' && args[n-1][strlen(args[n-1])-1] == '\"'){
            //Executes if there is only one word in " " - to avoid errors.
            if(n == 2) {
                char *new_arg = args[index];
                new_arg++; //Removes the first ".
                new_arg[strlen(new_arg)-1] = '\0'; //Removes the second ".
                printf("%s\n",new_arg); //Prints the string.
                return 1;
            }
            //Removes the " from first argument and prints it.
            char *new_arg = args[index];
            new_arg++, index++;
            printf("%s ",new_arg);
            //Prints the rest of the string until the last token.
            while (args[index][strlen(args[index])-1] != '\"') {
                printf("%s ", args[index]);
                index++;
            }
            //Removes the " from last argument and prints it.
            new_arg = args[index];
            new_arg[strlen(new_arg)-1] = '\0';
            printf("%s\n",new_arg);
        } else {
            //Prints all text after print - variable sensitive.
            do {
                //If the start of a token has $ check if it is a possible variable and replace it with the variable value.
                if (args[index][0] == '$') {
                    args[index] = set_var_value(args[index]);
                }
                printf("%s ", args[index]);
                index++;
            } while (args[index] != NULL);
            printf("\n");
        }
    }
    return 1;
}

//The 'chdir' internal command - Changes directories.
int chdir_comm(char **args){
    //Executes if no arguments were inputted after 'chdir'.
    if (args[1] == NULL){
        fprintf(stderr,"Error -- No arguments inputted after the command \'chdir\'.\n");
    } else {
        //Changes the directory using the 'chdir()' function.
        if (chdir(args[1]) == 0){
            printf("Directory has been changed successfully.\n");
            set_cwd(); //Updates the environment variable 'CWD'.
        } else {
            perror("Error -- chdir()");
        }
    }
    return 1;
}

//The 'all' internal command - Displays all variables and their values.
int all_comm(char **args){
    //Displays the environment variable and it's value.
    for(int i=0;i<VAR_SIZE;i++){
        printf("%s=%s\n", variables[i].name, variables[i].value);
    }
    return 1;
}

//The 'source' internal command - Opens a text file, reads it and uses the input to execute commands.
int source_comm(char **args){
    //Executes if no arguments were inputted after 'source'.
    if (args[1] == NULL){
        fprintf(stderr,"Error -- No arguments inputted after the command \'source\'.\n");
    } else {
        FILE *f;
        //Displays error if there are problems opening the file.
        if((f = fopen(args[1], "r")) == NULL){
            perror("Error -- fopen()");
        }
        //Scans each line of text file, parsing it and executing command.
        char line[MAX_SIZE];
        while(fgets(line,sizeof(line), f)){
            //Splits line.
            args = split_line(line);
            //Executes command.
            execute(args);
        }
        //Closing the file.
        fclose(f);
    }
    return 1;
}

//Executes external commands, searching for a program and launching a process.
int launch(char **args){
    int status;
    //Creating a process.
    pid_t pid = fork();
    if (pid == -1) {
        perror("Error - fork()");
    } else if (pid == 0) { //If PID is the child process.
        //Signal handling for processes.
        if(signal(SIGINT, signals) == SIG_ERR)
            perror("Error - signal()");
        //Creates an array of environment variables to be sent to the process.
        char *env[] = {return_env_var("TERMINAL"),return_env_var("CWD"),NULL};
        //Launches the process.
        //execvpe(args[0],args,env) - does not work.
        if (execvp(args[0], args) < 0) {
            perror("Error - execvp()");
        }
    } else { //If PID is the parent process.
        //Waits for the child process and returns exit code if waitpid() is successful.
            if(waitpid(pid, &status, WUNTRACED) == -1)
                perror("Error - waitpid()");
            else
                set_exitcode(status); //Sets the exitcode environment variable.
    }
    return 1;
}

/* --------------------- VARIABLES -------------------- */

//Defines environment variables for current shell.
void define_var(){
    //Adding variables to the environment variables array.
    modify_var("SHELL", "/home/student/cps1012/bin/eggshell");
    modify_var("USER", getenv("USER"));
    modify_var("PROMPT", "> ");
    modify_var("PATH",getenv("PATH"));
    modify_var("HOME",getenv("HOME"));
    set_terminal(); //Finds and sets the terminal value.
    set_cwd(); //Finds and sets the cwd value.
}

//Returns the value of a variable.
char *return_var_value(char *name){
    //Accessing each environment variable.
    for(int i=0;i<VAR_SIZE;i++){
        //If the current environment variable has the same name, return its value.
        if(strcmp(name,variables[i].name) == 0) {
            return variables[i].value;
        }
    }
    return 0;
}

//Re-assigns a value to an environment variable or creates a new one.
int modify_var(char *name, char *value){
    //If no variables were inputted yet, allocate memory for one.
    if(VAR_SIZE == 0){
        VAR_SIZE++;
        //Allocating memory for one variable.
        variables = calloc((size_t)VAR_SIZE, sizeof(VARIABLE));
        //Copying the arguments into the new array element.
        strcpy(variables[VAR_SIZE-1].name, name);
        strcpy(variables[VAR_SIZE-1].value, value);
        return 1;
    }
    //If the variable exists, replace it's contents.
    for(int i=0;i<VAR_SIZE;i++) {
        //If the current variable has the same name as the argument.
        if (strcmp(name, variables[i].name) == 0) {
            //Replaces the variable value.
            strcpy(variables[i].value, value);
            return 1;
        }
    }
    //Else, creates a new variable.
    VAR_SIZE++;
    //Allocating memory for a new variable.
    variables = realloc(variables,(VAR_SIZE)*sizeof(VARIABLE));
    //Copying the arguments into the new array element.
    strcpy(variables[VAR_SIZE-1].name, name);
    strcpy(variables[VAR_SIZE-1].value, value);
    return 1;
}

//Checks if there is an variable assignment - and modifies the environment variable accordingly.
int is_var_assignment(char *arg){
    //If there is an '=' in the string.
    if(strstr(arg, "=") != NULL){
        //Replace environment variables with their values if found.
        arg = set_var_value(arg);
        char *token;
        //Allocates memory for two tokens.
        char **tokens = malloc(2 * sizeof(char *));
        int index = 0;
        //Extracts the first token.
        token = strtok(arg, "=");
        while(token != NULL) {
            tokens[index] = token;
            index++;
            //If there are more than two tokens, it is not a variable assignment.
            if(index > 2){
                return 0;
            }
            //Extracts the next tokens.
            token = strtok(NULL, "=");
        }
        //Call function to add a new environment variable or replace its value.
        modify_var(tokens[0],tokens[1]);
        return 1;
    }
    return 0; //If it is not a variable assignment.
}

//Replaces the $VAR with the value of the variable and returns argument.
char *set_var_value(char *arg){
    char *start;
    static char new_arg[MAX_SIZE];
    //Loops through all environment variable.
    for(int i=0;i<VAR_SIZE;i++) {
        //Adds the $ to the current variable name.
        char temp[MAX_SIZE+1] = "$";
        strcat(temp,variables[i].name);
        //If the '$VAR' is found in argument, replace it with the value.
        if (!(start = strstr(arg, temp))) {
            continue;
        } else {
            //Replaces the location of the $VAR with it's value.
            strncpy(new_arg, arg, start-arg);
            new_arg[start-arg] = '\0';
            sprintf(new_arg+(start-arg), "%s%s", variables[i].value, start+strlen(temp));
            return new_arg; //Returns modified string.
        }
    }
    return arg; //Returns old string.
}

//Update exitcode variable based on input 'status'.
void set_exitcode(int status){
    char exitcode[MAX_SIZE];
    sprintf(exitcode,"%d",status);
    modify_var("EXITCODE",exitcode);
}

//Finds the cwd variable and updates it's variable.
void set_cwd(){
    char cwd[MAX_SIZE];
    getcwd(cwd, sizeof(cwd));
    modify_var("CWD",cwd);
}

//Finds the terminal variable and updates it's variable.
void set_terminal(){
    char *terminal;
    terminal = ttyname(STDOUT_FILENO);
    modify_var("TERMINAL",terminal);
}

//Sends an environment variable name and value as one string.
char *return_env_var(char *name){
    char *temp;
    //Accessing each environment variable.
    for(int i=0;i<VAR_SIZE;i++){
        //If the variable name and argument name are the same.
        if(strcmp(name,variables[i].name) == 0) {
            temp = strcat(variables[i].name,"=");
            //Returns a string in the form VAR=VALUE.
            return strcat(temp,variables[i].value);
        }
    }
    return 0;
}