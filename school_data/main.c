#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "queue.h"
#include "structure_types.h"

struct student *head = NULL;


int found_pos()
{
	
}
//int create_details(int queue_id)
int create_details(int queue_id,char *name,char *gender,float attend)
{
	
	struct student *current = (struct student*)malloc(sizeof(struct student));
	
	// int id;
	// char name[30];
	// float attend;
	
	
	
	//printf("enter student details name\nattend\n");
	//scanf("%s %f",name,&attend);
	current->id = queue_id;
	//current->id = id;
	strcpy(current->name,name);
	strcpy(current->gender,gender);
	current->u.attend = attend;
	current->next = NULL;
	
	//current->next = head;
	//head = current;
	if(head == NULL)
	{
	
		head= current;
		printf("1.create details id =%d name =%s gender = %s attend =%f\n",current->id,current->name,current->gender,current->u.attend);
		return 1;
	}
	
	struct student *prev =head;
	
		while(prev->next != NULL)
		{
			prev = prev->next;
		}
		prev->next = current;
	

	printf("2.create details id =%d name =%s attend =%f\n",current->id,current->name,current->u.attend);
	
	
	return 0;
}
int count =0;
int find_id_or_name(struct student* temp)
{
	int id,n=0;
	int id_num;
	char name[30];
	printf("enter to find id_num:");
		scanf("%d",&id_num);
	struct student *current = temp;
	int num;
	while(1)
	{
		while (current != NULL)
		{
			printf("enter to find id_num:%d",id_num);
			if (current->id == id_num) 
			{
				printf("Found id_num = %d\n", current->id);
				return current->id;
			}
			else
			{
				printf("not found id_num=%d\n",id_num);
				return -1;
			}
			count++;
		
			current = current->next;
		}
			
	}
		
	
	return 0;
}
void modify_details()
{
		struct student *mod = head;
	   
		int id_num,choose;
		int mod_id;
		char mod_name[30];
		float mod_attend;
		
		printf("enter id_num\n");
		scanf("%d",&id_num);
		printf("entered id_num=%d\n",id_num);
		
			while(mod != NULL)
			{
				if(mod->id == id_num)
				{
					
						printf("enter choose:\n");
						printf("\tA.id\t B.name\tC.attend\tD.exit\n");
						scanf("%d",&choose);
							switch(choose)
							{
								case 1:
											printf("enetr id for change:");
											scanf("%d",&mod_id);
											mod->id = mod_id;
											break;
								case 2:
											printf("enetr id for change:");
											scanf("%s",mod_name);
											strcpy(mod->name,mod_name);
											break;
								case 3:
											printf("enetr id for change:");
											scanf("%f",&mod_attend);
											mod->u.attend = mod_attend;
											break;
								default:
										break;
										 //exit(0);
								
							}
							printf("MODIFY details id =%d name =%s attend =%f\n\n",mod->id,mod->name,mod->u.attend);
					
				}
				
			  mod = mod->next;	
			}
	
	
}

void verify_details()
{
		struct student *temp = head;//= (struct student*)malloc(sizeof(struct student));
		
		while(temp !=NULL)
		{
			printf("verify details id =%d name =%s attend = %f\n",temp->id,temp->name,temp->u.attend);
			temp = temp->next;
		}
		
}

void student_verify_details() 
{
		struct student *temp = head;
		int id,id_num;
		
		//id = find_id_or_name(temp);
		
		printf("enter to find id_num:");
		scanf("%d",&id);
		
		 while(temp !=NULL)
		{
			if(temp->id == id)
			{
				printf("2.verify details id =%d name =%s attend = %f\n",temp->id,temp->name,temp->u.attend);
				printf("address details housenum=%s floor=%d colony =%s area=%s city=%s pin_code=%d\n",temp->may->housenum,temp->may->floor,temp->may->colony,temp->may->area,temp->may->city,temp->may->pin_code);
				break;
			}
		  temp = temp->next;
		} 
		printf("no stduent records\n");
		 
		
}
int update_information()
{
	struct student *update_details = head;
	
	
	int id,id_num;
	printf("enter id_num\n");
	scanf("%d",&id_num);
	printf("entered id_num=%d\n",id_num);
		
		while(update_details != NULL)
		{
			if(update_details->id == id_num)
			{
				printf("enter_details");
				scanf("%d",&id);
	
				update_details->u.course_id = id;
				printf("update_details id =%d name =%s attend=%f course_id=%d \n",update_details->id,update_details->name,update_details->u.attend,update_details->u.course_id);
				//free(update_details);
				break;
			}
			
			update_details = update_details->next;
		}
	return -1;
	
}

int address_details()
{
	
	struct address *may= (struct address*)malloc(sizeof(struct address));;
	char housenum[30];
	int floor;
	char colony[100];
	char area[100];
	char city[100];
	int pin_code;
	struct student *add = head;
	   
		int id_num;
		
		printf("enter id_num\n");
		scanf("%d",&id_num);
		printf("entered id_num=%d\n",id_num);
		
		while(add != NULL)
		{
			
			if(add->id == id_num)
			{
				printf("entered id_num=%d\n",add->id);
				strcpy(add->may->housenum,"134-2/81");
				add->may->floor = 3;
				strcpy(add->may->colony,"my-home_hub");
				strcpy(add->may->area,"hitech-city");
				strcpy(add->may->city,"hyd");
				add->may->pin_code = 518001;
				
	printf("address details housenum=%s floor=%d colony =%s area=%s city=%s pin_code=%d\n",add->may->housenum,add->may->floor,add->may->colony,add->may->area,add->may->city,add->may->pin_code);
				free(may);
			}
			add = add->next;
		}
		
	return 0;
}

void delete_details()
{
	struct student *del = head;
	struct student *del_temp = NULL; ;
		
		int id_num,choose;
		int del_id;
		char del_name[30];
		float del_attend;
		
		printf("enter id_num\n");
		scanf("%d",&id_num);
		printf("entered id_num=%d\n",id_num);
		
			 while(del != NULL)
			{
				
				if(del->id == id_num)
				{
					if(del == head)
					{
						head = head->next;
					}
					else
					{
						del_temp->next = del->next; 
						del->next = NULL;
						del->next = del_temp->next;	
						printf("deleting id =%d name =%s gender = %s attend=%f course_id=%d \n",del->id,del->name,del->gender,del->u.attend,del->u.course_id);
					   free(del);
					}
					break;
				}
				del_temp = del;
				del = del->next;
			} 
		
			
}

/*void Delete_node(){

	int pos;
	printf("enter the positon :");
	scanf("%d",&pos);
	
	if(pos < 0)
	{
		printf("invalid postion\n");
	}
	struct student *del; // = (struct student*)malloc(sizeof(struct student));	
	struct student *temp;	
	del = head;
	
	if(pos == 1){
		
		head = head->next;
		
	}else{
	
		int i = 1;
		while(i < pos-1){
			del = del->next;
			printf("i=%d\n",i);
			i++;
			
		}
		printf("deleting details id =%d name =%s attend = %f\n",del->id,del->name,del->u.attend);
		
		temp = del->next;
		
		if(temp->next == NULL){
			return ;
		}
		
		del->next = temp->next;
		
		// del->next = del->next->next;
		printf("after deleting details id =%d name =%s attend = %f\n",del->id,del->name,del->u.attend);
		}
}
*/
int main()
{
	int option,update=0;
	int queue_id,attend_id;
	char *str,*gender;
	
	while(1)
	{
		/* printf("enter option:\n");
		printf("\t1.create_details\t2.modify_details\t3.verify_details\t4.delete_details\n");
		scanf("%d",&option); */
		option = login_details();
		switch(option)
		{
			case 1:
					 queue_id =id_generator();
					  str = names_generator();
					  gender = gender_choose();
					 // attend_id = attend_generator();
					create_details(queue_id,str,gender,attend_generator());
					printf("enter to update course");
					scanf("%d",&update);
					if(update == 1)
					{
						update_information();
					}
					free(str);
					//free(gender);
					break;
			case 2:
					modify_details();
					printf("enter to update course");
					scanf("%d",&update);
					if(update == 1)
					{
						update_information();
					}
					break;
			case 3:
					verify_details();
					break;
			case 4:
					student_verify_details();
					break;
			case 5:address_details();
					break;
			case 6:
					delete_details();
					break;
			default:
					exit(0);
		}
	}
}

/* void name_generator()
{
	for(i=0;i<5;i++)
	{
		 queue_id =id_generator();
		create_details(queue_id);
		
	}
} */
