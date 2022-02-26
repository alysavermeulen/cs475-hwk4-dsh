/*
 * dsh.h
 *
 * Created on: February 25, 2022
 * Author: Alysa Vermeulen
 */

#include <stdbool.h>

#define MAXBUF 256
#define HISTORY_LEN 100

// TODO: Your function prototypes below
void manageInput(char *input);
int countArgs(char *input);
void runProgram(char **args, int numArgs, bool b);
void findLocation(char **args, int numArgs, bool b);
bool checkCwd(char **args, int numArgs, bool b);
bool checkOtherLocations(char **args, int numArgs, bool b);
bool builtIns(char **args, int numArgs);