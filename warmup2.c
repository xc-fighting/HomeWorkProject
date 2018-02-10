#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <pthread.h>
#include "my402list.h"

 int lambda;
 float mu;
 int r;
 int B;
 int P;
 int num;
 char* tsfile;

pthread_mutex_t lock=PTHREAD_MUTEX_INITIALIZER;
void printParam(){
	 printf("Emulation Parameters:\n");
	 printf("number to arrive = %d\n",num);
	 printf("lambda = %d\n",lambda);
	 printf("mu = %f\n",mu);

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
               r=atoi(argv[i+1]);
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
	return 0;
}