#include "mytinyshell.h"

int main(void)
{
    char input[MAX_INPUT];

    while (1)
    {
        printf("tinyshell> ");
        fflush(stdout);

        //εισοδος απο το πληκτρολογιο
        if (!fgets(input, sizeof(input), stdin))
        {
            printf("\n");
            break;
        }

        //αφαιρεση αλλαγης γραμμης απο την εισοδο του πληκτρολογιου και εισοδος τερματικου χαρακτηρα
        input[strcspn(input, "\n")] = '\0';

        char *line = input;
        while (*line == ' ' || *line == '\t') line++;
        if (*line == '\0') continue;

        //εντολη τερματισμου
        if (strcmp(line, "exit") == 0) break;

        //διαχωρισμος για διοχετευση αν και εφοσον υπαρχει στην εντολη μας
        char *segments[MAX_CMDS];
        int ncmds = 0;
        char *saveptr = NULL;

        char *tmp = strdup(line);
        char *seg = strtok_r(tmp, "|", &saveptr);
        // διαχωρισμος τις εντολης διοχετευσης σε τι εντολη θελουμε να διοχετευσουμε και που
        while (seg && ncmds < MAX_CMDS)
        {
            while (*seg == ' ' || *seg == '\t') seg++;
            segments[ncmds++] = strdup(seg);
            seg = strtok_r(NULL, "|", &saveptr);
        }
        free(tmp);

        // ευρεση της εντολης που πρεπει να εκτελεστει και διαχορισμο της απο τα υπολοιπα μερη
        char *args[MAX_CMDS][MAX_ARGS];
        char *infile[MAX_CMDS], *outfile[MAX_CMDS], *errfile[MAX_CMDS];
        int append_out[MAX_CMDS];
        char *cmdpath[MAX_CMDS];

        int parse_error = 0;

        for (int i = 0; i < ncmds; i++)
        {
            for (int j = 0; j < MAX_ARGS; j++)
                args[i][j] = NULL;
            
            //αρχικοποιηση των μεταβλητων
            infile[i] = outfile[i] = errfile[i] = NULL;
            append_out[i] = 0;

            // ευρεση ποια εντολη ειναι να εκτελεστει
            char *copy = strdup(segments[i]);
            int argc = parse_command(copy, args[i], &infile[i], &outfile[i], &append_out[i], &errfile[i]);
            free(copy);
            
            //αν το  argc ειναι αρνητικο τοτε σταματα με error
            if (argc <= 0)
            {
                parse_error = 1;
                break;
            }
            // διαχωρισμος εντελων και ευρεση τους στο path
            if (strchr(args[i][0], '/'))
                cmdpath[i] = strdup(args[i][0]);
            else
                cmdpath[i] = find_in_path(args[i][0]);
            //σε περιπτωση που η εντολη ειναι λανθασμενη τοτε ενημερωνει των χρηστη για το λαθος και γυρίζει το προγραμμα πισω
            if (!cmdpath[i])
            {
                fprintf(stderr, "%s: command not found\n", args[i][0]);
                parse_error = 1;
                break;
            }
        }

        // ελευθερωση χωρου απο τους δεικτες
        for (int i = 0; i < ncmds; i++)
            free(segments[i]);

        if (parse_error) continue;

        //εκτελει τις εντολες με την σειρα και διαχειριζεται την διωχετευση τους αν και εφοσον χρειαζεται
        int num_pipes = ncmds - 1;
        int pipefds[2 * MAX_CMDS];

        for (int i = 0; i < num_pipes; i++)
            pipe(pipefds + 2 * i);

        pid_t pids[MAX_CMDS];

        for (int i = 0; i < ncmds; i++)
        {
            pid_t pid = fork();

            if (pid == 0)
            {
                if (i > 0)
                    dup2(pipefds[(i - 1) * 2], STDIN_FILENO);

                if (i < ncmds - 1)
                    dup2(pipefds[i * 2 + 1], STDOUT_FILENO);

                for (int k = 0; k < 2 * num_pipes; k++)
                    close(pipefds[k]);

                // ανακατευθυνση της ειδοδου/εξοδου αν και εφοσον υπαρχει
                if (infile[i]) {
                    int fd = open(infile[i], O_RDONLY);
                    dup2(fd, STDIN_FILENO);
                    close(fd);
                }
                if (outfile[i]) {
                    int fd = open(outfile[i], O_WRONLY | O_CREAT | (append_out[i] ? O_APPEND : O_TRUNC), 0644);
                    dup2(fd, STDOUT_FILENO);
                    close(fd);
                }
                if (errfile[i]) {
                    int fd = open(errfile[i], O_WRONLY|O_CREAT|O_TRUNC, 0644);
                    dup2(fd, STDERR_FILENO);
                    close(fd);
                }

                execve(cmdpath[i], args[i], environ);
                perror("execve");
                exit(1);
            }

            pids[i] = pid;
        }

        for (int k = 0; k < 2 * num_pipes; k++)
            close(pipefds[k]);

        for (int i = 0; i < ncmds; i++)
        {
            int status;
            waitpid(pids[i], &status, 0);
        }

        // ελευθερωνει τους δεικτες
        for (int i = 0; i < ncmds; i++)
        {
            for (int j = 0; args[i][j]; j++)
                free(args[i][j]);
            free(cmdpath[i]);
            if (infile[i]) free(infile[i]);
            if (outfile[i]) free(outfile[i]);
            if (errfile[i]) free(errfile[i]);
        }
    }

    return 0;
}
