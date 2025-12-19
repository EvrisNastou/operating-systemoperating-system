#include "mytinyshell.h"


job_t jobs[MAX_JOBS];
int next_jid = 1;


char *find_in_path(const char *cmd)
{
    char fullpath[MAX_PATH];
    char *pathenv = getenv("PATH");
    if (!pathenv) return NULL;

    //ελεγχος για σωστη αντιγραφη του string
    char *p = strdup(pathenv);
    if (!p) return NULL;

    //αναζητηση στο Path για την ευρηση της εντολης
    char *token = strtok(p, ":");
    while (token)
    {
        snprintf(fullpath, sizeof(fullpath), "%s/%s", token, cmd);
        if (access(fullpath, X_OK) == 0)
        {
            char *res = strdup(fullpath);
            free(p);
            return res;
        }
        token = strtok(NULL, ":");
    }

    free(p);
    return NULL;
}

//ευρεση αν χρειαζεται η εντολη αλλαγη στην εισοδο/εξοδο η διοχετευση
int parse_command(char *segment, char *args[], char **infile, char **outfile, int *append_out, char **errfile)
{
    *infile = *outfile = *errfile = NULL;
    *append_out = 0;

    int argc = 0;
    char *token = strtok(segment, " \t");
    //αναγνωριζει την υπαρξη επανακατευθυνσης
    while (token && argc < MAX_ARGS - 1)
    {
        if (strcmp(token, "<") == 0)
        {
            token = strtok(NULL, " \t");
            if (!token) return -1;
            *infile = strdup(token);
        }
        else if (strcmp(token, ">") == 0)
        {
            token = strtok(NULL, " \t");
            if (!token) return -1;
            *outfile = strdup(token);
            *append_out = 0;
        }
        else if (strcmp(token, ">>") == 0)
        {
            token = strtok(NULL, " \t");
            if (!token) return -1;
            *outfile = strdup(token);
            *append_out = 1;
        }
        else if (strcmp(token, "2>") == 0)
        {
            token = strtok(NULL, " \t");
            if (!token) return -1;
            *errfile = strdup(token);
        }
        else
        {
            args[argc++] = strdup(token);
        }

        token = strtok(NULL, " \t");
    }

    args[argc] = NULL;
    return argc;
}