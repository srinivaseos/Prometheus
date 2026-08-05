#include <stdio.h>



void func(int arr[],int size)
{
    for(int i=0;i<size;i++)
    {
        for(int j=0;j<size-i-1;j++)
        {
            if(arr[j] > arr[j+1])
            {
                int data=arr[j];
                arr[j]=arr[j+1];
                arr[j+1] = data;
            }
        }
    }
}

int main()
{
    int arr[]={ 5, 6, 1, 3 };
    int size=sizeof(arr)/sizeof(arr[0]);
    func(arr,size);
    for(int i=0;i<size;i++)
    {
        printf("%d ",arr[i]);
    }
}