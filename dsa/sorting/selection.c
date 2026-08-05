#include <stdio.h>
#include <stdlib.h>

int compare(const void *a,const void *b)
{
    return *(int *)a- *(int *)b;
}

void func(int arr[],int size)
{
    qsort(arr,size,sizeof(int),compare);
}

int main()
{
    int arr[]={ 5, 6, 3, 1 };
    int size=sizeof(arr)/sizeof(arr[0]);
    func(arr,size);
    for(int i=0;i<size;i++)
    {
        printf("%d ",arr[i]);
    }
    printf("\n");
}
