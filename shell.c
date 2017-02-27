//
//  main.c
//  shell
//
//  Created by Janna Shaftan on 2/25/17.
//  Copyright © 2017 Janna S. All rights reserved.
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>


#define MAX_SUB_COMMANDS 5
#define MAX_ARGS 10

struct SubCommand
{
    char *line;
    char *argv[MAX_ARGS];
    char *stdin_redirect;
    int redirect_pos;
    char *stdout_redirect;
};

struct Command
{
    struct SubCommand sub_commands[MAX_SUB_COMMANDS];
    int num_sub_commands;
    char *stdin_redirect;
    char *stdout_redirect;
    int background;
};

void ReadCommand(char *line, struct Command *command);
void ReadRedirectsAndBackground(struct Command *command);
void ExecuteCommand(struct Command *cmd);
void ExecuteSubCommand(struct SubCommand *subcmd);
void ExecuteSubCommandInBackGround(struct SubCommand *subcmd);
void CreatePipe(struct Command *cmd);
void redirect_in(struct SubCommand *scmd);
void redirect_out(struct SubCommand *scmd);
void cd(char * path);
int ReadArgs(char *in, char **argv, int size);
void PrintCommand(struct Command *command);
void PrintArgs(char **argv);
void trim(char *str);


void main(int argc, const char * argv[]) {
   
    while(1){
        struct Command cmd;
        char s[200];
        printf("$ ");
        fgets(s, sizeof s, stdin);
        ReadCommand(s, &cmd);
        ReadRedirectsAndBackground(&cmd);
        ExecuteCommand(&cmd);
        fflush(stdin);
        fflush(stdout);
    }
}




void ReadCommand(char *line, struct Command *command){
    int i = 0;
    const char s[2] = "|";
    char *tokens, *token;
    
    struct SubCommand scmd [MAX_SUB_COMMANDS] = { NULL };
    
    tokens = strdup(line);
    token = strtok(tokens, s);
    
    while(token != NULL){
        scmd[i].line = token;
        trim(scmd[i].line);
        command -> sub_commands[i] = scmd[i];
        token = strtok(NULL, s);
        i++;
    }
    
    command -> num_sub_commands = i;
    
    for (int j = 0; j < i; j++){
        ReadArgs(command->sub_commands[j].line, command->sub_commands[j].argv, 10);
    }
}

void ReadRedirectsAndBackground(struct Command *command){
    int num_cmd = command->num_sub_commands - 1;
    
    if (strchr(command->sub_commands[num_cmd].line, '&')){
        command->background = 1;
    }
    
    for (int j = num_cmd + 1; j-- > 0;){
        int k = 0;
        while(command->sub_commands[j].argv[k] != NULL){
            if(strchr(command->sub_commands[j].argv[k], '<')){
                command->stdin_redirect = command->sub_commands[j].argv[k+1];
                command->sub_commands[j].stdin_redirect = command->sub_commands[j].argv[k+1];
                command->sub_commands[j].redirect_pos = k;
            }
            else if(strchr(command->sub_commands[j].argv[k], '>')){
                command->stdout_redirect = command->sub_commands[j].argv[k+1];
                command->sub_commands[j].stdout_redirect = command->sub_commands[j].argv[k+1];
                command->sub_commands[j].redirect_pos = k;
            }
            k++;
        }
    }
}

void ExecuteCommand(struct Command *cmd){
    if(strcmp(cmd->sub_commands[0].argv[0], "cd") == 0){
        cd(cmd->sub_commands[0].argv[1]);
    }
    else if(cmd->num_sub_commands > 1){
        CreatePipe(cmd);
    }
    else {
        if (cmd->background == 1) {
            ExecuteSubCommandInBackGround(&cmd->sub_commands[0]);
        }
        else {
            ExecuteSubCommand(&cmd->sub_commands[0]);
        }
    }
}

void redirect_out(struct SubCommand *scmd){
    int out = open(scmd->stdout_redirect, O_WRONLY | O_CREAT);
    
    if(out<0){
        printf("Couldn't create this file. \n");
    }
    else {

        int pid = fork();
    
        if(!pid){
            dup2(out, STDOUT_FILENO);
            char * new_arg[2] = {scmd->argv[0], NULL};
            if (execvp(scmd->argv[0], new_arg) < 0){
                write(out, scmd->argv[0], strlen(scmd->argv[0]));
                exit(1);
            }
        }
        else{
            wait(NULL);
        }
    }
}

void redirect_in(struct SubCommand *scmd){
    
    pid_t pid;
    int in = open(scmd->stdin_redirect, O_RDONLY, 0664);
    
    if(in < 0){
        printf("Couldn't open this file. \n");
    }
    else {
        pid = fork();
    
        if(!pid){
            dup2(in, 0);
            char * new_arg[2] = {scmd->argv[0], NULL};
            if ((execvp(scmd->argv[0], new_arg)) < 0){
                printf("No such file");
                exit(1);
            }
        }
        else {
            wait(NULL);
        }
    }

}


void CreatePipe(struct Command *cmd){
    int i;
    int numPipes = cmd->num_sub_commands-1;
    
    for( i = 0; i < numPipes; i++){
        int fd[2];
        pipe(fd);
        
        if(fork() == 0 && i < numPipes){ //child
            dup2(fd[1], 1);
            execvp(cmd->sub_commands[i].argv[0], cmd->sub_commands[i].argv);
            printf("Error: Command Not Found. \n");
        }
        
        dup2(fd[0], 0);
        close(fd[1]);
        
    }
    execvp(cmd->sub_commands[i].argv[0], cmd->sub_commands[i].argv);
    printf("Error: Command Not Found. \n");
}


void ExecuteSubCommand(struct SubCommand *subcmd){
    if(subcmd->stdin_redirect !=NULL){
        redirect_in(subcmd);
    }
    else if(subcmd->stdout_redirect !=NULL){
        redirect_out(subcmd);
    }
    else {
        int fdesc[2];
        pipe(fdesc);
    
        int fdesc_ret = fork();
    
        if(fdesc_ret == 0){

            execvp(subcmd->argv[0], subcmd->argv);
            printf("Error: Command Not Found. \n");
        }
    
        int child_pid = wait(NULL);
    }
}

void ExecuteSubCommandInBackGround(struct SubCommand *subcmd){
        int fdesc[2];
        pipe(fdesc);
        printf("[%d]\n", getpid());

        int fdesc_ret = fork();

        if(fdesc_ret == 0){
            execvp(subcmd->argv[0], subcmd->argv);
            printf("Error: Command Not Found. \n");
        }
    int status;
    waitpid(-1, &status, WNOHANG);

    }


void PrintCommand(struct Command *command){
    printf("The command has %d sub-commands\n",command->num_sub_commands);
    printf("Redirect stdin: ");
    if (command->stdin_redirect != NULL){
        printf("%s \n", command->stdin_redirect);
    }
    else {
        printf("None\n");
    }
    printf("Redirect stdout: ");
    if (command->stdout_redirect != NULL){
        printf("%s\n", command->stdout_redirect);
    }
    else {
        printf("None\n");
    }
    printf("Background: ");
    if (command->background == 1){
        printf("yes \n");
    }
    else{
        printf("no \n");
    }
    for (int i = 0; i < MAX_SUB_COMMANDS; i++){
        if (command->sub_commands[i].line != NULL){
            printf("sub_command [%d] is %s \n", i, command->sub_commands[i].line);
            PrintArgs(command->sub_commands[i].argv);
        }
    }
}

int ReadArgs(char *in, char **argv, int size){
    
    int i = 0;
    const char s[2] = " ";
    
    char *token, *tokens;
    tokens = strdup(in);
    token = strtok(tokens, s);
    
    while (token != NULL && strcmp(token, "&") != 0){
        if (i < size){
            argv[i++] = token;
        }
        token = strtok(NULL, s);
    }
    return i;
}

void PrintArgs(char **argv){
    int k = 0;
    while(argv[k] != NULL){
        printf("arg [%d] = %s \n", k, argv[k]);
        k++;
    }
}

void trim(char *str)
{
    int i;
    int begin = 0;
    int end = strlen(str) - 1;
    
    while (isspace((unsigned char) str[begin]))
        begin++;
    
    while ((end >= begin) && isspace((unsigned char) str[end]))
        end--;
    
    for (i = begin; i <= end; i++)
        str[i - begin] = str[i];
    
    //Add Null Termination Operator to String
    str[i - begin] = '\0';
}

void cd(char * path){
    char buffer[4096];
    char *home = NULL;
    
    //Check if it's a real directory
    if (chdir(buffer) != 0) {
        if(path[0] == '~') {
            home = getenv("HOME");
        }
        if (home != NULL){
            sprintf(buffer, "%s%*s",home, (int)(strlen(path)-1), path+1);
        }
        else{
            sprintf(buffer,"%s",path);
        }
    }
    else {
        printf("Invalid directory");
    }
}


/*This is for two pipes, and does not catch code in the parent.
 This is mentioned in the essay I have included
/*void CreatePipe(struct Command *cmd){

int fd[2];
pipe(fd);

pid_t pid = fork();

if (pid == 0){ //child
    close(fd[0]); //CLOSE READ
    close(1);
    dup(fd[1]);
    
    execvp(cmd->sub_commands[0].argv[0], cmd->sub_commands[0].argv);
}
else{
    
    pid_t pid2 = fork();
    
    if(pid2 == 0){
        close(fd[1]); // CLOSE WRITE
        close(0);
        dup(fd[0]);
        execvp(cmd->sub_commands[1].argv[0], cmd->sub_commands[1].argv);
    }
    else {
        close(fd[0]);
        close(fd[1]);
        
        int i;
        for(i = 0; i <2; i++){
            int child_pid = wait(NULL);
        }
    }
}
}
*/
