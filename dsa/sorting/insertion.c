#include <stdio.h>



void func(int arr[],int size)
{
    //printf("size=%d\n",size);
    for(int i=1;i<size;i++)
    {
        int k=arr[i];
        int j=i-1;
       printf("%d-%d ",arr[j],k); 
            while(j >= 0 && arr[j] > k )
            {
               
               
                arr[j+1] = arr[j];
                j--;
            }
          arr[j+1]=k;  
        
    }
    printf("\n");
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
    printf("\n");
}
