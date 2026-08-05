#include <stdio.h>
#include <stdlib.h>

#define MAX 6

int stack[MAX];

int top=0;

/* Check if the stack is full*/
int isfull(){
   if(top == MAX)
      return 1;
   else
      return 0;
}

void push(int data)
{
	if(isfull() == 1)
	{
	printf("is full list\n");
	   return ;
	}
	if(top < MAX)
	{
	printf("data=%d:top=%d\n",data,top);
	   stack[top++]=data;
	}
}

void pop()
{
	if(isfull()== 1)
	{
	printf("is full list\n");
	   return ;
	}
	
	while(top-1 >= 0)
	{
	printf("pop=data=%d:top=%d\n",stack[top-1],top-1);
	top--;
	}
	
}


void display()
{
  if(isfull()== 0)
	{
	printf("is empty list\n");
	   return ;
	}
  int list = top-1;
  while (list > -1 )
  {
  printf("%d:%d\n",stack[list],list);
  list--;
  }
}

int main()
{
   push(5);
   push(6);
   push(3);
   push(2);
   push(1);
	
  display();
  //push(8);
  
  pop();
  display();

}
