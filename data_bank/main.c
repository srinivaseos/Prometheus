#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "queues.c"
#include "server.c"
//#include "struct_Def.h"

struct bank_details *s_bank =NULL; 
void display()
{
     struct bank_details *current = s_bank;
 while(current != NULL)
 {
    
    printf("cretea bank details:%s %d %d %s %u %d\n",current->name,current->ifsc_code,current->acc_no,
    current->branch,current->phno,current->balance);

    current = current->next;
    }
}

int count =0;
int find_id_or_name(struct bank_details* head)
{
	int id,n=0;
	int acc;
	char name[30];
	printf("enter to find acc:");
		scanf("%d",&acc);
	struct bank_details *current = head;
	int num;
	
		while (current != NULL)
		{
			//printf("enter to find id_num:%d",acc);
			if (current->acc_no == acc) 
			{
				printf("Found id_num = %d\n", current->acc_no);
				return current->acc_no;
			}
			/*else
			{
				printf("not found id_num=%d\n",id_num);
				return -1;
			}*/
			count++;
		
			current = current->next;
		}
			
	
	printf("not found id_num=%d\n",acc);
				return -1;	
	
	//return 0;
}
void create_account(int ifsc_code,int phno,int acc_no)
{
    struct bank_details *current = malloc(sizeof(struct bank_details));
     struct bank_details *prev;
    strcpy(current->name,"srinivas");
    current->acc_no= acc_no; 
    current->ifsc_code = ifsc_code;
    strcpy(current->branch,"xius");
    current->phno=phno;
    current->balance =10000;
    current->next = NULL;
        if(s_bank == NULL)
        {
            s_bank=current;
        }
       else if(current != NULL)
        {
         strcpy(current->name,"reddy");
        current->acc_no= acc_no; 
        current->ifsc_code = ifsc_code;
        strcpy(current->branch,"xius");
        current->phno=phno;
         current->balance =10000;
        current->next = s_bank;
          
        }
        s_bank = current;
       printf("cretea bank details:%s %d %d %s %u %d\n",current->name,current->ifsc_code,current->acc_no,current->branch,current->phno,current->balance);

}

void modify_acc()
{
    struct bank_details *current = malloc(sizeof(struct bank_details));
    current = s_bank;
    int bank_num = find_id_or_name(current);
    int option;
    while(current !=NULL)
    {
        if(current->acc_no == bank_num)
        {
        do{
            printf("enter ,1.name,2.phno");
            scanf("%d",&option);
            switch(option)
            {
                case 1:printf("enetr name:");
                        scanf("%s",current->name);
                        break;
                case 2:printf("enter phon");
                        scanf("%d",&current->phno);
                        break;
                case 3:break;
                default:printf("choose correct option\n");
                        break;
            }
              printf("cretea bank details:%s %d %d %s %u %d\n",current->name,current->ifsc_code,current->acc_no,
         current->branch,current->phno,current->balance); 
            
        }while(option != 3);
        }
        current = current->next;

    }
}   

void amount_update()
{
     struct bank_details *current = malloc(sizeof(struct bank_details));
    current = s_bank;
    int bank_num = find_id_or_name(current);
    while(current != NULL)
    {

        if(current->acc_no == bank_num)
        {
            int money;
            printf("eneter amount to add:");
            scanf("%d",&money);
            current->balance += money;
            printf("total amount %d:: %d\n",current->acc_no,current->balance);
            break;
        }
        current = current->next;
    }
}

void amount_withdraw()
{
    struct bank_details *current = malloc(sizeof(struct bank_details));
    current = s_bank;
    int bank_num = find_id_or_name(current);
    while(current != NULL)
    {

        if(current->acc_no == bank_num)
        {
            if(current->balance > 0  && current->balance < 1000)
            {
                printf("\nu r maintaning low balance\n");
                break;
            }
            else if(current->balance > 1000)
            {
                int money;
                printf("eneter amount to witdraw:");
                scanf("%d",&money);
                current->balance -= money;
                if(current->balance < 1000)
                {
                    current->balance += money;
                    printf("enter sufficent balance\n");
                    break;
                }
                printf("total amount %d\n",current->balance);
                break;
             }
            
            else{
                printf("no blanace\n");
                break;
            }
        }
        current = current->next;
    }
}

void amount_check()
{
    struct bank_details *current = s_bank;// = malloc(sizeof(struct bank_details));
    //current = s_bank;
    int bank_num = find_id_or_name(current);
    while(current != NULL)
    {

        if(current->acc_no == bank_num)
        {
            printf("total amount to check %d:: %d\n",current->acc_no,current->balance);
            break;
        }
        current = current->next;
    }
}
void amount_transfer()
{
   struct bank_details *current = malloc(sizeof(struct bank_details));
   struct bank_details *prev = s_bank;
    current = s_bank;

    int bank_num = find_id_or_name(current);
    int dub_num = find_id_or_name(prev);
    while(current != NULL)
    {
        
        printf("exited\n");
        if(current->acc_no == bank_num )
        {
            while(prev != NULL)
            {
                if(current->acc_no == bank_num && prev->acc_no == dub_num)
                {
                    if(current->balance > 0  && current->balance < 1000)
                    {
                        printf("\nu r maintaning low balance\n");
                        break;
                    }
                else if(current->balance > 1000)
                {
                    int money;
                    printf("eneter amount to witdraw:");
                    scanf("%d",&money);
                    current->balance -=money;
                    
                    if(current->balance < 1000)
                    {
                        current->balance += money;
                        printf("enter sufficent balance\n");
                        break;
                    }
                    else
                    {
                    prev->balance +=money;
                    break;
                    }
                    printf("total amount %d\n",current->balance);
                    break;
                }
                else
                {
                    printf("no blanace\n");
                    break;
                }
            }
          
          prev = prev->next;
         }    
        }
    current = current->next;
    }
    //printf("non account trnasfer\n");
}
int main()
{
    
    
    int ifsc_code=5678,phno,acc_no,option;
    udp_server();
   while(1)
   {
    // printf("choose 1.display 2.create_acc 3:modify_acc 4:amount_update 5:amount_withdraw 6:amount_check 7:amount_transfer\n");
     //scanf("%d",&option);
	option= udp_recv();
        switch(option)
        {               
            case 1: display();
                    break;
            case 2: phno=ph(),acc_no=accno();
                    create_account( ifsc_code,phno, acc_no);
                    break;
            case 3:modify_acc();
                    break;
            case 4:amount_update();
                    break;
            case 5:amount_withdraw();
                    break;
            case 6:amount_check();
                    break;
            case 7:amount_transfer();
                    break;
            case 8:exit(1);
                    break;
            default:printf("choose correct option\n");
                    break;
        }
  }
   
   return 0;
}
