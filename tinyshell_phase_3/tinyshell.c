#include "mytinyshell.h"



int main(void)
{
    //----αρχικοποιηση----
    char input[MAX_INPUT];
    pid_t shell_pgid;

    
    // ελεγχει οτι τρεχει σε πραγματικο terminal
    int shell_terminal = STDIN_FILENO;
    if (!isatty(shell_terminal)) {
        // Αν δεν ειναι σε τερματισε
        exit(1);
    }

    //αγνωει τα σηματα για να μην παγωσει και περιμενει να εκτελεση της διεργασιας tcsetpgrp
    //δεν κινδυνευει να σκοτωσει τον ευαυτο του σε περιπτωση crtl+z
    signal(SIGINT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN); 

    //θετει το shell σαν leader του ιδιου του ευατου του 
    shell_pgid = getpid();
    if (setpgid(shell_pgid, shell_pgid) < 0) {
        perror("Couldn't put the shell in its own process group");
        exit(1);
    }

    //περνει τον ελεγχο απο το terminal
    tcsetpgrp(shell_terminal, shell_pgid);


    while (1)
    {
        printf("tinyshell> ");
        fflush(stdout);

        //περνει την εισοδο που περναει ο χρηστης
        if (!fgets(input, sizeof(input), stdin)) {
            printf("\n");
            break;
        }

        //κατασκευαζει την εισοδο σε token ωστε να μπορει να τα περασει μετα παρακατω
        input[strcspn(input, "\n")] = '\0';
        char *line = input;
        while (*line == ' ' || *line == '\t') line++;
        if (*line == '\0') continue;

        // ελεγχεις για διεργασιες στο παρασκηνιο '&'
        int background = 0;
        int len = strlen(line);
        if (len > 0 && line[len - 1] == '&') {
            background = 1; //ενεγροποιηση οτι βρεθηκε εντολη στο background
            line[len - 1] = '\0'; // βγαζει τον & που ειναι για να περασει την διεργασια στο παρασκηνιο 
        }

        if (strcmp(line, "exit") == 0) break; //τερματιζει οταν βρει την εντολη exit

        // --- δημιουργια του fg ---
        if (strncmp(line, "fg", 2) == 0) {
            // υποστιριζει "fg %1"
            char *percent = strchr(line, '%');
            if (!percent) {
                fprintf(stderr, "usage: fg %%job\n");
                continue;
            }
            int jid = atoi(percent + 1);
            
            // επιβεβαιωνει αν υπαρχει η διεγρασια
            if (jid <= 0 || jid >= next_jid || jobs[jid].state == -1) {
                fprintf(stderr, "no such job\n");
                continue;
            }

            pid_t pgid = jobs[jid].pgid;

            // 1. στελνει SIGCONT σε περιπτωση που διακοπει η διαδικασια
            if (kill(-pgid, SIGCONT) < 0) perror("kill (SIGCONT)");

            // 2. δινει τον ελεγχο στο terminal
            tcsetpgrp(shell_terminal, pgid);

            // 3. περιμενει να τελειωσει η διεργασια
            jobs[jid].state = RUNNING;
            int status;
            waitpid(-pgid, &status, WUNTRACED);

            // 4. δινει το terminal πισω
            tcsetpgrp(shell_terminal, shell_pgid);

            // 5. ελεγχει αμα εχει διακοπη η διεγρασια
            if (WIFSTOPPED(status)) {
                jobs[jid].state = STOPPED;
                printf("[%d]+ Stopped %s\n", jid, jobs[jid].cmdline);
            } 
            continue;
        }

        // --- δημιουγρια του BG ---
        if (strncmp(line, "bg", 2) == 0) {
            char *percent = strchr(line, '%');
            if (!percent) {
                fprintf(stderr, "usage: bg %%job\n");
                continue;
            }
            int jid = atoi(percent + 1);

            if (jid <= 0 || jid >= next_jid) {
                 fprintf(stderr, "no such job\n");
                 continue;
            }

            pid_t pgid = jobs[jid].pgid;
            
            // 1. στελνει SIGCONT
            kill(-pgid, SIGCONT);
            jobs[jid].state = RUNNING;
            
            // 2. δεν χρειαζεται να περιμενει τρεχει κανονικα το terminal 
            printf("[%d]+ %s &\n", jid, jobs[jid].cmdline);
            continue;
        }

        // εκτελεση απλων διεργασιων
        char *segments[MAX_CMDS];
        int ncmds = 0;
        char *saveptr = NULL;
        char *tmp = strdup(line);
        char *seg = strtok_r(tmp, "|", &saveptr);
        while (seg && ncmds < MAX_CMDS) {
            while (*seg == ' ' || *seg == '\t') seg++;
            segments[ncmds++] = strdup(seg);
            seg = strtok_r(NULL, "|", &saveptr);
        }
        free(tmp);


        char *args[MAX_CMDS][MAX_ARGS];
        char *infile[MAX_CMDS], *outfile[MAX_CMDS], *errfile[MAX_CMDS];
        int append_out[MAX_CMDS];
        char *cmdpath[MAX_CMDS];
        int parse_error = 0;

        for (int i = 0; i < ncmds; i++) {
            for (int j = 0; j < MAX_ARGS; j++) args[i][j] = NULL;
            infile[i] = outfile[i] = errfile[i] = NULL;
            append_out[i] = 0;

            char *copy = strdup(segments[i]);
            int argc = parse_command(copy, args[i], &infile[i], &outfile[i], &append_out[i], &errfile[i]);
            free(copy);

            if (argc <= 0) { parse_error = 1; break; }
            
            if (strchr(args[i][0], '/')) cmdpath[i] = strdup(args[i][0]);
            else cmdpath[i] = find_in_path(args[i][0]);

            if (!cmdpath[i]) {
                fprintf(stderr, "%s: command not found\n", args[i][0]);
                parse_error = 1;
                break;
            }
        }
        for (int i = 0; i < ncmds; i++) free(segments[i]);
        if (parse_error) continue;

        // ---εκτέλεση εντολων---
        int pipefds[2 * MAX_CMDS];
        for (int i = 0; i < ncmds - 1; i++) pipe(pipefds + 2 * i);

        pid_t pids[MAX_CMDS];
        pid_t group_leader = 0;

        for (int i = 0; i < ncmds; i++)
        {
            pid_t pid = fork();

            if (pid == 0) // παιδι
            {
                // κανει reset τα σηματα σε καθε παιδι για να ειναι ετοιμα
                signal(SIGINT, SIG_DFL);
                signal(SIGTSTP, SIG_DFL);
                signal(SIGQUIT, SIG_DFL);
                signal(SIGTTIN, SIG_DFL);
                signal(SIGTTOU, SIG_DFL);


                pid_t my_pid = getpid();
                if (i == 0) group_leader = my_pid; 
                setpgid(my_pid, group_leader);

                // C. σολινωση και ανακατευθηνση
                if (i > 0) dup2(pipefds[(i - 1) * 2], STDIN_FILENO);
                if (i < ncmds - 1) dup2(pipefds[i * 2 + 1], STDOUT_FILENO);
                for (int k = 0; k < 2 * (ncmds - 1); k++) close(pipefds[k]);

                if (infile[i]) {
                    int fd = open(infile[i], O_RDONLY);
                    dup2(fd, STDIN_FILENO); close(fd);
                }
                if (outfile[i]) {
                    int fd = open(outfile[i], O_WRONLY | O_CREAT | (append_out[i] ? O_APPEND : O_TRUNC), 0644);
                    dup2(fd, STDOUT_FILENO); close(fd);
                }
                if (errfile[i]) {
                    int fd = open(errfile[i], O_WRONLY|O_CREAT|O_TRUNC, 0644);
                    dup2(fd, STDERR_FILENO); close(fd);
                }

                execve(cmdpath[i], args[i], environ);
                perror("execve");
                exit(1);
            }
            
            //θετουμε και στον πατερα pid για να αποφυγουμε προβληματα
            if (i == 0) group_leader = pid;
            setpgid(pid, group_leader);
            pids[i] = pid;
        }

        // κλεινει τις σολιωνσεις στην στον πατερα
        for (int k = 0; k < 2 * (ncmds - 1); k++) close(pipefds[k]);

        // --- ελεγχος διεγρασια ---
        pid_t job_pgid = pids[0];

        // αποθηκευση τις διεγρασιας
        int jid = next_jid++;
        jobs[jid].jid = jid;
        jobs[jid].pgid = job_pgid;
        jobs[jid].state = RUNNING;
        strncpy(jobs[jid].cmdline, line, MAX_INPUT);

        if (!background) 
        {
            // δινει στο terminal για να εκτελεστει στο παρασκηνιο
            tcsetpgrp(shell_terminal, job_pgid);

            // 2. περιμενει μια διεργασια
            int status;
            waitpid(-job_pgid, &status, WUNTRACED);

            // 3.επιστρεφει το terminal
            tcsetpgrp(shell_terminal, shell_pgid);

            if (WIFSTOPPED(status)) {
                jobs[jid].state = STOPPED;
                printf("\n[%d]+ Stopped %s\n", jid, jobs[jid].cmdline);
            }
        } 
        else 
        {
            // εκτελεση στο παρασκηνιο
            printf("[%d] %d\n", jid, job_pgid);
        }

        // ελευθερωση μνημης
        for (int i = 0; i < ncmds; i++) {
            for (int j = 0; args[i][j]; j++) free(args[i][j]);
            free(cmdpath[i]);
            if (infile[i]) free(infile[i]);
            if (outfile[i]) free(outfile[i]);
            if (errfile[i]) free(errfile[i]);
        }
    }
    return 0;
}