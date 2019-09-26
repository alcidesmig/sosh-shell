#ifndef SHELL_H
#define SHELL_H

#include <sys/types.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <limits.h>
#include <sys/wait.h> 
#include <vector>
#include <iostream>
#include <sys/stat.h>
#include "linenoise.h"
using namespace std;


void init_shell();
int verifyDirectory(char * dir);
void completionHook (char const* prefix, linenoiseCompletions* lc);
void attCwdFiles();
void attCwd();
void handle(char *result);

typedef struct {
	pid_t pid;
	string cmd;
	int id_job;
	int active;
} Job;

#endif /* SHELL_H */
