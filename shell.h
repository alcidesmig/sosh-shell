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
#include <fcntl.h>
#include "linenoise/linenoise.h"

using namespace std;


int verifyDirectory(char *dir);
void completionHook (char const *prefix, linenoiseCompletions *lc);
void attCwdFiles();
void attCwd();
void addJob(pid_t pid, char *result, int active, int stopped);
void verifySetStopped(int status, pid_t pid);
void executeProgram(char *cmd, char *result, char *argv, int background, int out, char *outFile, int in, char *inFile);
void executeFile(char *cmd, char *argv, char *result, int background);
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
    int stopped;
} Job;

#endif /* SHELL_H */
