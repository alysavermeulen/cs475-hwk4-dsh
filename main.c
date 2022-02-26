/*
 * main.c
 *
 * Created on: February 25, 2022
 * Author: Alysa Vermeulen
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <ctype.h>
#include "dsh.h"

int main(int argc, char **argv)
{
	char cmdline[MAXBUF]; // stores user input from commmand line

	// check if .dsh_motd exists in HOME
	char *homeValue = getenv("HOME");
    char *fileName = "/.dsh_motd";
    int pathLength = strlen(homeValue) + strlen(fileName) + 1;
    char *path = (char*) malloc(pathLength * sizeof(char));
    int i, j = 0;
	// concatenate home directory and file name
    for (i = 0; i < pathLength; i++){
        if (i < strlen(homeValue)){
            path[i] = homeValue[i];
        }
        else{
            path[i] = fileName[j];
            j++;
        }
    }

	// attempt to open file (will succeed if file exists and is readable)
	FILE *file;
    file = fopen(path, "r");
    free(path);
    int c;
    if (file){ // if file was successfully opened
        while ((c = getc(file)) != EOF){ // print contents of file
            putchar(c);
        }
        fclose(file);
    }
    printf("\n");
    
	bool processCommand = false; // tracks whether user has issued a command
    do{
		if (processCommand && isprint(cmdline[0])){ // if user has issued a command
			manageInput(cmdline);
		}
		printf("dsh> ");
		fgets(cmdline, MAXBUF, stdin);
        int len = strlen(cmdline);
        if(cmdline[len-1] == '\n'){
            cmdline[len-1] = '\0'; // remove newline character
            // remove leading whitespace
            i = 0; // will hold index of last leading whitespace character
            while(cmdline[i] == ' ' || cmdline[i] == '\t'){ 
                i++;
            }
            if (i != 0){ // if there is leading whitespace
                j = 0;
                while (cmdline[j + i] != '\0'){ // shift characters to the left 
                    cmdline[j] = cmdline[j + i];
                    j++;
                }
                cmdline[j] = '\0';
            }
            // remove trailing whitespace
            i = 0; // will store last index of a non-whitespace character
            j = 0;
            while(cmdline[j] != '\0'){
                if(cmdline[j] != ' ' && cmdline[j] != '\t'){
                    i = j;
                }
                j++;
            }
            cmdline[i + 1] = '\0';
            processCommand = true;
        }
        else{ // input longer than maxbuf (256 characters)
            printf("Exceeded max input length of %d characters\n", MAXBUF);
            while ((getchar()) != '\n'); // clear input buffer
            processCommand = false;
        }
	} while (1);

	return 0;
}
