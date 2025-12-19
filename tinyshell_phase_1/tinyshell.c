#include "mytinyshell.h"

int main(){
    char input[MAX_INPUT];
    char* args[MAX_ARGS];
    char* token=NULL;
    int i=0,status;
    pid_t pid;
    char *cmd_path;

    while (1)
    {
        //prompt
        printf("tinyshell>");
        fflush(stdout);

        if(fgets(input,MAX_INPUT,stdin)==NULL){
            printf("\n");
            break;
        }

        //αφαιρει το '\n' και το αντικαθηστα με '\0' και αν εχει δωθει η εντολή exit αμα ειναι για τερματισμει
        input[strcspn(input,"\n")]='\0';

        if(strcmp(input, "exit")==0){
            break;
        }

        
        //αναλύει τα ορίσματα
        token=strtok(input," ");
        
        i=0;
        while (token!=NULL && i<MAX_ARGS)
        {
            args[i++]=token;
            token=strtok(NULL, " ");
        }
        args[i]=NULL;

        //  εντοπίζει εκτελέσιμα στο PATH
        cmd_path = args[0];
        if (strchr(args[0], '/') == NULL) {
            cmd_path = find_in_path(args[0]);
            if (!cmd_path) {
                printf("%s: command not found\n", args[0]);
                continue;
            }
        }
        //τα εκτελεί με fork και execve
        pid=fork();
        
        if (pid==0){
            execve(cmd_path,args, environ);
            perror("execvp failed");
            exit(EXIT_FAILURE);
        }
        else if (pid>0)
        {
            waitpid(pid,&status,0);
            //αναφέρει τους κωδικούς εξόδου
            if (WIFEXITED(status)) {
                printf("Exit code: %d\n", WEXITSTATUS(status));
            } else if (WIFSIGNALED(status)) {
                printf("Terminated by signal: %d\n", WTERMSIG(status));
            }
        }
        else{
            perror("Forck failed");
        }
    }

    return 0;
}
