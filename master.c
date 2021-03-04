//Author: Caleb Riese
//Date: 2/19/2021

#include <stdio.h>
#include <getopt.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>
#include <signal.h>
#include <math.h>
#include <sys/wait.h>
#include <unistd.h>


void sig_handler(int sig)//called for ctrl c and alarm
{
    printf("Handler Function\n");
    //exit()? gets rid of shared memory
    //Ctrl-c and free up the shared memory, send a kill signal to the child and then terminate itself.
}

int countNonBlankLines(FILE * inputFile) //counts the lines in file that arent blank
{
    int lineCount = 0;
    char line[10];
    while (fgets(line, 10, inputFile))
    {
        if (line[0] != '\n') //if the line is not blank, then line count goes up
        {
            lineCount++;
        }
    }
    rewind(inputFile);
    return lineCount;
}

void checkArgument(char * input, char * myError) //checks if parameter is a digit
{
    for (int i = 0; i < strlen(input); i++)
    {
        if (!isdigit(input[i]))
        {
            errno = EINVAL;
            perror(myError);
            exit(0);
        }
    }
}

int main(int argc, char * argv[])
{

    signal(SIGINT,sig_handler); //Ctrl-C
    signal(SIGALRM,sig_handler); //alarm()
    //signal(SIGCHLD,sig_handler); //Child process finishes

    int opt;
    char myError[256] = "";
    strcat(myError, argv[0]);
    strcat(myError, ": Error");
    int maxTime = 100;
    int maxChildren = 20;

    if (argc == 1)
    {
        printf("No Arguments\n");
    }
    while ((opt = getopt (argc, argv, ":hs:t:")) != -1)
    {
        switch (opt)
        {
            case 'h':
                printf("Usage: master [-h] [-s i] [-t time] datafile\n");
                return 0;
            case 's':
                checkArgument(optarg,myError);
                maxChildren = atoi(optarg);
                printf("Max Children: %d\n",maxChildren);
                break;
            case 't':
                checkArgument(optarg,myError);
                maxTime = atoi(optarg);
                printf("Max Time: %d\n",maxTime);
                break;
            case '?':
                printf("Unknown Option\n");
                errno = EINVAL;
                perror(myError);
                exit(EXIT_FAILURE);
        }
    }
    alarm(maxTime);


    //Opens the file
    FILE * inputFile;
    inputFile = fopen(argv[argc - 1], "r");
    if (inputFile == NULL)
    {
        errno = ENOENT;
        perror(myError);
        exit(EXIT_FAILURE);
    }


    //Parses the file to an integer array and fills in zeros
    int lineCount = countNonBlankLines(inputFile);
    double logOfLines = log2(lineCount);
    if ((logOfLines - (int)logOfLines) != 0.0)
    {
        lineCount = (int)pow(2,ceil(log2(lineCount)));
    }
    int index = 0;
    int integerArray[lineCount];
    char holder[10] = {};
    while (fgets(holder, 10, inputFile))
    {
        integerArray[index] = atoi(holder);
        index++;
    }
    fclose(inputFile);
    while (index < lineCount)//fill remaining index's with 0
    {
        integerArray[index] = 0;
        index++;
    }





    //Shared Memory Code
    key_t key = ftok("README",1); //CHANGE WHEN ON HOARE
    printf("parent key:%d\n", key); //REMOVE ME AFTER TESTING ON HOARE!!!!!!!
    int shmid = shmget(key,sizeof(integerArray),0666|IPC_CREAT);
    int * sharedMemory = (int*) shmat(shmid, (void*)0, 0);
    for (int i = 0; i < lineCount; i++)
    {
        sharedMemory[i] = integerArray[i];
    }


    //Sum Of Integers/Child Processes Code
    int depth = 1;
    int status;
    while (lineCount > 1)
    {
        int numberOfProcesses = lineCount/2;
        for(int i = 0; i < numberOfProcesses; i++)
        {
            pid_t pid = fork();
            if (pid == -1)//fork() error
            {
                perror(myError);//fork() sets errno itself
                exit(EXIT_FAILURE);
            }
            else if (pid == 0)//Child Process
            {
                char iString[64];
                sprintf(iString, "%d", i);
                char depthString[64];
                sprintf(depthString, "%d", depth);
                char * args[] = {"./BIN_ADDER", iString, depthString, NULL};

                execvp(args[0], args);
                exit(EXIT_FAILURE);//In case exec fails this keeps the child from continuing, otherwise its ignored
            }
        }
        for (int i = 0; i < numberOfProcesses; i++)//Parent waits for all children before next depth starts
        {
            pid_t pid = wait(&status);
            printf("Process %i finished\n", pid);
//            1. WIFEXITED(status): child exited normally
//            • WEXITSTATUS(status): return code when child exits
//
//            2. WIFSIGNALED(status): child exited because a signal was not caught
//            • WTERMSIG(status): gives the number of the terminating signal
//
//            3. WIFSTOPPED(status): child is stopped
//            • WSTOPSIG(status): gives the number of the stop signal
        }

        lineCount = lineCount/2;
        depth++;
    }

    printf("Final Sum: %d\n", sharedMemory[0]);
    shmdt(sharedMemory);
    shmctl(shmid,IPC_RMID,NULL); //should only be done by ONE process after all are done
    return 0;
}