#include <stdio.h>
#include <stdlib.h>


typedef struct link
{
	int data;
	struct link *next;
}s_link;

s_link *head=NULL;
static int count=0;
void insert(int num)
{
   s_link *newnode=(struct link*)malloc(sizeof(struct link));
   
   if(newnode ==NULL)
   {
      printf("newnode alloction failed%s:%d\n",__FILE__,__LINE__);
      exit(-1);
   }
    newnode->data = num;
    newnode->next = NULL;
    if(head != NULL)
    {
      newnode->next =head;
      
    }
	head = newnode;
	count++;
}

void printlist()
{
    s_link *temp=head;
    
    while(temp !=NULL)
    {
    	printf("%d ",temp->data);
       temp = temp->next ;
    }
    printf("count=%d\n",count);
}

void freelist()
{
    s_link *temp;
    while (head != NULL)
    {
        temp = head;
        head = head->next;
        free(temp);
    }
}

void delete(int pos)
{
     s_link *temp=head;
     int t=1;
    if(pos < count)
    {
	while(temp !=NULL)
	{
		if(pos-1 == t)
		{
			s_link *temp1 = temp->next;
			 
			printf("delete %d:%d\n ",temp->data,temp1->data);
			temp->next = temp1->next;
			free(temp1);
			count--;
			return;
		}
	  temp = temp->next;
	  t++;
	}
     }
}

void reverselist()
{
	s_link *temp=head;
	s_link *temp1=NULL,*temp2;
    while (temp->next != NULL)
    {
    	temp->next=temp1;
    	temp1=temp;
        temp = temp->next;
    }
    	temp1->next=NULL;
	//head=temp;
	
	//temp2=head;
    
    while(temp1 !=NULL)
    {
    	printf("loop=%d ",temp1->data);
       temp1 = temp1->next ;
    }
    printf("\n");
}

int recuresive(int lock)
{
s_link *temp;
  if(lock == 1)
  {
    s_link *temp=head;
  }
  if (temp == NULL)
    {
    	return 0;
    }
    temp=temp->next;
    recuresive(2);
    
    printf("ec=%d ",temp->data);

}
int main()
{
	
  insert(1);
  insert(2);
  insert(3);
  insert(8);
  insert(7);
  insert(6);
  
  printlist();
  delete(2);
  reverselist();
recuresive(1);
 printlist();  
 freelist();
  head=NULL; 

}
