#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>


struct class
{
	float attend;
	int course_id;
};

typedef struct student
{
	int id;
	char name[30];
	char gender[30];
	struct address *may;  // student address details
	struct student *next;
	struct class u;
	
}student_t;


typedef struct address
{
	char housenum[30];
	int floor;
	char colony[100];
	char area[100];
	char city[100];
	int pin_code;
}address_t;