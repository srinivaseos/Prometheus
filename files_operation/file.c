#include <stdio.h>
#include <stdlib.h>

int main()
{

	const char *name = "file_name.txt";
  FILE* file = fopen(name,"w");
	
	if(file ==NULL)
	{
	printf("file is not open to write\n");
	return -1;
	}  
	else
	{
	  printf("file open to writes\n");
	}

 	fprintf(file,"helloworldnewdocument\n");
 	fputs("files operation\n",file);
	
	fclose(file); 	
	
 	file = fopen(name,"r");
 	if(file ==NULL)
	{
	printf("file is not open to read\n");
	return -1;
	}  
	else
	{
	  printf("file open to read\n");
	}
	int num;
	char buff[50];
	if(fscanf(file,"%s",buff) > 1)
	{
	 printf("%s\n",buff);
  	}
  	else
  	{
  	 printf("failed to read from file\n"); 
  	}
  fclose(file);
  return 0;
  
}
