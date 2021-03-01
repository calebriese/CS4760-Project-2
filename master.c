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
#include <math.h>
//void countNonBlankLines(FILE * inputFile, char *myError)
//fork, exec, wait, exit, shmget, shmctl, shmat, shmdt

int countNonBlankLines(FILE * inputFile) //counts lines in file that arent blank
{
    int lineCount = 0;
    char line[5];
    while (fgets(line, 5, inputFile))
    {
        if (line[0] != '\n') //if the line is not blank, then line count goes up
        {
            lineCount++;
        }
    }
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

int readIndexFromString(char * input, int index, int lineCount)
{
    int count = 0;
    int i = 0;
    char holder[5];
    int letter = 0;
    while (count < lineCount)
    {
        if (input[i] == 10) //10 is newline, 0 is \0
        {
            count++;
        }
        if (input[i] != 10 && input[i] != 0 && count == index)
        {
            holder[letter] = input[i];
            letter++;
        }
        i++;
    }
    return atoi(holder);
}

//void readArrayFromMem(char * input, int lineCount)
//{
//
//    int count = 0;
//    int i = 0;
//    char holder[5];
//    int letter = 0;
//    while (count < lineCount)
//    {
//        if (input[i] == 10) //10 is newline, 0 is \0
//        {
//            count++;
//        }
//        if (input[i] != 10 && input[i] != 0 && count == index)
//        {
//            holder[letter] = input[i];
//            letter++;
//        }
//        i++;
//    }
//    return atoi(holder);
//}

int overwriteSHM(char * input, int assignedIndex, int blankIndex, int lineCount, char stringArray[256][5], int sum)
{
    sprintf(stringArray[assignedIndex], "%d", sum);
    for (int j = 0; j < 5; j++)
    {
        stringArray[assignedIndex+1][j] = '\0';
    }



    //char blank[1024] = {};
    //strcpy(input,blank);
    for (int i = 0; i < lineCount; i++)
    {
        strcat(input,stringArray[i]);
    }
    strcat(input,"\n");
}

int main(int argc, char * argv[])
{
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
                printf("children allowed: %d\n",maxChildren);
                break;
            case 't':
                checkArgument(optarg,myError);
                maxTime = atoi(optarg);
                printf("time allowed: %d\n",maxTime);
                break;
            case '?':
                printf("Unknown Option\n");
                errno = EINVAL;
                perror(myError);
                return 1;
        }
    }

    FILE * inputFile;
    char filename[128];
    strcpy(filename, argv[argc - 1]); //change to last index, I THINK I DID ALREADY
    inputFile = fopen(filename, "r");
    if (inputFile == NULL)
    {
        errno = ENOENT;
        perror(myError);
        return 1;
    }


    int lineCount = countNonBlankLines(inputFile);
    char stringArray[256][5]; //not sure how dynamically to linecount
    int index = 0;
    rewind(inputFile);
    int integerArray[lineCount];
    char holder[5] = {};
    while (fgets(holder, 5, inputFile)) //5 bytes 256\n\0 null and terminating
    {
        integerArray[index] = atoi(holder);
        index++;
    }


    key_t keyI = ftok("shmfileI",65);
    int shmidI = shmget(keyI,512,0666|IPC_CREAT);
    int * sharedIntegerArray = (int*) shmat(shmidI,(void*)0,0);
    if (fork() == 0)
    {
        for (int i = 0; i < lineCount; i++)
        {
            sharedIntegerArray[i] = integerArray[i];
        }
        exit(0);
    }
    wait(1);
    for (int i = 0; i < lineCount; i++)
    {
        printf("Shared: %d\n",sharedIntegerArray[i]);
    }


    key_t key = ftok("shmfile",65);
    int shmid = shmget(key,1024,0666|IPC_CREAT);
    char * str = (char*) shmat(shmid,(void*)0,0);
    for (int i = 0; i < lineCount; i++)
    {
        strcat(str,stringArray[i]);
    }
    strcat(str,"\n");



    int level = 1;
    int origLineCount = lineCount;
    while (lineCount > 1)
    {
        printf("\nlevel:%d\n",level);
        int numberOfProcesses = lineCount/2;
        for(int i = 0; i < numberOfProcesses; i++)
        {
            if (fork() == 0)
            {
                int sum = 0;
                int firstIndex = i * pow(2,level);
                int secondIndex = firstIndex + pow(2,(level-1));
                sum += readIndexFromString(str,firstIndex,lineCount);
                sum += readIndexFromString(str,secondIndex,lineCount);
                overwriteSHM(str, firstIndex, secondIndex, origLineCount, stringArray, sum); //giveOriginal lineCount
                printf("level:%d , i:%d , index:%d , Sum:%d\n",level, i, firstIndex, sum);
                printf("level:%d , i:%d , index:%d , Sum:%d\n",level, i, secondIndex, sum);
                exit(0);
            }
        }
        int status;
        for (int i = 0; i < numberOfProcesses ; ++i)
            wait(&status);
        lineCount = lineCount/2;
        level++;
    }

    printf("Parent:\n%s\nEND",str);





    shmdt(str);
    sleep(2);
    shmctl(shmid,IPC_RMID,NULL); //should only be done by ONE process after all are done
    shmctl(shmidI,IPC_RMID,NULL);
    fclose(inputFile);
    //printf("\nEnd");
    return 0;
}