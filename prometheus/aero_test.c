#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <prom.h>
#include <promhttp.h>
#include <microhttpd.h>

#include "metrics_link.h"

// Define the structure for a node
typedef struct Node {
    int data;
    struct Node* next;
} Node;

Node* createNode(int data) {
    Node* newNode = (Node*)malloc(sizeof(Node));
    newNode->data = data;
    newNode->next = NULL;
	
    return newNode;
}

double calculate_elapsed_time(struct timespec start, struct timespec end) {
    double start_sec = start.tv_sec + start.tv_nsec / 1e9;
    double end_sec = end.tv_sec + end.tv_nsec / 1e9;
    return end_sec - start_sec;
}


void insertAtHead(Node** head, int data) {
	clock_t start, end;
		double duration;

     // Get the current CPU time at the start
    start = clock();

  //  clock_gettime(CLOCK_MONOTONIC, &start_time);
    Node* newNode = createNode(data);
    newNode->next = *head;
    *head = newNode;
	  // Get the current CPU time at the end
    end = clock();

    // Calculate the duration in seconds
    duration = (double)(end - start) / CLOCKS_PER_SEC;
	//printf("enetr insertAtHead\n ");
    successfull_task_req("PUT");
	//successfull_task_request("http://192.168.144.27:9096/metrics","PUT");
	//successful_task_completed("update", "PUT", start_time);
}

void insertAtTail(Node** head, int data) {

    clock_t start, end;
		double duration;

     // Get the current CPU time at the start
    start = clock();

  //  clock_gettime(CLOCK_MONOTONIC, &start_time);
    Node* newNode = createNode(data);
    if (*head == NULL) {
        *head = newNode;
        return;
    }
    Node* temp = *head;
    while (temp->next != NULL) {
        temp = temp->next;
    }
    temp->next = newNode;

    end = clock();
    duration = (double)(end - start) / CLOCKS_PER_SEC;
	//printf("enetr insertAtHead\n ");
	//successful_task(duration);
}

void deleteNode(Node** head, int data) {
    Node* temp = *head;
    Node* prev = NULL;
    
    // If the node to be deleted is the head
    if (temp != NULL && temp->data == data) {
        *head = temp->next; // Change head
        free(temp); // Free old head
        return;
    }
    
    // Search for the node to be deleted
    while (temp != NULL && temp->data != data) {
        prev = temp;
        temp = temp->next;
    }
    
    // If the data was not present in the list
    if (temp == NULL) 
	{
		
	//ErrorCountIncrement("delete_route", "delete");
		return;
	}
    
    // Unlink the node from the linked list
    prev->next = temp->next;
    deletionfull_task_req("DELETE");
    free(temp); // Free memory
}

void printList(Node* head) {
    Node* temp = head;
    while (temp != NULL) {
       // printf("%d -> ", temp->data);
        temp = temp->next;
    }
    //printf("NULL\n");
}

//gcc aero_test.c metrics_link.c -o metrics_link -lmicrohttpd -lpromhttp -lprom -lpthread

//http://192.168.144.27:9527/metrics\n

int main() {
	int ret =  metrics_test("promo_config.json");
	if(ret !=0)
	{
		perror("metrics failed\n");
		
		return EXIT_FAILURE;
	}
    Node* head = NULL; // Start with an empty list
    int n=0,sl=0,dl=0;
    while(n !=1)
    {
        for(int i=0;i<20;i++)
        {
            if(sl ==150)
            {
                sl=0;
                sleep(4);
            }
        // Insert some elements
            insertAtHead(&head, i);
            sl++;
        }
    // insertAtTail(&head, 2); 
    // insertAtTail(&head, 3);
        
        //printf("Linked List: ");
    // printList(head);

        // Delete an element
        for(int i=0;i<10;i++)
        {
            if(dl ==150)
            {
                dl=0;
                sleep(4);
            }
            dl++;
        deleteNode(&head, i);
        }
        n++;
    }
   // printf("After deletion: ");
   // printList(head);

    // Free remaining nodes
    while (head != NULL) {
        Node* temp = head;
        head = head->next;
        free(temp);
    }
    printf("out aero loop 12:40\n");
    return 0;
}

/*int main()
{
	int ret = materics();
	if(ret<0)
	{
		perror("metrics failed\n");
		
		return EXIT_FAILURE;
	}
	else{
		printf("materics succefull\n");
	}
	printf("1.aero succefull\n");
	 while (1) {
        sleep(10);
    }
	int res = aero();
	printf("2.aero succefull\n");
	if(ret<0)
	{
		perror("aero failed\n");
		
		return EXIT_FAILURE;
	}
	else{
		printf("aero succefull\n");
	}
	
}*/





