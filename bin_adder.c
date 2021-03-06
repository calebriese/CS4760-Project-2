//Author: Caleb Riese
//Date: 2/19/2021

#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>

//Shared memory pointers
int * sharedIntArray;
int * sharedPIDArray;
int * sharedFlagAndTurn;

void sig_handler(int sig)//my signal handler that detaches shared memory and exits
{
    shmdt(sharedIntArray);
    shmdt(sharedPIDArray);
    shmdt(sharedFlagAndTurn);
    _exit(0);
}

int main(int argc, char * argv[])
{
    //sets up signal handler
    signal(SIGTERM,sig_handler);
    signal(SIGINT,sig_handler);

    //gets the time
    time_t startTime = time(NULL);
    struct tm * myTime = localtime(&startTime);


    //Shared Memory Code
    //size and IPC_CREAT doesn't need to be defined here since we're not creating just attaching
    key_t keyOne = ftok("README", 1);
    key_t keyTwo = ftok("Makefile",1);
    key_t keyThree = ftok("datafile",1);
    int shmidOne = shmget(keyOne, 0, 0);
    int shmidTwo = shmget(keyTwo,0,0);
    int shmidThree = shmget(keyThree,0,0);
    sharedIntArray = (int*) shmat(shmidOne, (void*)0, 0);
    sharedPIDArray = (int*) shmat(shmidTwo, (void*)0, 0);
    sharedFlagAndTurn = (int*) shmat(shmidThree, (void*)0, 0);


    //Parsing Arguments from input to variable
    int i = atoi(argv[1]);
    int depth = atoi(argv[2]);

    //calculating the sum of the two index's
    int firstIndex = i * pow(2, depth);
    int secondIndex = firstIndex + pow(2,(depth - 1));
    int firstNumber = sharedIntArray[firstIndex];
    int secondNumber = sharedIntArray[secondIndex];
    int sum = firstNumber + secondNumber;
    sharedIntArray[firstIndex] = sum;




    //Critical Section Code

    //flag[n]: sharedPIDArray[n]
    //turn: sharedFlagAndTurn[0]
    //n = sharedFlagAndTurn[1]
    const int idle = 0;
    const int wantIn = 1;
    const int inCS = 2;

    fprintf(stderr, "%d:%d:%d  Trying to enter critical section.\n",myTime->tm_hour,myTime->tm_min,myTime->tm_sec);
    int j;
    int n = sharedFlagAndTurn[1];
    do{
        sharedPIDArray[i] = wantIn; //set  flag to wantIn
        j = sharedFlagAndTurn[0]; //Set local variable to whoever is inside critical section
        while ( j != i ) //wait for my turn
        {
            j = (sharedPIDArray[j] != idle ) ? sharedFlagAndTurn[0] : (j + 1 ) % n;
        }

        sharedPIDArray[i] = inCS; //set my flag to insideCS

        // Check that no one is in critical section
        for ( j = 0; j < n; j++ )
        {
            if ( ( j != i ) && (sharedPIDArray[j] == inCS ) )
            {
                break;
            }
        }
    } while (( j < n ) || (sharedFlagAndTurn[0] != i && sharedPIDArray[sharedFlagAndTurn[0]] != idle ));
    sharedFlagAndTurn[0] = i;//set turn to my index

    fprintf(stderr, "%d:%d:%d  Inside the critical section.\n",myTime->tm_hour,myTime->tm_min,myTime->tm_sec);
    sleep(1);
    FILE * outfile = fopen( "adder_log", "a");
    fprintf(outfile, "Time:%d:%d:%d  PID:%d  Index:%d  Depth:%d\n",myTime->tm_hour,myTime->tm_min,myTime->tm_sec,getpid(),i,(int)log2(n));
    fprintf(outfile, "%d + %d = %d\n",firstNumber,secondNumber,sum);
    fclose(outfile);


    fprintf(stderr, "%d:%d:%d  Exiting the critical section.\n",myTime->tm_hour,myTime->tm_min,myTime->tm_sec);
    j = (sharedFlagAndTurn[0] + 1) % n;
    while (sharedPIDArray[j] == idle)//find next person that wants in
    {
        j = (j + 1) % n;
    }
    sharedFlagAndTurn[0] = j;//set turn to j whoever is in
    sharedPIDArray[i] = idle;//set my flag to idle


    //detatch from shared memory
    shmdt(sharedIntArray);
    shmdt(sharedPIDArray);
    shmdt(sharedFlagAndTurn);

    return 0; //status being returned to parent
}
