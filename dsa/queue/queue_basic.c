#include <stdio.h>
#include <stdlib.h>

#define MAX 5

int top=0;

int array[MAX];

int isempty()
{
 if(top == 0)
   return 0;
}

int isfull()
{

  if(top == MAX)
  {printf("top=%d\n",top);
  return 1;
  }
  return 0;
}
void enqueue(int data)
{
   if(isfull())
   {
     printf("array is fulltop=%d:%d\n",top,MAX);
     return;
   }
 
   array[top++]=data;
   
  // top = top+1;
    printf("afterenqueue array=%d:%d\n",array[top],top);
}


void dequeue()
{
    int list=0;
    //while(isempty() !=0)
    while(list < MAX-1)
    {
       list = MAX-top;
      printf("deuqueue=%d:%d==%d\n",array[list],MAX,list);
      top=top-1;
    }

}
void display()
{
	printf("top=%d\n",top);
  if(!isempty())
  {
    printf("display in empty msg\n");
    return;
  }
  int list=0;
  while(list != top)
  //while()
  {
	printf("disaply array=%d:%d\n",array[list],list);
	list=list+1;
   }
}

int main()
{
   enqueue(1);
   dequeue();
   enqueue(2);
   enqueue(3); 
   enqueue(4);
   enqueue(5);
   enqueue(6);
   display();
   
   dequeue();
   //dequeue();
   
   display();
   
   
  } 
   

