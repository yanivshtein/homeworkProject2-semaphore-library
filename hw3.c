#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <semaphore.h>
#include <unistd.h>

#define N 10
#define K 4


typedef struct Node{
	int data;
	struct Node* next;
	sem_t sem;
} node;

typedef struct Queue{
	struct Node* front;
	struct Node* rear;
} queue;

sem_t mutex1,mutex2,mutex3,mutex4,mutex5,librarian,customer,libDone,leave,customers[N+2];
queue* qLib,*qCouch,*qOut;
int inside = 0,couch = 0,waiting = 0,waitLib = 0,Lib = 0;

//function to create node
node* createNode(int data,sem_t s){
	//allocate memory for new node
	node* newNode = (node*)malloc(sizeof(node));
	
	//check if memory allocation was successful
	if(newNode == NULL){
		printf("memory allocation was unsuccessful");
		exit(1);
	}
	//init the node data and next
	newNode->data = data;
	newNode->sem = s;
	newNode->next = NULL;
	return newNode;
}

//function to create queue
queue* createQueue(){
	//allocate memory for new queue
	queue* newQueue = (queue*)malloc(sizeof(queue));
	//check if memory allocation was successful
	if(newQueue == NULL){
		printf("memory allocation was unsuccesful");
		exit(1);
	}
	//init front and rear to NULL
	newQueue->front = NULL;
	newQueue->rear = NULL;
	return newQueue;
}

//function to add a node to the queue
void enqueue(queue* q, node* n){
	//if the queue is empty, set the front and rear nodes
	if(q->rear == NULL){
		q->rear = n;
		q->front = n;
		return;
	}
	//if queue is not empty, add the new node to the end of the line
	q->rear->next = n;
	q->rear = n;
}

//function to remove a node from the line
void dequeue(queue* q){
	//check if the queue is empty
	if(q->front == NULL){
		return;
	}
	//remove the front node from the queue and move the queue forward
	node* temp = q->front;
	q->front = q->front->next;
	if(q->front == NULL){
		q->rear = NULL;
	}
	temp->next = NULL;
	//let this specific customer keep going
	sem_post(&temp->sem);
}
	

void* Customer(void* node1){
	int i;
	node* n = (node*)node1;
	while(1){
		//checking if the library is full
		if(inside < N){
			//critical code
			sem_wait(&mutex1);
			//updating the amount of people inside the library
			inside++;
			sem_post(&mutex1);
			printf("I'm Reader #%d, I got into the library\n",n->data);
			sleep(1);
			//check how many people are sitting on the couch
			if(couch < K){
				//count up the amount of people now sitting and continue
				sem_wait(&mutex2);
				couch++;
				sem_post(&mutex2);
			}else{
				sem_wait(&mutex3);
				//queue this person in line to sitting on the couches
				enqueue(qCouch,n);
				//count up the amount of people waiting for the couches
				waiting++;
				sem_post(&mutex3);
				//the couches are full if we reached here, so make this person wait untill a couch is free
				sem_wait(&n->sem);
			}
			//continue to couch
			//if couch is free, the reader can read on a couch
			printf("I'm Reader #%d, I'm reading a book\n",n->data);
			sleep(1);
			//wait for a librarian
			sem_wait(&librarian);
			//end of couch for this person
			if(waiting > 0){
				sem_wait(&mutex3);
				//dequeue the next person in line from the queue and let them keep going
				dequeue(qCouch);
				//count down the amount of people waiting for the couches
				waiting--;
				sem_post(&mutex3);
			}
			//enter the queue to the librarian
			if(Lib < 3){
				//count up the amount of people now talking to a librarian and continue
				sem_wait(&mutex4);
				Lib++;
				sem_post(&mutex4);
			}else{
				sem_wait(&mutex5);
				//queue this person in line to talking to a librarian
				enqueue(qLib,n);
				//count up the amount of people waiting for a librarian
				waitLib++;
				sem_post(&mutex5);
				//the librarians are all busy if we reached here, so make this person wait untill one is free
				sem_wait(&n->sem);
			}
			printf("I'm Reader #%d, I'm speaking with a librarian\n",n->data);
			sleep(1);
			//signal librarian i am ready
			sem_post(&customer);
			//make room on the couch
			sem_wait(&mutex2);
			couch--;
			sem_post(&mutex2);
			//waiting for signal from librarian that she is done working with me
			sem_wait(&libDone);
			printf("I'm Reader #%d, I've finished\n",n->data);
			sleep(1);
			//end of librarian
			if(waitLib > 0){
				sem_wait(&mutex5);
				//dequeue the next person in line from the queue and let them keep going
				dequeue(qLib);
				//count down the amount of people waiting for the librarians
				waitLib--;
				sem_post(&mutex5);
			}
			//count down how many people are inside the library
			sem_wait(&mutex1);
			inside--;
			sem_post(&mutex1);
			//signal that someone left the library
			sem_post(&leave);
			//let the next person go to a librarian
			sem_wait(&mutex4);
			Lib--;
			sem_post(&mutex4);
			//give chance to a different person to enter the library
			sleep(1);
			
		}else{
			printf("I'm Reader #%d, I'm out of the library\n",n->data);
			sleep(1);
			sem_wait(&leave);
		}
	}
}

void* Librarian(void* numLib){
	int num = *((int*) numLib);
	while(1){
		sem_wait(&customer);
		printf("I'm Librarian #%d, I'm working now\n",num); 
		sleep(1);
		sem_post(&libDone);
		sem_post(&librarian);
	}
}


int main(){
	pthread_t arrCus[N+2], arrLib[3];
	int i, arr[3];
	sem_init(&mutex1,0,1);
	sem_init(&mutex2,0,1);
	sem_init(&mutex3,0,1);
	sem_init(&mutex4,0,1);
	sem_init(&mutex5,0,1);
	sem_init(&libDone,0,0);
	sem_init(&leave,0,0);
	sem_init(&librarian,0,3);
	sem_init(&customer,0,0);
	//init for all the customers
	for(i = 0; i < N + 2; i++){
		sem_init(&customers[i],0,0);
	}
	node* nodes[N+2];
	//create que for librarians and for the couch
	qLib = createQueue();
	qCouch = createQueue();
	//creating a node for all the customers
	for(i = 0; i < N + 2; i++){
		nodes[i] = createNode(i+1,customers[i]);
	}
	//creating customer threads
	for(i = 0; i < N + 2; i++){ 
		if(pthread_create(&arrCus[i],NULL,Customer,(void*)nodes[i]) != 0){
			printf("error creating thread\n");
			exit(1);
		}
	}
	//creating librarian threads
	for(i = 0; i < 3; i++){ 
		arr[i] = i+1;
		if(pthread_create(&arrLib[i],NULL,Librarian,(void*)&arr[i])  != 0){
			printf("error creating thread\n");
			exit(1);
		}
	}
	//joining customer threads
	for(i = 0; i < N + 2; i++){
		if(pthread_join(arrCus[i],NULL) != 0){
			printf("error joining threads\n");
			exit(1);
		}
	}
	//joining librarian threads
	for(i = 0; i < 3; i++){
		if(pthread_join(arrLib[i],NULL) != 0){
			printf("error joining threads\n");
			exit(1);
		}
	}
	//freeing memory
	for(i = 0; i < N + 2; i++){
		free(nodes[i]);
	}
	
	return 0;
}
		




