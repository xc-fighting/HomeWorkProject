#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
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
 //store current bucket size and packet solved
 int bucket=0;
 int token_sum=0;
 int solved=0;
 //mutext and signal
 pthread_mutex_t lock=PTHREAD_MUTEX_INITIALIZER;
 pthread_cond_t cv=PTHREAD_COND_INITIALIZER;
 //four thread handler
 pthread_t packet_thread;
 pthread_t token_thread;
 pthread_t server1_thread;
 pthread_t server2_thread;
 //thread for monitor
 pthread_t sig_thread;
 sigset_t set;
 int INTFlag=FALSE;
//struct for packet
 typedef struct tagpacket{
 	long birth_time;
 	long arrive_time_q1;
 	long departure_time_q1;
 	long arrive_time_q2;
 	long departure_time_q2;
 	long death_time;
 	int demand;
 	long servicet;
 	long interval;
 	int id;
 }packet;

//track file identifier
 FILE* fp=NULL;
 int hasFile=FALSE;

 //variables for statics
 float total_time_q1=0;
 float total_time_q2=0;
 float total_time_s1=0;
 float total_time_s2=0;
 int token_droped=0;
 int packet_droped=0;
 int packet_removed=0;
 float total_service=0;
 float total_interval=0;
 long last_arr_time=0;
 float total_emulation=0;
 float total_time_spend=0;
 double sum_standard=0;
//transfer a us long time to formated string
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
//read from track file and assembly the packet struct
 int readFromFile(packet* item){
 	  //first allocate 1024 space
      char* buffer=(char*)malloc(1024*sizeof(char));
      memset(buffer,0,1024);
      //get the current line into buffer
      fgets(buffer,1024,fp);
      //allocate an long arr to store all the line info
      long* arr=(long*)malloc(sizeof(long)*3);
      //long array index,current val and string index
      int index=0;
      long val=0;
      int i=0;
    
      for(i=0;buffer[i]!='\n';){
      	//meet with any ' ' or '\t' continue
      	  if(buffer[i]==' '||buffer[i]=='\t'){
      	  	   while(buffer[i]==' '||buffer[i]=='\t'){
      	  	   	    i++;
      	  	   }
      	  	   //store the value reset the val
      	  	   arr[index]=val;
      	  	   val=0;
      	  	   index++;
      	  }
      	  else{
      	  	   //when meet others character reurn false,remember to free the space
      	  	   if(buffer[i]<'0'||buffer[i]>'9'){
      	  	   	     free(buffer);
      	  	   	     free(arr);
      	  	         return FALSE;
		      	  }
		          else{
                      val=val*10+buffer[i]-'0';
                      i++;
		          }
      	  }
      	 
      }
      //remember to store the last value
      arr[index]=val;
      //set the information
      item->demand=arr[1];
      //directly store the value as us
      item->interval=arr[0]*1000;
      //store service time as ms
      item->servicet=arr[2];
      //don't forget to free space
      free(arr);
      free(buffer);
      return TRUE;
 }


void traverse(My402List* list,int id) {
	struct timeval curTime;
	char* ts=NULL;
	if (My402ListEmpty(list) == TRUE) {
		return;
	}
	else {
		My402ListElem* p = list->anchor.next;
		while (p != &(list->anchor)) {
			packet* temp=(packet*)(p->obj);
			gettimeofday(&curTime,NULL);
            long now=curTime.tv_sec*1000000+curTime.tv_usec;  
            ts=getFormedTime(now-start);                 
			printf("%sms: p%d removed from Q%d\n",ts,temp->id,id);
			free(ts);
			p = p->next;
			packet_removed++;
		}
	}
}
 void* monitor(){
 	int sig;
 	while(TRUE){
            sigwait(&set,&sig);
           
            	  pthread_mutex_lock(&lock);
			        printf("you have pressed Ctrl+C,program will end soon !\n");
			        pthread_cancel(packet_thread);
			        pthread_cancel(token_thread);
			        printf("Q1 size=%d\n",My402ListLength(&Q1));
			        printf("Q2 size=%d\n",My402ListLength(&Q2));
			        printf("%d \n",solved);
			        traverse(&Q1,1);
			        traverse(&Q2,2);
			        My402ListUnlinkAll(&Q1);
			        My402ListUnlinkAll(&Q2);
			        printf("Q1 size=%d\n",My402ListLength(&Q1));
			        printf("Q2 size=%d\n",My402ListLength(&Q2));
			        printf("%d \n",solved);
			        INTFlag=TRUE;
			        pthread_cond_broadcast(&cv);
			        pthread_mutex_unlock(&lock); 
			        break;
            
 	}
 	printf("signal monitor thread terminated\n");
 	
 	return (void*)NULL;
 }





/*
  function for two server program
  input param for identify the server id 
*/
 void* serverFunction(void* arg){
 	//get the server id 1 or 2
    int serverId=*((int*)arg);
    //curTime for get current running time stamp and ts for store time string
    struct timeval curTime;
    char* ts=NULL;
 	while(TRUE){
        //add the lock and wait for Q2 not empty signal,wait is atomic operation when get the signal return and lock again
        printf("%d server prepare to enter critical\n",serverId);
 		pthread_mutex_lock(&lock);
 		 printf("%d server enter critical\n",serverId);
 		while(My402ListEmpty(&Q2)){
 			//this part below is used for last server thread to exit
 			if((My402ListEmpty(&Q1)==TRUE&&My402ListEmpty(&Q2)==TRUE&&solved>=num) || (INTFlag==TRUE)){
 			   pthread_mutex_unlock(&lock);
 			   printf("%d end1\n",serverId);	
 			   pthread_cancel(sig_thread);
 			   return 0;
 			}
 			printf("%d server waiting for signal\n",serverId);
 			pthread_cond_wait(&cv,&lock);
 			printf("%d server get the signal\n",serverId);
 	//		
 		}
 		
                 //get the head point of Q2
			        My402ListElem* head=My402ListFirst(&Q2);
			        //get the head packet of Q2
			        packet* delItem=(packet*)(head->obj);
			        //delete the head from Q2
			        My402ListUnlink(&Q2,head);
			        //get current time stamp store in curTime
			        gettimeofday(&curTime,NULL);
			        //calculate the time in us
			        long now=curTime.tv_sec*1000000+curTime.tv_usec;
			        //we set the packet departure time of Q2 to that value
			        delItem->departure_time_q2=now;
			        //the used that stamp again for curtime format
			        ts=getFormedTime(now-start);
			        printf("%sms: p%d leaves Q2, time in Q2 = %.3fms\n",
			        	ts,delItem->id,((delItem->departure_time_q2-delItem->arrive_time_q2)/1000.0));
			        free(ts);
			        //we add solved packet once
			        solved++;
			        //then we calculate the time again for start service time
			        gettimeofday(&curTime,NULL);
			        now=curTime.tv_sec*1000000+curTime.tv_usec;
			        //set the service begin time to thata
			        long service_begin=now;
			        //get formated time stamp
			        ts=getFormedTime(now-start);
			        //if we have file
			        if(hasFile==TRUE){
			            printf("%sms: p%d begins service at S%d, requesting %ldms of service\n",ts,delItem->id,serverId,delItem->servicet);
			        }
			        else{
			        	printf("%sms: p%d begins service at S%d, requesting %.3fms of service\n",ts,delItem->id,serverId,service_interval*1000);
			        }
			        
			        free(ts);

 		    
 	   
        //if we see that server has solved all the packet
        if((My402ListEmpty(&Q1)==TRUE&&My402ListEmpty(&Q2)==TRUE&&solved>=num) || (INTFlag==TRUE)){
                //first return the lock
 				pthread_mutex_unlock(&lock);
 				//then tell others the Q2 is not empty signal. however this is used for check end
 				pthread_cond_signal(&cv);
 				//sleep pretent that packet is serviced
 				if(hasFile==FALSE){
 				    	usleep(service_interval*1000000);
 				}
 				else{
                        usleep(delItem->servicet*1000);
 				}
		        //get current time
		        gettimeofday(&curTime,NULL);
		        //get birth time of packet
		        long birth=delItem->birth_time;
		        int id=delItem->id;
		        free(delItem);
		        gettimeofday(&curTime,NULL);
		        long service_end=curTime.tv_sec*1000000+curTime.tv_usec;
		        ts=getFormedTime(service_end-start);
		        printf("%sms: p%d departs from S%d, service time=%.2fms, time in system=%.3fms\n",ts,id,serverId,((service_end-service_begin)/1000.0),(service_end-birth)/1000.0);
		        if(serverId==1){
		        	 total_time_s1+=service_end-service_begin;
		        }
		        else{
		        	 total_time_s2+=service_end-service_begin;
		        }
		        total_service+=service_end-service_begin;
		        total_time_spend+=service_end-birth;
		        
		        sum_standard+=((service_end-birth)/1000000.0f)*((service_end-birth)/1000000.0f);
		        free(ts);
		        printf("%d end2\n",serverId);
 				//finally exit
 				return 0;
 		}
 		//when it is not finished, first return the lock and get current time stamp
        pthread_mutex_unlock(&lock);
        if(INTFlag==TRUE){
        	return 0;
        }
        gettimeofday(&curTime,NULL);
        //pretend to be served
        if(hasFile==FALSE){
            usleep(service_interval*1000000);
        }
        else{
        	usleep(delItem->servicet*1000);
        }
        //same like above
        total_time_q1=delItem->departure_time_q1-delItem->arrive_time_q1;
        total_time_q2=delItem->departure_time_q2-delItem->arrive_time_q2;
        long birth=delItem->birth_time;
        int id=delItem->id;
        free(delItem);
        gettimeofday(&curTime,NULL);
        long service_end=curTime.tv_sec*1000000+curTime.tv_usec;
        ts=getFormedTime(service_end-start);
        printf("%sms: p%d departs from S%d, service time=%.2fms, time in system=%.3fms\n",ts,id,serverId,((service_end-service_begin)/1000.0),(service_end-birth)/1000.0);
        if(serverId==1){
        	 total_time_s1+=service_end-service_begin;
        }
        else{
        	 total_time_s2+=service_end-service_begin;
        }
        total_service+=service_end-service_begin;
        total_time_spend+=service_end-birth;
       
		 sum_standard+=((service_end-birth)/1000000.0f)*((service_end-birth)/1000000.0f);
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
    //the thread will only end when solved all
 	while(solved<num && !INTFlag){
 		//get the most start time of loop
 		gettimeofday(&curTime,NULL);
 		long lastTime=curTime.tv_sec*1000000+curTime.tv_usec;
 		pthread_mutex_lock(&lock);
 		//interval arrive sleep time
        int sleeptime=0;
        //if not full arrived
 		if(i<num){
 			//read file true?
 			//int flag=TRUE;
 			packet* item=(packet*)malloc(sizeof(packet));
 			//if has file then we set sleep time to that packet arrive interval time
	 		if(hasFile==TRUE){
	            //flag=
	            readFromFile(item);
	            usleep(1000);
	            sleeptime=item->interval;
	 		}
            else{
            	 sleeptime=arrival_interval*1000000;
            	 item->demand=P;
            	 item->servicet=service_interval*1000;
            	 item->interval=arrival_interval*1000;
            }
            if(item->demand>B){
            	 i++;
            	 solved++;
            	 packet_droped++;
            	 pthread_mutex_unlock(&lock);
            	 continue;
            }
            gettimeofday(&curTime,NULL);
	 	    long endTime=curTime.tv_sec*1000000+curTime.tv_usec;
	 	    //if there is a need to sleep then just sleep
	 	    if(endTime-lastTime<sleeptime){
	 	    	//before sleep return the lock to some others who need that
	 	    	pthread_mutex_unlock(&lock);
                usleep(sleeptime-(endTime-lastTime));
	 	    }
	 	    else{
	 	    	//if no need to sleep return first 
	 	    	pthread_mutex_unlock(&lock);
	 	    	usleep(1000);
	 	    	
	 	    }
	 	    //lock it again
	 	    pthread_mutex_lock(&lock);
	 	    //we set packet id and birth time
            item->id=i+1;
            gettimeofday(&curTime,NULL);
            long now=curTime.tv_sec*1000000+curTime.tv_usec;  
            item->birth_time=now; 
            ts=getFormedTime(now-start);           
            printf("%sms: p%d arrives,needs %d tokens,inter-arrival time=%.3fms\n",ts,item->id,item->demand,sleeptime/1000.0);  
            total_interval+=now-last_arr_time;
       //     printf("interval =%ld\n",now-last_arr_time);
            last_arr_time=now;
            free(ts);
            //then add it to Q1
            My402ListAppend(&Q1,(void*)item);
            gettimeofday(&curTime,NULL);
            now=curTime.tv_sec*1000000+curTime.tv_usec;
            item->arrive_time_q1=now;
            ts=getFormedTime(now-start);
            printf("%sms: p%d enters Q1\n",ts,item->id);
            free(ts);
            //arrived ++
            i++;
 		}
 		//first judge whether Q1 is empty
        if(My402ListEmpty(&Q1)==FALSE){
        	    //get the first element
        	    My402ListElem* head=My402ListFirst(&Q1);
		        packet* temp=(packet*)(head->obj);
		        //if current size of bucket fit the demand of packet
		     //   printf("now we have bucket %d\n",bucket);
		        if(bucket>=temp->demand){
		        	//minus corresponding buckets
		        	bucket-=temp->demand;
		            //get the header out of Q1
		        	My402ListUnlink(&Q1,head);
		        	//get time stamp set as departure time of Q1
		            gettimeofday(&curTime,NULL);
		        	long now=curTime.tv_sec*1000000+curTime.tv_usec;
		        	temp->departure_time_q1=now;
		        	ts=getFormedTime(now-start);
                    printf("%sms: p%d leaves Q1, time in Q1=%.3fms,token bucket now has %d token\n",
                    	ts,temp->id,((temp->departure_time_q1-temp->arrive_time_q1)/1000.0),bucket);
                    free(ts);
                    //then put it into Q2 and set the enter time of Q2
		        	My402ListAppend(&Q2,(void*)temp);
		        	gettimeofday(&curTime,NULL);
		        	now=curTime.tv_sec*1000000+curTime.tv_usec;
		        	temp->arrive_time_q2=now;
		        	ts=getFormedTime(now-start);
		        	printf("%sms: p%d enters Q2\n",ts,temp->id);
		        	free(ts);
		        	//tell the server thread that Q2 is no longer empty
		        	pthread_cond_signal(&cv);
		        
		        }
        }
       
 	    pthread_mutex_unlock(&lock);
 	    //get time stamp
 	  
      //  usleep(1000);
 	}
 //	printf("packet arrival thread end\n");
 //	pthread_mutex_unlock(&lock);
 	return (void*)NULL;
 }

/*
   function for generating tokens
*/
 void* bucketFunction(){
 	struct timeval curTime;
     char* ts=NULL;
     usleep(token_interval*1000000);
 	 while(solved<num&&!INTFlag){
             gettimeofday(&curTime,NULL);
 		     long lastTime=curTime.tv_sec*1000000+curTime.tv_usec;
             pthread_mutex_lock(&lock);
             token_sum++;
		     if(bucket<B){
		 			bucket++;
		 			gettimeofday(&curTime,NULL);
				 	long cur=curTime.tv_sec*1000000+curTime.tv_usec;
				 	ts=getFormedTime(cur-start);
				    printf("%sms: token t%d arrives,token bucket now have %d token\n",ts,token_sum,bucket);
				    free(ts);		
		 	 }
		     else{
		     	    token_droped++;
		     	    gettimeofday(&curTime,NULL);
				 	long cur=curTime.tv_sec*1000000+curTime.tv_usec;
				 	ts=getFormedTime(cur-start);
				    printf("%sms: token t%d arrives,dropped\n",ts,token_sum);
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
 	         if(endTime-lastTime<token_interval*1000000){
 	         	 usleep(token_interval*1000000-(endTime-lastTime));
 	         }
             


 	 }
 //	 printf("bucket token thread end\n");
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
	 printf("\tEmulation Parameters:\n");
	 printf("\tnumber to arrive = %d\n",num);
	 if(hasFile==FALSE){
	 	printf("\tlambda = %f\n",lambda);
	 	printf("\tmu = %f\n",mu);
	 }
	 printf("\tr = %f\n",r);
	 printf("\tB = %d\n",B);
	 if(hasFile==FALSE){
	 	 printf("\tP = %d\n",P);
	 }
	 if(hasFile==TRUE){
	 	printf("\ttsfile = %s\n",tsfile);
	 }
     printf("00000000.000ms: emulation begins\n");
     gettimeofday(&programStart,NULL);
    // printf("cur time:%ld\n",programStart.tv_usec);
     start=programStart.tv_sec*1000000+programStart.tv_usec;
     last_arr_time=start;
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
               fp=fopen(tsfile,"r");
               char* numstr=(char*)malloc(sizeof(char)*256);
               memset(numstr,0,256);
               fgets(numstr,255,fp);
               num=atoi(numstr);
            //   printf("read from file the number of p:%d\n",num);
               free(numstr);
               hasFile=TRUE;
     	 }
     }
     return;
}
void display(){
	printf("\n");
	printf("Statistics:\n");
	total_interval=total_interval/1000000.0;
	float average=total_interval/(solved-packet_droped);
	printf("average packet inter-arrival time= %f s\n",average);
	total_service=total_service/1000000.0;
	average=total_service/(solved-packet_droped);
	printf("average packet service time = %f s\n",average);
	printf("\n");
	printf("average number of packets in Q1 =%f\n",(float)total_time_q1/total_emulation);
	printf("average number of packets in Q2 = %f\n",(float)total_time_q2/total_emulation);
	printf("average number of packets in S1 = %f\n",(float)total_time_s1/total_emulation);
	printf("average number of packets in S2 = %f\n",(float)total_time_s2/total_emulation);
    printf("\n");
    total_time_spend=total_time_spend/1000000.0;
    average=total_time_spend/(solved-packet_droped);
    sum_standard=sum_standard/(solved-packet_droped);
    sum_standard=sum_standard*sum_standard;

    printf("average time a packet spent in system=%f s\n",average);
    if(sum_standard-average*average>=0){
    	printf("standard deviation for time spent in system=%f\n",sqrtf(sum_standard-average*average));
    }
    else{
    	printf("standard deviation for time spent in system=%f\n",sqrtf(average*average-sum_standard));
    }
    printf("\n");
    printf("token drop probability= %f\n",(float)token_droped/token_sum);
    printf("packet drop probability=%f\n",(float)packet_droped/solved);
}

/*
  program running frame work
*/
int main(int argc,char* argv[]){
    
    sigemptyset(&set);
    sigaddset(&set,SIGINT);
    sigprocmask(SIG_BLOCK,&set,0);
    makeChoice(argc,argv);
    printParam();
    My402ListInit(&Q1);
    My402ListInit(&Q2);
    int id1=1;
    int id2=2;
    pthread_create(&sig_thread,0,monitor,0);
    pthread_create(&packet_thread,0,packetFunction,(void*)NULL);
    pthread_create(&token_thread,0,bucketFunction,(void*)NULL);
    pthread_create(&server1_thread,0,serverFunction,(void*)(&id1));
    pthread_create(&server2_thread,0,serverFunction,(void*)(&id2));
    pthread_join(sig_thread,0);
    pthread_join(packet_thread,0);
    pthread_join(token_thread,0);
    pthread_join(server1_thread,0);
    pthread_join(server2_thread,0);
    printf("emulation ends\n");
    struct timeval curTime;
    gettimeofday(&curTime,NULL);
    long now=curTime.tv_sec*1000000+curTime.tv_usec;
    //printf("%d\n",now-start);
    total_emulation=now-start;
    display();
   // pthread_mutex_unlock(&lock);
	return 0;
}