#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main()
{
    int iFileCreationMode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    int sts = mkdir("logs", iFileCreationMode);
    
    if(sts == 0 )
    {
        printf("sts=%d\n",sts);
    }
    else
    {
         printf("sts=%d\n",sts);
    }

    int rts = creat("logs/pfcp.log",iFileCreationMode);
     if(rts == 0 )
    {
        printf("rts=%d\n",rts);
    }
    else
    {
         printf("rts=%d\n",rts);
    }

    printf("Hello World");

    return 0;
}
