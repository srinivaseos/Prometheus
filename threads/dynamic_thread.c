#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

int thread = 0;

void *long_run(void *arg)
{
    int *limit_t = (int*)arg;
    int limit = *limit_t;
    
    static int sum = 0;

    printf("limit=%d\n", limit);

    for(int i = 0; i < limit; i++)
    {
        sum += i;
    }

    printf("sum-%d=%d\n", thread, sum);
    thread++;
    int *answer=malloc(sizeof(*answer));
	*answer=sum;
    pthread_exit(answer);
}

int main(int argc, char *argv[])
{
    printf("usage count: %d\n", argc);

    if(argc < 3)
    {
        printf("less usage: %s\n", argv[0]);
        exit(-1);
    }

    for(int i = 0; i < argc; i++)
    {
        printf("usage-%d: %s\n", i, argv[i]);
    }
	int argu = atoi(argv[1]);
	printf("usage argu: %d\n", argu);
   // int *limit = malloc(sizeof(*limit));
    int limit=atoi(argv[2]);
	
    pthread_t id[argu];
    int *value[argu]; 
	printf("usage *argv[1]: %d\n", *argv[1]-'0');
    for(int i = 0; i < *argv[1]-'0'; i++)
    {
        pthread_create(&id[i], NULL, long_run, &limit);
        pthread_join(id[i], (void**)&value[i]);

        printf("result=%d:%d\n", i,*value[i]);
        free(value[i]);
    }
   
    //free(value);
    //value = NULL;
    return 0;
}
