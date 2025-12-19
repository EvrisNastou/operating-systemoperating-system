#include "mytinyshell.h"


char *find_in_path(char *cmd){
    static char fullpath[MAX_INPUT];
    char *path = getenv("PATH");
    char *p = strdup(path);
    char *token = strtok(p, ":");

    while (token) {
        snprintf(fullpath, sizeof(fullpath), "%s/%s", token, cmd);
        // ελεγχει αμα υπαρχει η εντολη
        if (access(fullpath, X_OK) == 0) {
            free(p);
            return fullpath;
        }
        token = strtok(NULL, ":");
    }
    free(p);
    return NULL;
}
