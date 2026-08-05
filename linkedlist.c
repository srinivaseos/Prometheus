#include <stdio.h>
#include <stdlib.h>
#include <string.h>


struct node
{
	int id;
	struct node *next;
};

struct node * head;
struct node *current;

void insert(int data)
{
  printf("%d\n",data);
  
  if(head == NULL)
  {
    head->id = data;
  }
  else
  {
  	current->id = data;
  	current->next = head;
  }
  
}
int main()
{

   int num;
   struct node * head = (struct node *)malloc(sizeof(struct node *));
   printf("enter  number:");
   scanf("%d",&num);
   
   insert(num);
   
   
   return 0;
}
