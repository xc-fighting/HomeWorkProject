#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "my402list.h"
/*
  global variables for time intervals
*/
 int lambda=1;
 float mu=0.35;
 float  r=1.5;
 int B=10;
 int P=3;
 int num=20;
 char* tsfile;
 
/*
  two queue needed for thread
*/
 My402List Q1;
 My402List Q2;
 int bucket=0;;

 pthread_mutex_t lock=PTHREAD_MUTEX_INITIALIZER;
 
 pthread_t packet_thread;
 pthread_t token_thread;
 pthread_t server1_thread;
 pthread_t server2_thread;

 typedef struct tagpacket{
 	long arrive_time;
 	long departure_time;
 	int demand;
 	int id;
 }packet;
 void* serverFunction(){
 	while(TRUE){
 		pthread_mutex_lock(&lock);
 		if(bucket<B){
 			bucket++;
            printf("bucket %d arrives,token bucket now have %d token\n",bucket,bucket);
 		}
        
 	    pthread_mutex_unlock(&lock);
 	    sleep(2);
 	}
 	
 	return (void*)NULL;
 }

 void* packetFunction(){
 	int i=0;
 	int solved=0;
 	while(solved<num){
 		pthread_mutex_lock(&lock);
 		if(i<num){
 			packet* item=(packet*)malloc(sizeof(packet));
            item->id=i+1;
            item->demand=P;
            printf("p%d arrives,needs %d,inter-arrival time=%.3fms\n",item->id,item->demand,(float)(1000/lambda));
            My402ListAppend(&Q1,(void*)item);
            printf("p%d enters Q1\n",item->id);
            i++;
 		}
        
        My402ListElem* head=My402ListFirst(&Q1);
        packet* temp=(packet*)(head->obj);
        if(bucket>=temp->demand){
        	bucket-=temp->demand;
        	My402ListUnlink(&Q1,head);
        	solved++;
            printf("get out of Q1 and bucket size is %d\n",bucket);
        }
 	    pthread_mutex_unlock(&lock);
 	    sleep(1);

 	}
 	
 	return (void*)NULL;
 }

 void* bucketFunction(){
 	 pthread_mutex_lock(&lock);
     

 	 pthread_mutex_unlock(&lock);
 	 return (void*)NULL;
 }
 void printParam(){
	 printf("Emulation Parameters:\n");
	 printf("number to arrive = %d\n",num);
	 printf("lambda = %d\n",lambda);
	 printf("mu = %f\n",mu);
	 printf("r = %f\n",r);
	 printf("B = %d\n",B);
	 printf("P = %d\n",P);
     printf("%s\n",tsfile);
     printf("emulation begins\n");
 }
 void makeChoice(int argc,char* argv[]){
     if(argc==1){
     	 fprintf(stderr,"command line param number misformed/n");
     }
     int i;
     for(i=1;i<argc;i+=2){
     	 if(strcmp(argv[i],"-lambda")==0){
               lambda=atoi(argv[i+1]);
     	 }
     	 else if(strcmp(argv[i],"-mu")==0){
               mu=atof(argv[i+1]);
     	 }
     	 else if(strcmp(argv[i],"-r")==0){
               r=atof(argv[i+1]);
     	 }
     	 else if(strcmp(argv[i],"-B")==0){
               B=atoi(argv[i+1]);
     	 }
     	 else if(strcmp(argv[i],"-P")==0){
               P=atoi(argv[i+1]);
     	 }
     	 else if(strcmp(argv[i],"-n")==0){
               num=atoi(argv[i+1]);
     	 }
     	 else if(strcmp(argv[i],"-t")==0){
               int len=0;
               int index=0;
               for(index=0;argv[i+1][index]!='\0';index++){
               	    len++;
               }
               tsfile=(char*)malloc(sizeof(char)*(len+1));
               strcpy(tsfile,argv[i+1]);
     	 }
     }
     return;
}
int main(int argc,char* argv[]){

    makeChoice(argc,argv);
    printParam();
    My402ListInit(&Q1);
    My402ListInit(&Q2);
    
    pthread_create(&packet_thread,0,serverFunction,(void*)NULL);
    pthread_create(&token_thread,0,packetFunction,(void*)NULL);
    pthread_create(&server1_thread,0,serverFunction,(void*)NULL);
    pthread_create(&server2_thread,0,serverFunction,(void*)NULL);
    pthread_join(packet_thread,0);
    pthread_join(token_thread,0);
    pthread_join(server1_thread,0);
    pthread_join(server2_thread,0);
    printf("emulation ends\n");
	return 0;
}