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
  struct dll *m_head;
  struct dll *m_last;
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




void merge_sorts_list(node_t *d_list,node_t *d_list1)
{
    struct dll *tmp=d_list->head;
    struct dll *rev=d_list1->head;
    struct dll *tmp1=NULL;
   struct dll *tmp2=NULL;
	
	
	if (!tmp) 
    {
        d_list->head = d_list1->head;
        d_list->last = d_list1->last;
        return;
    }

    
    if(tmp->data < rev->data)
    {
       tmp1 = tmp2 = tmp;
       tmp=tmp->next;
    }
    else
    {
       tmp1 = tmp2 = rev;
       rev = rev->next;
    }
    
    tmp1->prev=NULL;
    
    while(tmp && rev)
    {
      if(tmp->data < rev->data)
      {
        tmp2->next =tmp;
        tmp->prev = tmp2;
        tmp2=tmp;
        tmp =tmp->next;
      }
      
      else
      {
        tmp2->next = rev;
        rev->prev = tmp2;
        tmp2=rev;
        rev=rev->next;
      }
    }
    
    
    // Attach the remaining nodes
    if (tmp) {
        tmp2->next = tmp;
        tmp->prev = tmp2;
    } else if (rev) {
        tmp2->next = rev;
        rev->prev = tmp2;
    }

    // Update the head and tail of d_list
    d_list->head = tmp1;
    dll_t *tail = tmp1;
    while (tail->next) {
        tail = tail->next;
    }
    d_list->last = tail;

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
   printf("\n");
}

int main()
{
   node_t *d_list = (struct node*)malloc(sizeof(struct node));
    node_t *d_list1 = (struct node*)malloc(sizeof(struct node));
   d_list->head = NULL;
   d_list->last = NULL;
   
   if(d_list == NULL)
   {
   	printf("d_list memory alloction failed");
   	return -1;
   }
   
   for(int  i=0;i<1;i++)
   {
     insert(d_list,1);
      insert(d_list,4);
       insert(d_list,5);
   }
   
   for(int  i=3;i<4;i++)
   {
     insert(d_list1,3);
     insert(d_list1,6);
     insert(d_list1,2);
     
   }
   free_ptr(d_list);
   	
   merge_sorts_list(d_list,d_list1);
   	
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
