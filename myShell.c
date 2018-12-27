#include "myShell.h"

//
//main shell loop
void shellLoop()
{
    char *line;     //allocated in readLine
    char **args;    //allocated in splitLine
    int status, hisSize, sTmp, i;
    char strTmp[2048];
    hisSize = argsCount(history);
    do
    {
        printf("> ");
        //
        //read line
        line = readLine();
        if(line == NULL){
            fprintf(stderr, "no line input!\n");
            continue;
        }
        printf("> > > > line = %s\n", line);
        //
        //save line in history
        sTmp = argsCount(history);
        printf("> > > > history size = %d\n", sTmp);
        history[sTmp] = (char*)malloc(sizeof(char)*(strlen(line)+1));
        printf("> > > > allocated memory in index %d of history : %p\n", sTmp, history[sTmp]);
        if(history[sTmp] == NULL){
            perror("malloc err in shellLoop()");
            fprintf(stderr, "malloc err for history. %s won't be saved\n", line);
            if(line != NULL) free(line);
            continue;
        }
        strcpy(history[sTmp], line);
        printf("> > > > after copy: %s\n", history[sTmp]);
        history[sTmp + 1] = NULL;
        //
        //split line into args
        strcpy(strTmp, line);
        args = splitLine(strTmp);
        if(args == NULL){
            fprintf(stderr, "couldn't splitLine\n");
            if(line != NULL) free(line);
            continue;
        }
            // printf("> args: \n");
            // printArgs(args);
        //
        if(strcmp("!", args[0]) == 0){
            histNum = -1;
            histNum = atoi(args[1]);
            strcpy(strTmp, history[histNum]);
            args = splitLine(strTmp);
        }
        //
        //run the command
        status = runCommand(args);
        if(status == -1){
            fprintf(stderr, "certainy wrong input: %s", line);
        }
        if(status == 0){
            fprintf(stderr, "runCommand() didn't work\n");
        }
        //
        //free memory
        if(line != NULL)    free(line);
        if(args == NULL){
            continue;
        }
        // sTmp = argsCount(args);
        // for(i = 0; i < sTmp; i++){
        //     if(args != NULL && args[i] != NULL){
        //         free(args[i]);
        //     }
        // }
        if(args != NULL){
            free(args);
        }
        if(status == -99){
            break;
        }
    } while (status);
}
//
//run relevant command function
int runCommand(char **args)
{
    int i, r = 1;

    if (args[0] == NULL)
    {
        // An empty command was entered.
        return 0;
    }
    //
    //activate relevant command
    for (i = 0; i < commsNum(); i++)
    {
        //certain it's wrong command
        if(args == NULL){
            return -1;
        }
        //all commands
        if (strcmp(args[0], commsCodes[i]) == 0){
            r = (*commsFuncs[i])(args);
            //printf("matching command: %s\n", commsCodes[i]);
            //app commands
            if(r == 1 && isAppCommand(commsCodes[i]))
            r = appLaunch(args);
            break;
        }
    }
    //
    return r;
}
/*
    l   -   exec_wait         no implemntation
    &   -   exec_continue     no implemenatation
    >   -   change STDOUT and exec_wait implement change stdout
*/
//
//
//called by runCommand to activate program
int appLaunch(char **args)
{
        //printf("> appLaunch() called with args:\n");
        //printArgs(args);
    //vars
    pid_t pid, wpid;
    int status, comm = -1, i, lenT, outFD, s;
    char **argv, *newOut = NULL;
    //argv for exec()
    argv = (char**)malloc(tknsNum * sizeof(char *));
    //create argv for runApp&Wait
    if(strcmp(args[0], "l") == 0){
        comm = 0;
        for(i = 1; i < argsCount(args); i++){
            lenT = strlen(args[i]);
            argv[i-1] = (char*)malloc(sizeof(char)*(lenT+1));
            if(argv[i-1] == NULL){
                perror("malloc err in appLaunch()");
            }
            strcpy(argv[i-1], args[i]);
        }
        argv[++i] = NULL;
    }
    //create argv for runApp&Continue
    if(strcmp(args[0], "&") == 0){
        comm = 1;
        for(i = 1; i < argsCount(args); i++){
            lenT = strlen(args[i]);
            argv[i-1] = (char*)malloc(sizeof(char)*(lenT+1));
            if(argv[i-1] == NULL){
                perror("malloc err in appLaunch()");
                return 0;
            }
            strcpy(argv[i-1], args[i]);
        }
        argv[++i] = NULL;
    }
    //create argv for runAppRedirectOutput&Wait
    if(strcmp(args[0], ">") == 0){
        comm = 2;
        for(i = 1; i < argsCount(args)-1; i++){
            lenT = strlen(args[i]);
            argv[i-1] = (char*)malloc(sizeof(char)*(lenT+1));
            if(argv[i-1] == NULL){
                perror("malloc err in appLaunch()");
                return 0;
            }
            strcpy(argv[i-1], args[i]);
        }
        argv[++i] = NULL;
        lenT = strlen(args[argsCount(args) - 1]);
        newOut = (char*)malloc(sizeof(char) * (lenT + 1));
        if(newOut == NULL){
            perror("malloc err in appLaunch()");
            return 0;
        }
        strcpy(newOut, args[argsCount(args) - 1]);
    }
    //
    //out redirection
    if(comm == 2){
        s = argsCount(args);
        outFD = open(args[s-1], O_WRONLY | O_CREAT | O_TRUNC,S_IRWXU);
    }
    //fork
    pid = fork();
    if (pid == 0)
    {
        // Child process
        //
        //change output stream
        if(comm == 2){
            if(close(STDOUT_FILENO) < 0){
                perror("close in redirect");
                return 0;
            }
            dup(outFD);
        }
        if (execv(args[1], argv) == -1)
        {
            perror("> >execv in appLaunch()");
            return 0;
        }
        exit(EXIT_FAILURE);
    }
    else if (pid < 0)
    {
        // Error forking
        perror("> >fork in appLaunch()");
        return 0;
    }
    else
    {
        // Parent process
        //wait if needed
        if(comm == 0 || comm == 2){
            do
            {
                wpid = waitpid(pid, &status, WUNTRACED);
            } while (!WIFEXITED(status) && !WIFSIGNALED(status));
        }
        //save pid if needed
        if(comm == 1){
            //save pid
            myPids[sizePids] = pid;
            //save index of matching line from history array
            pidsStrHistory[sizePids] = argsCount(history) - 1;
            //advance
            sizePids++;
        }
    }
    int sTmp = argsCount(argv);
    for(i = 0; i < sTmp; i++){
        if(argv[i] != NULL) free(argv[i]);
    }
    if(argv != NULL) free(argv);
    if(newOut != NULL) free(newOut);
    return 1;
}
