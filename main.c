//Author: Caleb Riese
//Date: 2/21/2021

#include <stdio.h>
#include <getopt.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>


void processArgs(int argc, char **argv, char myError[])
{

}

int main(int argc, char * argv[])
{
    int opt;
    char myError[256] = "";
    strcat(myError, argv[0]);
    strcat(myError, ": Error");

    if (argc == 1)
    {
        printf("No Arguments\n");
    }
    while ((opt = getopt (argc, argv, ":hst")) != -1)
    {
        switch (opt)
        {
            case 'h':
                printf("Usage: master [-i] [name=value ...] [utility [argument ...]]\n");
                return 0;
            case 's':
                printf("s\n");
                break;
            case 't':
                printf("t\n");
                break;
            case '?':
                printf("Unknown\n");
                errno = EINVAL;
                perror(myError);
                return 1;
        }
    }
    printf("blah\n");
    FILE *fp;
    int count = 0;  // Line counter (result)
    char filename[256];
    char c;  // To store a character read from file
    strcpy(filename, argv[1]);
    fp = fopen(filename, "r");

    // Check if file exists
    if (fp == NULL)
    {
        printf("Could not open file %s", filename);
        return 0;
    } else {
        count++;
    }

    while(!feof(fp))
    {
        c = fgetc(fp);
        if(c == '\n')
        {
            count++;
        }
    }

    // Close the file
    fclose(fp);
    printf("The file %s has %d lines\n ", filename, count);
    //processArgs(argc, argv, myError);
    return 0;
}