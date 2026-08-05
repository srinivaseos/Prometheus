#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "queues_def.h"
int i =0;
int id_generator()
{
	int id;
	
	id = 20240001 + i;
	//printf("id_generaotr=%d",id);
	i++;
	return id;
}

/* char *names_genertor(int queue_id)
{
	char name[30];
	memset(name,0,sizeof(name));
	sprintf(name,"srinu-%d",queue_id);
	
	return name;
}
 */
 
 //char* names_generator(int queue_id) {
 char* names_generator() {
    // Allocate memory dynamically for the name string
    char* name = (char*)malloc(30 * sizeof(char));
 
    if (name == NULL) {
        perror("Memory allocation failed\n");
        exit(1); // Exit the program if malloc fails
    }
	
    if (name != NULL) {
        memset(name, 0, sizeof(name));  // Initialize memory
        
        sprintf(name,"%s","srinu-xius");
    } else {
        // Handle memory allocation failure if needed
        printf("Memory allocation failed\n");
    }

    return name;
	
}

char *gender_choose()
{
	int choose =1; //1-male 2-female
	
	char *gender = (choose ==1) ? "Male" :"Female" ;
	if (gender == NULL) {
        perror("Memory allocation failed\n");
        exit(1); // Exit the program if malloc fails
    }
	
	return gender;
	
	
}
int y=0;
float attend_generator()
{
	int attend;
	
	attend = 65 + y;
	//printf("attend_generaotr=%d",attend);
	y++;
	return attend;
}

