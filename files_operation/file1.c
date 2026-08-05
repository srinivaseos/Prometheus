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

 	fprintf(file,"1hello world\n");
 	fputs("files operationn \n",file);
 	 fclose(file);
 	file = fopen(name,"r");
 	if(file ==NULL)
	{
	printf("file is not open to read\n");
	return 1;
	}  
	else
	{
	  printf("file open to read\n");
	}
	int num;
	
	char buff1[80];
	if(fscanf(file,"%d %[^n]s",&num,buff1) == 2)
	{
	 printf("%d %s\n",num,buff1);
  	}
  	else
  	{
  	 printf("failed to read from file%d\n"); 
  	}
	
		file = fopen(name,"r");
	char buff[50];
	 while (fgets(buff, 50, file) != NULL) {

            // Print the data
            printf("assume=%s", buff);
        }

  fclose(file);
  return 0;
  
}
