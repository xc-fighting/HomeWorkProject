#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "my402list.h"
/*
  global variables for time intervals
*/
//packet interval rate
 float lambda=2;
 float arrival_interval=0;
 //server interval rate
 float mu=0.35;
 float service_interval=0;

 //token interval rate
 float  r=4;
 float  token_interval=0;

 //bucket capacity
 int B=10;
 //packet demand
 int P=3;
 //number of packet
 int num=20;
 //external file name
 char* tsfile;
 
 struct timeval programStart;
 long start=0;

/*
  two queue needed for thread
*/
 My402List Q1;
 My402List Q2;
 int bucket=0;;
 int solved=0;
 pthread_mutex_t lock=PTHREAD_MUTEX_INITIALIZER;
 pthread_cond_t cv=PTHREAD_COND_INITIALIZER;
 pthread_t packet_thread;
 pthread_t token_thread;
 pthread_t server1_thread;
 pthread_t server2_thread;

 typedef struct tagpacket{
 	long birth_time;
 	long arrive_time_q1;
 	long departure_time_q1;
 	long arrive_time_q2;
 	long departure_time_q2;
 	long death_time;
 	int demand;
 	int id;
 }packet;

  char* getFormedTime(long time){
 	 char* res=(char*)malloc(sizeof(char)*13);
     res[8]='.';
     res[12]='\0';
     int i=11;
     while(i>8){
     	 res[i]='0'+time%10;
     	 time=time/10;
     	 i--;
     }
     i=7;
     while(i>=0){
     	res[i]='0'+time%10;
     	time=time/10;
     	i--;
     }
     return res;
 }


/*
  function for two server program
 
*/


 void* serverFunction(void* arg){
    int serverId=*((int*)arg);
    struct timeval curTime;
    char* ts=NULL;
 	while(TRUE){

 		gettimeofday(&curTime,NULL);
      //  long lastTime=curTime.tv_sec*1000000+curTime.tv_usec;
 		pthread_mutex_lock(&lock);
 		while(My402ListEmpty(&Q2)){
 			if(solved>=num){
 			//	printf("%d exit the thread from break 1\n",serverId);
 				pthread_mutex_unlock(&lock);
 				pthread_exit(0);
 			}
 			pthread_cond_wait(&cv,&lock);
 		}    
 	    
        My402ListElem* head=My402ListFirst(&Q2);
        packet* delItem=(packet*)(head->obj);
        My402ListUnlink(&Q2,head);
        gettimeofday(&curTime,NULL);
        long now=curTime.tv_sec*1000000+curTime.tv_usec;
        delItem->departure_time_q2=now;
        ts=getFormedTime(now-start);
        printf("%sms: p%d leaves Q2, time in Q2 = %.3fms\n",
        	ts,delItem->id,((delItem->departure_time_q2-delItem->arrive_time_q2)/1000.0));
        free(ts);
        solved++;
        gettimeofday(&curTime,NULL);
        now=curTime.tv_sec*1000000+curTime.tv_usec;
        long service_begin=now;
        ts=getFormedTime(now-start);
        printf("%sms: p%d begins service at S%d, requesting %.3fms of service\n",ts,delItem->id,serverId,service_interval*1000);
        free(ts);
        
        if(solved>=num){
        //	    printf("%d exit the thread from break 2\n",serverId);
 				pthread_mutex_unlock(&lock);
 				pthread_cond_signal(&cv);
 				
		     //   long endTime=curTime.tv_sec*1000000+curTime.tv_usec;
		    //    if(endTime-lastTime<service_interval*1000000)
		        usleep(service_interval*1000000);
		        gettimeofday(&curTime,NULL);
		        long birth=delItem->birth_time;
		        int id=delItem->id;
		        free(delItem);
		        gettimeofday(&curTime,NULL);
		        long service_end=curTime.tv_sec*1000000+curTime.tv_usec;
		        ts=getFormedTime(service_end-start);
		        printf("%sms: p%d departs from S%d, service time=%.2fms, time in system=%.3fms\n",ts,id,serverId,((service_end-service_begin)/1000.0),(service_end-birth)/1000.0);
		        free(ts);
 				
 				pthread_exit(0);
 			}
        pthread_mutex_unlock(&lock);
        gettimeofday(&curTime,NULL);
     //   long endTime=curTime.tv_sec*1000000+curTime.tv_usec;
    //    if(endTime-lastTime<service_interval*1000000)
        usleep(service_interval*1000000);
        long birth=delItem->birth_time;
        int id=delItem->id;
        free(delItem);
        gettimeofday(&curTime,NULL);
        long service_end=curTime.tv_sec*1000000+curTime.tv_usec;
        ts=getFormedTime(service_end-start);
        printf("%sms: p%d departs from S%d, service time=%.2fms, time in system=%.3fms\n",ts,id,serverId,((service_end-service_begin)/1000.0),(service_end-birth)/1000.0);
        free(ts);
 	}
    
 	return (void*)NULL;
 }
 

 
/*
  function for packet arrival
*/

 void* packetFunction(){
 	int i=0;
 	struct timeval curTime;
    char* ts=NULL;
    
 	while(solved<num){
 		gettimeofday(&curTime,NULL);
 		long lastTime=curTime.tv_sec*1000000+curTime.tv_usec;

 		pthread_mutex_lock(&lock);

 		if(i<num){
 			packet* item=(packet*)malloc(sizeof(packet));
            item->id=i+1;
            item->demand=P;
            long now=curTime.tv_sec*1000000+curTime.tv_usec;  
            item->birth_time=now; 
            ts=getFormedTime(now-start);       
            printf("%sms: p%d arrives,needs %d tokens,inter-arrival time=%.3fms\n",ts,item->id,item->demand,(float)(1000/lambda));
            free(ts);
            My402ListAppend(&Q1,(void*)item);
            gettimeofday(&curTime,NULL);
            now=curTime.tv_sec*1000000+curTime.tv_usec;
            item->arrive_time_q1=now;
            ts=getFormedTime(now-start);
            printf("%sms: p%d enters Q1\n",ts,item->id);
            free(ts);
            i++;
 		}
        if(My402ListEmpty(&Q1)==FALSE){
        	    My402ListElem* head=My402ListFirst(&Q1);
		        packet* temp=(packet*)(head->obj);
		        if(bucket>=temp->demand){
		        	bucket-=temp->demand;
		    
		        	My402ListUnlink(&Q1,head);
		            gettimeofday(&curTime,NULL);
		        	long now=curTime.tv_sec*1000000+curTime.tv_usec;
		        	temp->departure_time_q1=now;
		        	ts=getFormedTime(now-start);
                    printf("%sms: p%d leaves Q1, time in Q1=%.3fms,token bucket now has %d token\n",
                    	ts,temp->id,((temp->departure_time_q1-temp->arrive_time_q1)/1000.0),bucket);
                    free(ts);
		        	My402ListAppend(&Q2,(void*)temp);
		        	gettimeofday(&curTime,NULL);
		        	now=curTime.tv_sec*1000000+curTime.tv_usec;
		        	temp->arrive_time_q2=now;
		        	ts=getFormedTime(now-start);
		        	printf("%sms: p%d enters Q2\n",ts,temp->id);
		        	free(ts);
		        	pthread_cond_signal(&cv);
		        
		        }
        }
       
 	    pthread_mutex_unlock(&lock);
 	    //get time stamp
 	    gettimeofday(&curTime,NULL);
 	    long endTime=curTime.tv_sec*1000000+curTime.tv_usec;
 	    if(endTime-lastTime<arrival_interval*1000000);
 	    usleep(arrival_interval*1000000-(endTime-lastTime));

 	}
 //	printf("packet thread end\n");
 //	pthread_mutex_unlock(&lock);
 	return (void*)NULL;
 }

/*
   function for generating tokens
*/
 void* bucketFunction(){
 	struct timeval curTime;
     char* ts=NULL;
 	 while(solved<num){
             gettimeofday(&curTime,NULL);
 		     long lastTime=curTime.tv_sec*1000000+curTime.tv_usec;
             pthread_mutex_lock(&lock);
		     if(bucket<B){
		 			bucket++;
		 			gettimeofday(&curTime,NULL);
		 			long cur=curTime.tv_sec*1000000+curTime.tv_usec;
		 			ts=getFormedTime(cur-start);
		            printf("%sms: token t%d arrives,token bucket now have %d token\n",ts,bucket,bucket);
		            free(ts);
		 	 }
		     
		     if(My402ListEmpty(&Q1)==FALSE){
		        My402ListElem* head=My402ListFirst(&Q1);
		        packet* temp=(packet*)(head->obj);
		        if(bucket>=temp->demand){
		        	bucket-=temp->demand;
		    
		        	My402ListUnlink(&Q1,head);
		            gettimeofday(&curTime,NULL);
		        	long now=curTime.tv_sec*1000000+curTime.tv_usec;
		        	temp->departure_time_q1=now;
		        	ts=getFormedTime(now-start);
                    printf("%sms: p%d leaves Q1, time in Q1=%.3fms,token bucket now has %d token\n",
                    	ts,temp->id,((temp->departure_time_q1-temp->arrive_time_q1)/1000.0),bucket);
                    free(ts);
		        	My402ListAppend(&Q2,(void*)temp);
		        	gettimeofday(&curTime,NULL);
		        	now=curTime.tv_sec*1000000+curTime.tv_usec;
		        	temp->arrive_time_q2=now;
		        	ts=getFormedTime(now-start);
		        	printf("%sms: p%d enters Q2\n",ts,temp->id);
		        	free(ts);
		        	pthread_cond_signal(&cv);
		        
		     
		     	 } 	 

		     }

		 	 pthread_mutex_unlock(&lock);
		 	 gettimeofday(&curTime,NULL);
 	         long endTime=curTime.tv_sec*1000000+curTime.tv_usec;
 	         if(endTime-lastTime<token_interval*1000000)
		 	 usleep(token_interval*1000000);
 	 }
 //	 printf("bucket thread end\n");
 	 pthread_mutex_unlock(&lock);
 	 return (void*)NULL;
 }

 /*
  function for print emulation header information
 */
 void printParam(){
 	 arrival_interval=1/lambda;
 	 token_interval=1/r;
 	 service_interval=1/mu;
	 printf("Emulation Parameters:\n");
	 printf("number to arrive = %d\n",num);
	 printf("lambda = %f\n",lambda);
	 printf("mu = %f\n",mu);
	 printf("r = %f\n",r);
	 printf("B = %d\n",B);
	 printf("P = %d\n",P);
     printf("tsfile = %s\n",tsfile);
     printf("00000000.000ms: emulation begins\n");
     gettimeofday(&programStart,NULL);
    // printf("cur time:%ld\n",programStart.tv_usec);
     start=programStart.tv_sec*1000000+programStart.tv_usec;
 }

 /*
   select and set running param
 */
 void makeChoice(int argc,char* argv[]){
     if(argc==1){
     	 fprintf(stderr,"command line param number misformed/n");
     }
     int i;
     for(i=1;i<argc;i+=2){
     	 if(strcmp(argv[i],"-lambda")==0){
               lambda=atof(argv[i+1]);
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

/*
  program running frame work
*/
int main(int argc,char* argv[]){
    
    makeChoice(argc,argv);
    printParam();
    My402ListInit(&Q1);
    My402ListInit(&Q2);
    int id1=1;
    int id2=2;
    pthread_create(&packet_thread,0,packetFunction,(void*)NULL);
    pthread_create(&token_thread,0,bucketFunction,(void*)NULL);
    pthread_create(&server1_thread,0,serverFunction,(void*)(&id1));
    pthread_create(&server2_thread,0,serverFunction,(void*)(&id2));
    pthread_join(packet_thread,0);
    pthread_join(token_thread,0);
    pthread_join(server1_thread,0);
    pthread_join(server2_thread,0);
    printf("emulation ends\n");
   // pthread_mutex_unlock(&lock);
	return 0;
}