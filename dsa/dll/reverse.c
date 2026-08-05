#include <stdio.h>
#include <stdlib.h>

typedef struct dll
{
   int data;
   struct dll *prev;
   struct dll *next;
}dll_t;

typedef struct node
{
  struct dll *head;
  struct dll *last;
}node_t;


void insert(node_t *d_list,int i)
{
   struct dll *current=(struct dll*)malloc(sizeof(struct dll));
   
   current->data =i;
   current->prev =NULL;
   current->next =NULL;
   
   if(d_list->head == NULL)
   {
      d_list->head = d_list->last = current;
      return;
   }
   else
   {
        current->prev=d_list->last;
        d_list->last->next=current;
        d_list->last = current; 
       
        return;
   } 
   
}

void reverse_nonrelfect(node_t *d_list)
{
	
	struct dll *tmp=d_list->head;
    	struct dll *dup=NULL,*rev=NULL;
    	struct dll *rotation;//=(struct dll*)malloc(sizeof(struct dll));
	   while(tmp != NULL)
	   {
	   	struct dll *rotation=(struct dll*)malloc(sizeof(struct dll));
	   	//printf("d_list rotation head = %p:%d--%p:%p\n",tmp->prev,tmp->data,tmp,tmp->next);
	   	  	 
	   	 rotation->data=tmp->data;
	   	  rotation->prev=NULL;
	   	  rotation->next=rev;
	   	  if(rev !=NULL)
	   	  {
	   	  rev->prev=rotation;
	   	  }
	   	  rev=rotation;
	   	  
	    	 tmp=tmp->next;
	    	 printf("d_list rotation check = %p:%d--%p:%p\n",rotation->prev,rotation->data,rotation,rotation->next); 
	    	 //printf("\n");
	   }
    	printf("reverse list\n");
    	
    	struct dll *rotation1=rev;
     while(rotation1 != NULL)
   {
   	printf("d_list rotation next = %p:%d--%p:%p\n",rotation1->prev,rotation1->data,rotation1,rotation1->next); 
   	 rotation1=rotation1->next;
   }	
}



void reverse(node_t *d_list)
{
    	struct dll *tmp=d_list->head;
    	struct dll *dup,*rev=NULL;
    	struct dll *tmp1=tmp;
    	
   while(tmp1 != NULL)
   {
   	printf("d_list rev head = %p:%d--%p:%p\n",tmp1->prev,tmp1->data,tmp1,tmp1->next); 
   	 
   	 dup=tmp1->next;
   	 tmp1->next = rev;
   	 rev=tmp1;
   	 rev->prev=dup;
    	 tmp1=dup;
   }
   d_list->last=d_list->head;
   d_list->head=rev;
    	printf("reverse list\n");
     while(rev != NULL)
   {
   	printf("d_list rev list = %p:%d--%p:%p\n",rev->prev,rev->data,rev,rev->next); 
   	 
   	 rev=rev->next;
   }	
    	
}

void sumof_sorts_list(node_t *d_list)
{
   struct dll *tmp=d_list->head;
   int count=0;
   while(tmp != NULL)
   {
   	count++; 
    	 tmp = tmp->next;
   }
   printf("count=%d\n",count);
	   
   tmp=d_list->head;
    
    while(tmp != NULL)
   {
   	 struct dll *rev=tmp->next;
   	 while(rev !=NULL)
   	 {
   	 if(count == tmp->data+rev->data)
   	 {
   	   printf("tmp=%d \trev=%d\n",tmp->data,rev->data);
   	 }
   	   rev=rev->next;
   	   //return;
   	 }
   	 
    	 tmp = tmp->next;
   }
	
}

void rotation_list(node_t *d_list)
{
 	struct dll *tmp=d_list->head;
   
     
        d_list->head=d_list->head->next;
 	
 	tmp->prev=d_list->last;
 	d_list->last->next = tmp;
 	d_list->last = tmp;
 	d_list->last->next = NULL;
 	
 	d_list->head->prev=NULL;
 	
 	
 	//printf("d_list=%p:%p\n",d_list->head->prev,current->prev);
 	printf("\n\n");
}

void free_ptr(node_t *d_list)
{
	struct dll *tmp=d_list->head;
   while(tmp != NULL)
   {
   	printf("d_list head = %p:%d--%p:%p\n",tmp->prev,tmp->data,tmp,tmp->next); 
    	 tmp = tmp->next;
   }
   
   printf("\n");
   tmp=d_list->last ;
   while(tmp!= NULL)
   {
   	printf("d_list prev = %p:%d--%p:%p\n",tmp->prev,tmp->data,tmp,tmp->next); 
    	 tmp = tmp->prev;
   }
}

int main()
{
   node_t *d_list = (struct node*)malloc(sizeof(struct node));
   d_list->head = NULL;
   d_list->last = NULL;
   
   if(d_list == NULL)
   {
   	printf("d_list memory alloction failed");
   	return -1;
   }
   
   for(int  i=0;i<5;i++)
   {
     insert(d_list,i);
   }
   insert(d_list,6);
   insert(d_list,7);
   insert(d_list,9);
   
   
    free_ptr(d_list);
   //reverse_nonrelfect(d_list);
   
   //reverse(d_list);
   
   //sumof_sorts_list(d_list);
   
   rotation_list(d_list);
   
   free_ptr(d_list);
   
    struct dll *current = d_list->head;
    while (current != NULL) {
        struct dll *temp = current;
        current = current->next;
        free(temp);
        }
   
    // Free the wrapper struct as well


   
   return 0;
}

