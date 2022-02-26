/*
 * dsh.c
 *
 * Created on: February 25, 2022
 * Author: Alysa Vermeulen
 */
#include "dsh.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wait.h>
#include <sys/types.h>
#include <errno.h>
#include <err.h>
#include <sys/stat.h>
#include <string.h>
#include <stdbool.h>
#include "builtins.h"

char hist[HISTORY_LEN][MAXBUF] = {""}; // holds commands user has given
int commandCount = 0; // tracks number of commands user has given (up to HISTORY_LEN)

/**
 * Manages command line input from the user: constructs the list
 * of args, determines whether the new process should run in the
 * background, and handles the command based on whether the user
 * has provided an absolute path to an executable file or not
 * @param input command-line input from the user
 */
void manageInput(char *input){
    // add input to history
    if (commandCount==HISTORY_LEN){
        int j;
        for (j = 0; j < HISTORY_LEN-1; j++){
            strcpy(hist[j], hist[j+1]);
        }
        strcpy(hist[HISTORY_LEN-1], input);
    }
    else{
        strcpy(hist[commandCount], input);
        commandCount++;
    }
    // get arguments from input
    int numArgs = countArgs(input);
    char *args[numArgs];
    char *token = NULL;
    token = strtok(input, " "); // get first argument in command line (the command)
    int i = 0;
	while (token != NULL){
        // allocate space for one argument (and null terminator)
        args[i] = malloc(strlen(token)+1);
        strcpy(args[i], token); // copy argument into args list
        token = strtok(NULL, " "); // get next argument in command line
        i++;
    }
    bool b = false; // tracks whether new process should run in background
    // check if last character in command line is ampersand
    char *c = args[numArgs-1];
    if (strcmp(c,"&") == 0){
        b = true;
        free(args[numArgs-1]); // remove & from args list
        numArgs--;
    }
    args[numArgs] = NULL; // denote end of args list
    if (input[0] == '/'){ // if user has given an absolute path to an executable file
		runProgram(args, numArgs, b);
	}
    else{ // if user has simply provided the name of an executable
        findLocation(args, numArgs, b);
    }
    // after command line has been processed, free pointers in args list
    for (i = 0; i < numArgs; i++){
        free(args[i]);
    }
}

/**
 * Counts the number of arguments in the command line
 * @param input command-line input from the user
 * @return the number of arguments in the command line
 */
int countArgs(char *input){
    int i = 0;
    int numArgs = 1;
	while (input[i] != '\0'){
        if(input[i] == ' '){
            numArgs++;
        }
        i++;
    }
    return numArgs;
}

/**
 * Attempts to run a new program given an absolute path
 * to the executable file
 * @param args command-line arguments for the executable
 * @param numArgs the number of arguments in the command line
 * @param b true if new process should run in background, false otherwise
 */
void runProgram(char **args, int numArgs, bool b){
    char *path = args[0];
    // check if path exists
    if (access(path, F_OK | X_OK) == 0) { // if file exists and is executable
        if (fork() != 0) { // parent
            if (!b){ // run new process in foreground
                wait(NULL);
            }
            else{ // run new process in background
                return;
            }
        }
        else{ // child
            int status = execv(path, args);
            if (status != 0){
                perror("ERROR: ");
            }
        }
    }
    else { // file doesn't exist or is not executable
        printf("ERROR: file does not exist or is not executable.\n");
    }
}

/**
 * Finds the true location of a given command (if command is not builtin)
 * @param args command-line arguments for the executable
 * @param numArgs the number of arguments in the command line
 * @param b true if new process should run in background, false otherwise
 */
void findLocation(char **args, int numArgs, bool b){
    // check if command is a builtin command
    bool builtIn = builtIns(args, numArgs);
    if (!builtIn){
        bool found = false;
        // check if executable can be found in current working directory
        found = checkCwd(args, numArgs, b);
        if (!found){ // check if executable can be found in other locations
            found = checkOtherLocations(args, numArgs, b);
        }
        if (!found){
            char *command = args[0];
            printf("ERROR: %s not found!\n", command);
        }
    }
}

/**
 * Checks if executable can be found in the current working directory
 * @param args command-line arguments for the executable
 * @param numArgs the number of arguments in the command line
 * @param b true if new process should run in background, false otherwise
 * @return true if executable found in current working directory, false otherwise
 */
bool checkCwd(char **args, int numArgs, bool b){
    char *command = args[0];
    char cwd[MAXBUF];
    char *currDir = getcwd(cwd, sizeof(cwd));
    int pathLength = strlen(currDir) + strlen(command) + 2;
    char *path = (char*) malloc(pathLength * sizeof(char));
    int i, j = 0;
    // concatenate command to end of current working directory
    for (i = 0; i < pathLength; i++){
        if (i < strlen(currDir)){
            path[i] = currDir[i];
        }
        else if (i == strlen(currDir)){
            path[i] = '/';
        }
        else{
            path[i] = command[j];
            j++;
        }
    }
    if (access(path, F_OK | X_OK) == 0) { // if file exists and is executable
        free(args[0]);
        args[0] = malloc(strlen(path)+1);
        strcpy(args[0], path); // put absolute path to executable file in args
        free(path);
        runProgram(args, numArgs, b);
        return true;
    }
    free(path);
    return false;
}

/**
 * Checks if executable can be found in any of the locations
 * stored in the environment variable PATH
 * @param args command-line arguments for the executable
 * @param numArgs the number of arguments in the command line
 * @param b true if new process should run in background, false otherwise
 * @return true if executable found in PATH locations, false otherwise
 */
bool checkOtherLocations(char **args, int numArgs, bool b){
    char *command = args[0];
    char *locations = NULL;
    const char *temp = getenv("PATH");
    locations = (char*) malloc(strlen(temp) + 1);
    strcpy(locations, temp);
    char *token = NULL;
    token = strtok(locations, ":"); // get first location
    while (token != NULL){
        int pathLength = strlen(token) + strlen(command) + 2;
        char *path = (char*) malloc(pathLength * sizeof(char));
        int i, j = 0;
        // concatenate command to end of location
        for (i = 0; i < pathLength; i++){
            if (i < strlen(token)){
                path[i] = token[i];
            }
            else if (i == strlen(token)){
                path[i] = '/';
            }
            else{
                path[i] = command[j];
                j++;
            }
        }
        if (access(path, F_OK | X_OK) == 0) { // if file exists and is executable
            free(args[0]);
            args[0] = malloc(strlen(path)+1);
            strcpy(args[0], path); // put absolute path to executable file in args
            free(path);
            free(locations);
            runProgram(args, numArgs, b);
            return true;
        }
        free(path);
        token = strtok(NULL, ":"); // get next location
    }
    free(locations);
    return false;
}

/**
 * Performs builtin commands
 * @param args command-line arguments for the executable
 * @param numArgs the number of arguments in the command line
 * @return true if builtin command, false otherwise
 */
bool builtIns(char **args, int numArgs){
    char *command = args[0];
    int i;
    cmd_t check = chkBuiltin(command);
    if (check == CMD_EXT){
        return false;
    }
    else{
        if (check == CMD_CD){
            char *path;
            if (numArgs > 1){
                path = args[1];
            }
            else{
                path = getenv("HOME");
            }
            chdir(path);
        }
        else if (check == CMD_PWD){
            char cwd[MAXBUF];
            char *currDir = getcwd(cwd, sizeof(cwd));
            printf("%s\n", currDir);
        }
        else if (check == CMD_ECHO){
            if (numArgs > 1){
                for (i = 1; i < numArgs; i++){
                    printf("%s ", args[i]);
                }
                printf("\n");
            }
        }
        else if (check == CMD_HIST){
            for (i = 0; i < commandCount; i++){ // print strings in history list
                printf("%s\n", hist[i]);
            }
        }
        else if (check == CMD_EXIT){
            for (i = 0; i < numArgs; i++){ // free strings in args list
                free(args[i]);
            }
            exit(0);
        }
        return true;
    }
}