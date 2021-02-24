//Author: Caleb Riese
//Date: 2/19/2021

#include <stdio.h>
#include <getopt.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
//void countNonBlankLines(FILE * inputFile, char *myError)

int countNonBlankLines(FILE * inputFile)
{
    int lineCount = 0;
    char character;
    char lastCharacter = '\n';
    while (character = fgetc(inputFile), character != EOF)
    {
        if (character == '\n' && lastCharacter != '\n')
            lineCount++;
        lastCharacter = character;
    }
    if (lastCharacter != '\n')
        lineCount++;
    return lineCount;
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
                printf("Usage: master [-h] [-s i] [-t time] datafile\n");
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

    FILE * inputFile;
    char filename[128];
    strcpy(filename, argv[1]);
    inputFile = fopen(filename, "r");
    if (inputFile == NULL)
    {
        errno = ENOENT;
        perror(myError);
        return 1;
    }

    int lineCount = countNonBlankLines(inputFile);




    fclose(inputFile);
    return 0;
}