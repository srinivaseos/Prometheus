#include <stdio.h>
#include <stdlib.h>



int admin(int admin_id)
{
		
		/* int option;
		printf("enter option:\n");
		printf("\t1.create_details\t2.modify_details\n3.verify_details\t4.stduent_verfify_details\t5.address_details\t6.delete_details\n");
		scanf("%d",&option);
		if(option != 7)
		{
			return option;
		}
		else if (option == 7)
		{
			printf("enxit from admin\n");
			return -1;
		}
		else
		{
			printf("non option choossed\n");
		} */
	return admin_id;
}
int student(int student_id)
{
	printf("stduent_verfify_details\t");
	return student_id;
}
int login_details() 
{
	int option; 
	int admin_id =1,student_id =2;
	
		
		printf("1.admin login\n 2.student login \n");
		scanf("%d",&option);
		
		switch(option)
		{
			
			case 1:printf("enetering into admin login\n");
					
					return admin(admin_id);
					//break;
					
			case 2:printf("enetering into student login\n");
					return student(student_id);
					break;
			case 3:
					printf("exiting from application\n");
					exit(0);
			
		}
		return 0;
}
