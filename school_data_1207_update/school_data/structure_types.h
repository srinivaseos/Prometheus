#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>


#define SUBJECT_SIZE  60
struct class
{
	float attend;
	int course_id;
};


typedef struct subjects
{
	struct score
	{
		int marks[6];
	}u;
	char math[SUBJECT_SIZE]; 		
	char telugu[SUBJECT_SIZE];	    
	char science[SUBJECT_SIZE];     
	char social[SUBJECT_SIZE]; 		
	char english[SUBJECT_SIZE];     
	char hindi[SUBJECT_SIZE]; 		
}subjects_t;

typedef struct address
{
	char housenum[30];
	int floor;
	char colony[100];
	char area[100];
	char city[100];
	int pin_code;
}address_t;

typedef struct student
{
	int id;
	char name[30];
	char gender[30];
	struct address may;  // student address details
	struct subjects stu_sub;
	struct student *next;
	struct class u;
	
}student_t;


