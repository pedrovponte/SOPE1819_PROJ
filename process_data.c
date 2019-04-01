// Program that processes file data

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <dirent.h>

#include "forensic.h"

int process_data(fore_args file_arguments)
{
    if (file_arguments.arg_h)
    {
        if (file_arguments.h_args[0] == NULL)
        {
            printf("-h flag requires arguments!!!\n");
            exit(1);
        }
    }

    if (file_arguments.arg_o)
    {
        if (file_arguments.outfile == NULL)
        {
            printf("-v flag requires an argument!!!\n");
            exit(1);
        }
    }


    //Read File Type
    char* file_name = malloc(255*sizeof(char));
    sprintf(file_name, "%s.txt", file_arguments.f_or_dir);
    for(size_t i = 0; i < strlen(file_name); i++){
        if(file_name[i] == '/')
            file_name[i] = '_';
    }

    int fd1 = open(file_name, O_RDWR | O_CREAT, 0777);
    if (fd1 < 0)
    {
        perror("open");
        exit(-1);
    }

    FILE *fp = fdopen(fd1, "r");
    if (fp == NULL)
    {
        perror("fdopen");
        exit(-1);
    }

    char *file_string = malloc(255 * sizeof(char));
    sprintf(file_string, "file %s > %s", file_arguments.f_or_dir, file_name);
    system(file_string);
    memset(file_string, '\0', sizeof(file_string) * sizeof(char));
    fgets(file_string, 255, fp);
    fclose(fp);
    close(fd1);

    char *file_string_result = strstr(file_string, " ") + 1;

    size_t file_string_len = strlen(file_string_result);
    file_string_result[file_string_len - 1] = '\0';

    //Read File Data
    char *path_string = malloc(255 * sizeof(char));
    struct stat statbuf;
    sprintf(path_string, "%s", file_arguments.f_or_dir);
    if (stat(path_string, &statbuf) < 0)
    {
        exit(1);
    }
    free(path_string);

    char file_access_owner[3];
    time_t file_modification_date;
    time_t file_access_date;
    int file_size = 0;
    for (unsigned int i = 0; i < 3; i++)
    {
        file_access_owner[i] = '\0';
    }

    file_size = statbuf.st_size;

    if (statbuf.st_mode & S_IRUSR)
    {
        file_access_owner[0] = 'r';
    }

    if (statbuf.st_mode & S_IWUSR)
    {
        if (file_access_owner[0] == '\0')
        {
            file_access_owner[0] = 'w';
        }
        else
        {
            file_access_owner[1] = 'w';
        }
    }

    if (statbuf.st_mode & S_IXUSR)
    {
        if (file_access_owner[0] == '\0')
        {
            file_access_owner[0] = 'x';
        }
        else if (file_access_owner[1] == '\0')
        {
            file_access_owner[1] = 'x';
        }
        else
        {
            file_access_owner[2] = 'x';
        }
    }

    file_access_date = statbuf.st_atime;
    file_modification_date = statbuf.st_mtime;

    struct tm *access_info;
    struct tm *modification_info;
    char accessDate[20];
    char modificationDate[20];
    access_info = localtime(&file_access_date);
    modification_info = localtime(&file_modification_date);

    strftime(accessDate, 20, "%Y-%m-%dT%H:%M:%S", access_info);
    strftime(modificationDate, 20, "%Y-%m-%dT%H:%M:%S", modification_info);

    char *info_to_write = malloc(1000 * sizeof(char));

    sprintf(info_to_write, "%s,%s,%d,%s,%s,%s", file_arguments.f_or_dir, file_string_result, file_size, file_access_owner, accessDate, modificationDate);

    //Calculate file fingerprints
    if (file_arguments.arg_h)
    {
        for (unsigned int i = 0; i < 3; i++)
        {
            if (file_arguments.h_args[i] != NULL)
            {
                fd1 = open(file_name, O_RDWR, 0777);
                fp = fdopen(fd1, "r");
                char *h_string = malloc(255 * sizeof(char));
                char *tmp_string = malloc(25 * sizeof(char));
                sprintf(h_string, "%ssum %s > %s", file_arguments.h_args[i], file_arguments.f_or_dir, file_name);
                system(h_string);
                free(h_string);
                h_string = malloc(255 * sizeof(char));
                fgets(h_string, 255, fp);
                sscanf(h_string, "%s %s", h_string, tmp_string);
                sprintf(info_to_write + strlen(info_to_write), ",%s", h_string);
                free(h_string);
                free(tmp_string);
                fclose(fp);
                close(fd1);
            }
        }
    }

    sprintf(info_to_write + strlen(info_to_write), "\n");

    if (file_arguments.arg_o)
    {
        int fd_o = open(file_arguments.outfile, O_RDWR | O_CREAT | O_APPEND, 0777);
        write(fd_o, info_to_write, strlen(info_to_write));
    }
    else
        write(STDOUT_FILENO, info_to_write, strlen(info_to_write));

    size_t fs = sizeof(file_string);
    memset(file_string, '\0', fs);

    sprintf(file_string, "rm %s", file_name);

    system(file_string);

    free(info_to_write);
    free(file_string);
    free(file_name);

    return 0;
}
