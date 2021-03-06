//Author: Caleb Riese
//Date: 2/19/2021

#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <getopt.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>
#include <signal.h>
#include <math.h>
#include <sys/wait.h>
#include <unistd.h>


int shmidOne;
int shmidTwo;
int shmidThree;
int * sharedIntArray;
int * sharedPIDArray;
int * sharedFlagAndTurn;
time_t startTime;
struct tm * myTime;
FILE * outfile;

void sig_handler(int sig)//my signal handler that deletes shared memory and kills children
{
    printf("Program Terminating...\n");
    outfile = fopen( "output.log", "a");
    startTime = time(NULL);
    myTime = localtime(&startTime);
    fprintf(outfile, "Time:%d:%d:%d Terminating...\n",myTime->tm_hour,myTime->tm_min,myTime->tm_sec);
    fclose(outfile);
    kill(0,SIGTERM);//sends SIGTERM to the all processes with same group id, so all children
    int status;
    while (wait(&status) != -1){} //get rid of any leftover zombies
    shmdt(sharedIntArray);//detach and destroy memory
    shmdt(sharedPIDArray);
    shmdt(sharedFlagAndTurn);
    shmctl(shmidOne, IPC_RMID, NULL);
    shmctl(shmidTwo, IPC_RMID, NULL);
    shmctl(shmidThree, IPC_RMID, NULL);
    _exit(0);
}
void ignore_handler(int sig)//ignores the SIGTERM sent to the PGID
{
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
    //sets up signal handlers
    signal(SIGINT,sig_handler);
    signal(SIGALRM,sig_handler);
    signal(SIGTERM,ignore_handler);

    int opt;
    //my custom error message
    char myError[256] = "";
    strcat(myError, argv[0]);
    strcat(myError, ": Error");
    //default values
    int maxTime = 100;
    int maxChildren = 19;

    //handles all arguments/parameters
    if (argc == 1)
    {
        printf("Usage: master [-h] [-s i] [-t time] datafile\n");
        exit(EXIT_FAILURE);
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
                maxChildren = atoi(optarg) - 1; //minus the parent process
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
    if (maxChildren > 19)
    {
        printf("Max Processes Allowed is 20, set to 20.\n");
        maxChildren = 19; //Max allowed is 20 processes: 1 Parent + 19 Children
    }
    else if (maxChildren <= 0)
    {
        printf("At least one child is required, set to 1 child.\n");
        maxChildren = 1;
    }
    else if (maxTime <= 0) // cant set time to 0
    {
        printf("Time must be greater than zero, set to default.\n");
        maxTime = 100;
    }
    alarm(maxTime);//sets the timer that sends SIGALARM

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
    if ((logOfLines - (int)logOfLines) != 0.0)//if its not 2^k then find how many its missing
    {
        lineCount = (int)pow(2,ceil(log2(lineCount)));
    }
    int index = 0;
    int integerArray[lineCount];
    char holder[10] = {};
    while (fgets(holder, 10, inputFile))//get integers from file
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
    key_t keyOne = ftok("README", 1);
    key_t keyTwo = ftok("Makefile",1);
    key_t keyThree = ftok("datafile",1);

    shmidOne = shmget(keyOne, sizeof(integerArray), 0666 | IPC_CREAT);//these are rounded up to page size apparently
    shmidTwo = shmget(keyTwo, sizeof(integerArray)/2, 0666 | IPC_CREAT);
    shmidThree = shmget(keyThree, sizeof(int)*2, 0666 | IPC_CREAT);

    //i found it easier to read using different shared memory segments than trying to use one
    //i chose readability over using less code
    sharedIntArray = (int*) shmat(shmidOne, (void*)0, 0);
    sharedPIDArray = (int*) shmat(shmidTwo, (void*)0, 0);
    sharedFlagAndTurn = (int*) shmat(shmidThree, (void*)0, 0);

    //puts the integers into shared memory
    for (int i = 0; i < lineCount; i++)
    {
        sharedIntArray[i] = integerArray[i];
    }
    //sets all PID's to idle (flag[n])
    for (int i = 0; i < ((lineCount/2)); i++)
    {
        sharedPIDArray[i] = 0;
    }


    //open output.log
    outfile = fopen( "output.log", "a");
    //Sum Of Integers/Child Processes Code
    int depth = 1;
    int status;
    while (lineCount > 1) //while the sum is not found
    {
        int activeChildren = 0;
        int childrenInDepth = lineCount / 2;
        sharedFlagAndTurn[1] = childrenInDepth; //n for flag[n]
        for(int i = 0; i < childrenInDepth; i++)//create each child and execute
        {
            if (activeChildren < maxChildren)
            {
                activeChildren++; //before fork so parent does it and not child
                pid_t pid = fork();
                if (pid == -1)//fork() error
                {
                    perror(myError);//fork() sets errno itself
                    exit(EXIT_FAILURE);
                }
                else if (pid == 0)//Child Process
                {
                    char iString[64];
                    char depthString[64];
                    sprintf(iString, "%d", i);
                    sprintf(depthString, "%d", depth);
                    char * args[] = {"./bin_adder", iString, depthString, NULL};
                    execvp(args[0], args);
                    exit(EXIT_FAILURE);//In case exec fails this keeps the child from continuing, otherwise its ignored
                }
            } else {
                //if max child is reached wait for one to end
                pid_t temp;
                temp = wait(&status);
                startTime = time(NULL);
                myTime = localtime(&startTime);
                fprintf(outfile, "Time:%d:%d:%d Child Terminated PID:%d\n",myTime->tm_hour,myTime->tm_min,myTime->tm_sec,temp);
                fflush(outfile); //in case program is terminated early
                activeChildren--;
                i--;
            }
        }
        while (activeChildren > 0)//Parent waits for all children before next depth starts
        {
            pid_t temp;
            temp = wait(&status);
            startTime = time(NULL);
            myTime = localtime(&startTime);
            fprintf(outfile, "Time:%d:%d:%d Child Terminated PID:%d\n",myTime->tm_hour,myTime->tm_min,myTime->tm_sec,temp);
            fflush(outfile); //in case program is terminated early
            activeChildren--;
        }
        lineCount = lineCount/2;
        depth++;
    }

    //gets time
    startTime = time(NULL);
    myTime = localtime(&startTime);
    //writes final sum and time of completion
    printf("Final Sum: %d\n", sharedIntArray[0]);
    fprintf(outfile, "Time:%d:%d:%d  Final Sum: %d\n", myTime->tm_hour, myTime->tm_min, myTime->tm_sec, sharedIntArray[0]);

    //close all files and destroy shared memory
    fclose(outfile);
    shmdt(sharedIntArray);
    shmdt(sharedPIDArray);
    shmdt(sharedFlagAndTurn);
    shmctl(shmidOne, IPC_RMID, NULL);
    shmctl(shmidTwo, IPC_RMID, NULL);
    shmctl(shmidThree, IPC_RMID, NULL);
    return 0;
}