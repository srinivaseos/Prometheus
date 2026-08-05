#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
//int ifsc_code,phno,acc_no;
int i=0;
int accno()
{
    int no = 202418001;
    i++;
    return no+i;

}

int ph()
{
    srand(time(NULL));
    return rand();
}
