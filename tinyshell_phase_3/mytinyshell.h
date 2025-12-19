#ifndef MYTINYSHELL_H
#define MYTINYSHELL_H

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
#include <signal.h>
#include <termios.h>

#define MAX_INPUT 2048
#define MAX_CMDS 32
#define MAX_ARGS 128
#define MAX_PATH 1024
#define MAX_JOBS 64

extern char **environ;

//δημιρουργια δομων 
typedef enum {
    RUNNING,
    STOPPED
} job_state;

typedef struct {
    int jid;
    pid_t pgid;
    job_state state;
    char cmdline[MAX_INPUT];
} job_t;

//global μεταβλητες πινακας με δομες και μετρητης διεγρασιων
extern job_t jobs[MAX_JOBS];
extern int next_jid;

//αναφορες συναρτήσεων
char *find_in_path(const char *cmd);
int parse_command(char *segment, char *args[], char **infile, char **outfile, int *append_out, char **errfile);

#endif
