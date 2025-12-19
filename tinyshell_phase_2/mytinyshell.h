#ifndef UTILITIES_H
#define UTILITIES_H

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#define MAX_INPUT 2048
#define MAX_CMDS 32
#define MAX_ARGS 128
#define MAX_PATH 1024

extern char **environ;


char *find_in_path(const char *cmd);
int parse_command(char *segment, char *args[], char **infile, char **outfile, int *append_out, char **errfile);



#endif