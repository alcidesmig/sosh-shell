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
#include <errno.h>
#include "linenoise.h"
using namespace std;


int verifyDirectory(char *dir);
void completionHook (char const *prefix, linenoiseCompletions *lc);
void attCwdFiles();
void attCwd();
void executeProgram(char *cmd, char *result, char *argv, int background);
void executeFile(char *cmd, char *argv);
void foreground(char *fg);
void changeDirectory(char *path);
void listFiles();
void printCurrentDirectory();
void listJobs();
void handle(char *result);
void sigHandler(int sig);
void init_shell ();

typedef struct
{
    pid_t pid;
    string cmd;
    int id_job;
    int active;
} Job;

#endif /* SHELL_H */
