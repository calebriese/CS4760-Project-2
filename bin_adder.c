#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <sys/shm.h>

int main(int argc, char * argv[])
{

//    struct tm * myTime = localtime(&startTime);
//    printf("Time %d:%d:%d",myTime->tm_hour,myTime->tm_min,myTime->tm_sec);
    //Start Time
//    time_t startTime, currentTime, timeElapsed;
//    time(&startTime);
//        time(&currentTime);
//        timeElapsed = currentTime - startTime;
//        if (timeElapsed >= maxTime)
//        {
//            printf("\nMax Time was reached: %ld\n", timeElapsed);
//        }


    //Parsing Arguments
    int i = atoi(argv[1]);
    int depth = atoi(argv[2]);


    //Shared Memory Code
    key_t key = ftok("README",1);
    int shmid = shmget(key,0,0);//size and IPC_CREAT doesn't need to be defined here since we're not creating
    int * sharedMemory = (int*) shmat(shmid, (void*)0, 0);

    printf("key:%d\n", key);

    int sum = 0;
    int firstIndex = i * pow(2, depth);
    int secondIndex = firstIndex + pow(2,(depth - 1));
    sum += sharedMemory[firstIndex];
    sum += sharedMemory[secondIndex];
    sharedMemory[firstIndex] = sum;
    printf("depth:%d , i:%d , index:%d , Sum:%d\n", depth, i, firstIndex, sum);
    printf("depth:%d , i:%d , index:%d , Sum:%d\n", depth, i, secondIndex, sum);


    shmdt(sharedMemory);
    return 0; //this is the status being returned to wait(&status)
}
