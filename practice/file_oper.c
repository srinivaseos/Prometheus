#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>


int main()
{
    int ifile = S_IRWXU | S_IRWXG | S_IRWXO;
    int ret =0;

    ret = mkdir("logs",ifile);
    if(ret < 0)
    {
        printf("error create");
    }
    int fd = 
    ret = write(fd,"hello world",11);
    if(ret < 0)
    {
        printf("error create");
    }

    return 0;
}

