#include "../include/cassini.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "../include/timing.h"
#include "../src/timing-text-io.c"
#include <endian.h>

const char usage_info[] = "\
   usage: cassini [OPTIONS] -l -> list all tasks\n\
      or: cassini [OPTIONS]    -> same\n\
      or: cassini [OPTIONS] -q -> terminate the daemon\n\
      or: cassini [OPTIONS] -c [-m MINUTES] [-H HOURS] [-d DAYSOFWEEK] COMMAND_NAME [ARG_1] ... [ARG_N]\n\
          -> add a new task and print its TASKID\n\
             format & semantics of the \"timing\" fields defined here:\n\
             https://pubs.opengroup.org/onlinepubs/9699919799/utilities/crontab.html\n\
             default value for each field is \"*\"\n\
      or: cassini [OPTIONS] -r TASKID -> remove a task\n\
      or: cassini [OPTIONS] -x TASKID -> get info (time + exit code) on all the past runs of a task\n\
      or: cassini [OPTIONS] -o TASKID -> get the standard output of the last run of a task\n\
      or: cassini [OPTIONS] -e TASKID -> get the standard error\n\
      or: cassini -h -> display this message\n\
\n\
   options:\n\
     -p PIPES_DIR -> look for the pipes in PIPES_DIR (default: /tmp/<USERNAME>/saturnd/pipes)\n\
";
 //STRUCTURES
  typedef struct string {
    uint32_t L;
    char * chaine;
  }string;
  typedef struct commandline {
    int ARGC;
    string *ARGV;
  }commandline;
//FIN STRUCTURES

int main(int argc, char * argv[]) {
  errno = 0;
  
  char * minutes_str = "*";
  char * hours_str = "*";
  char * daysofweek_str = "*";
  char * pipes_directory = "/home/emma/Documents/Cours/2021/SYS5/sy5-projet-2021-2022/run/pipes/";
  
  uint16_t operation = CLIENT_REQUEST_LIST_TASKS;
  uint64_t taskid;
  
  int opt;
  char * strtoull_endp;
  
  //VARIABLES
  char * fichier_test = "/tmp/fichier.txt";
  char * fifo_request = "/saturnd-request-pipe";
  char * fifo_reply = "/saturnd-reply-pipe";
  
  char *str_request = malloc(strlen(pipes_directory)+strlen(fifo_request)+1);
  str_request[strlen(pipes_directory)+strlen(fifo_request)]='\0';
  strcat(str_request,pipes_directory);
  strcat(str_request,fifo_request);
  
  int fd=open(fichier_test/*str_request*/, O_CREAT | O_WRONLY);
	if(fd<0){
	perror("open");
	exit(EXIT_FAILURE);
	}
  struct timing *time;//timing
  void *buffer;
  void *buf;
  void *buf3;
  uint16_t bigE;
  commandline *command;
  string *tab;
  int t;//timing from strings value
  int i;//indice
  char *pointeur;
  //FIN VARIABLES
  
  while ((opt = getopt(argc, argv, "hlcqm:H:d:p:r:x:o:e:")) != -1) {
    switch (opt) {
    case 'm':
      minutes_str = optarg;
      break;
    case 'H':
      hours_str = optarg;
      break;
    case 'd':
      daysofweek_str = optarg;
      break;
    case 'p':
      pipes_directory = strdup(optarg);
      if (pipes_directory == NULL) goto error;
      break;
    case 'l':
      operation = CLIENT_REQUEST_LIST_TASKS;
      break;
    case 'c':
      operation = CLIENT_REQUEST_CREATE_TASK;
      break;
    case 'q':
      operation = CLIENT_REQUEST_TERMINATE;
      break;
    case 'r':
      operation = CLIENT_REQUEST_REMOVE_TASK;
      taskid = strtoull(optarg, &strtoull_endp, 10);
      if (strtoull_endp == optarg || strtoull_endp[0] != '\0') goto error;
      break;
    case 'x':
      operation = CLIENT_REQUEST_GET_TIMES_AND_EXITCODES;
      taskid = strtoull(optarg, &strtoull_endp, 10);
      if (strtoull_endp == optarg || strtoull_endp[0] != '\0') goto error;
      break;
    case 'o':
      operation = CLIENT_REQUEST_GET_STDOUT;
      taskid = strtoull(optarg, &strtoull_endp, 10);
      if (strtoull_endp == optarg || strtoull_endp[0] != '\0') goto error;
      break;
    case 'e':
      operation = CLIENT_REQUEST_GET_STDERR;
      taskid = strtoull(optarg, &strtoull_endp, 10);
      if (strtoull_endp == optarg || strtoull_endp[0] != '\0') goto error;
      break;
    case 'h':
      printf("%s", usage_info);
      return 0;
    case '?':
      fprintf(stderr, "%s", usage_info);
      goto error;
    }
  }
  // --------
  // | TODO |
  // --------

  //SWITCH ENVOI DEMON
  switch(operation){
  case (CLIENT_REQUEST_LIST_TASKS)://'LS' *
    if(fd<0){
       exit(EXIT_FAILURE);
    }
    if(write(fd,&operation,sizeof(uint16_t))<0){
       exit(EXIT_FAILURE);
    }
    break;
  case (CLIENT_REQUEST_CREATE_TASK)://'CR'
    time=malloc(sizeof(struct timing));//struct timing
    if(time==NULL)exit(EXIT_FAILURE);//exit si le malloc est mauvais
    buffer=malloc(sizeof(uint16_t)+sizeof(struct timing)+sizeof(commandline));//buffer
    if(buffer==NULL) exit(EXIT_FAILURE);
    
    t=timing_from_strings(time,minutes_str,hours_str,daysofweek_str);
    if(t<0)exit(EXIT_FAILURE);
   
    command=malloc(sizeof(commandline));//command
    if(command==NULL) exit(EXIT_FAILURE);
   
    i=0;//In case -m -h -d option are defined
    if(strcmp(minutes_str,"*")!=0)i+=2;
    if(strcmp(hours_str,"*")!=0)i+=2;
    if(strcmp(daysofweek_str,"*")!=0)i+=2;
    
    if(argc-1<1) exit(EXIT_FAILURE);
    
    command->ARGC=argc-i-2;//le nombre d'argument - le temps e - "cassini -l"
    
    tab=malloc(sizeof(string)*(argc-i-1));
    for(int j=0;j<argc-i-2;j++){
      tab[j].chaine=argv[j+i+2];
      tab[j].L=strlen(tab[j].chaine);
    }
    printf("ARGC-i-2 = %d\n",argc-i-2);
    for(int k=0;k<(argc-i-1);k++){
      
      printf("k=%d, %d\n",k,tab[k].L);
      for(int m=0;m<tab[k].L;m++){
	printf("%c",tab[k].chaine[m]);
      }
      if(tab[k].L==0)printf("NULL\n");
      printf("\n");
    }
    command->ARGV=tab;
    //On alloue au buffer les différents arguments
    memcpy(buffer,&operation,sizeof(uint16_t));//HTOBE
    memcpy(buffer+sizeof(uint16_t),time,sizeof(struct timing));
    memcpy(buffer+sizeof(uint16_t)+sizeof(time),command,sizeof(commandline));
    
    //printf("%jx\n",operation);
    
    if(write(fd,buffer,sizeof(buffer))<0){
		perror("write");
		printf("ca\n");
      exit(EXIT_FAILURE);
    }
    free(buffer);
    free(time);
    free(command);
    free(tab);
    break;
  case CLIENT_REQUEST_TERMINATE://'TM' *
    if(fd<0){
      exit(EXIT_FAILURE);
    }
    if(write(fd,&operation,sizeof(uint16_t))<0){
      exit(EXIT_FAILURE);
    }
    break;
  case CLIENT_REQUEST_REMOVE_TASK://'RM' *
    buffer=malloc(sizeof(uint16_t)+sizeof(uint64_t));
    //On alloue au buffer les différents arguments
    memcpy(buffer,&operation,sizeof(uint16_t));
    memcpy(buffer+sizeof(uint16_t),&taskid,sizeof(uint64_t));
    if(write(fd,buffer,sizeof(uint16_t)+sizeof(uint64_t))<0){//uint64_t est converti dans fd (utiliser des valeurs > 33)
       exit(EXIT_FAILURE);
    }
    
    free(buffer);
    break;
  case CLIENT_REQUEST_GET_TIMES_AND_EXITCODES://'TX' *
    buffer=malloc(sizeof(uint16_t)+sizeof(uint64_t));
    //On alloue au buffer les différents arguments
    memcpy(buffer,&operation,sizeof(uint16_t));
    memcpy(buffer+sizeof(uint16_t),&taskid,sizeof(uint64_t));
    if(write(fd,buffer,sizeof(uint16_t)+sizeof(uint64_t))<0){
       exit(EXIT_FAILURE);
    }
    free(buffer);
    break;
  case CLIENT_REQUEST_GET_STDOUT://'SO' *
    buffer=malloc(sizeof(uint16_t)+sizeof(uint64_t));
    //On alloue au buffer les différents arguments
    memcpy(buffer,&operation,sizeof(uint16_t));
    memcpy(buffer+sizeof(uint16_t),&taskid,sizeof(uint64_t));
    if(write(fd,buffer,sizeof(uint16_t)+sizeof(uint64_t))<0){
       exit(EXIT_FAILURE);
    }
    free(buffer);
    break;
  case CLIENT_REQUEST_GET_STDERR://'SE' *
    buffer=malloc(sizeof(uint16_t)+sizeof(uint64_t));
    //On alloue au buffer les différents arguments
    memcpy(buffer,&operation,sizeof(uint16_t));
    memcpy(buffer+sizeof(uint16_t),&taskid,sizeof(uint64_t));
    if(write(fd,buffer,sizeof(uint16_t)+sizeof(uint64_t))<0){
       exit(EXIT_FAILURE);
    }
    free(buffer);
    break;
  }
  printf("ICI \n");
  char *buffer2 = malloc(1024);
  char *str = malloc(strlen(pipes_directory)+strlen(fifo_reply)+1);
  str[strlen(pipes_directory)+strlen(fifo_reply)]='\0';
  
  //Ajout du chemin vers le fichier dans str
  strcat(str,pipes_directory);
  strcat(str,fifo_reply);
  
  printf("%s\n",str);
  int fd_reply=open(str,O_RDONLY);
  if(fd<0){
	   perror("open");
	   exit(EXIT_FAILURE);
  }
  
  uint16_t buf2;
  
  int rd;
  int wr;
  uint16_t *er=SERVER_REPLY_ERROR;
  
  switch(operation){
	  
	  case CLIENT_REQUEST_LIST_TASKS:
	  buf2=malloc(sizeof(uint16_t));
	  rd=read(fd_reply,&buf2,sizeof(uint16_t));//REPTYPE
	  //wr=write(STDOUT_FILENO,buf2,sizeof(uint16_t));//OK
	  //if(wr<0)goto error;

	  buf3 = malloc(sizeof(uint32_t));
	  rd=read(fd_reply,&buf3,sizeof(uint32_t));//TASKID
	  printf("apres : %p\n",buf3);
	  uint32_t *nb=htobe32(buf3);
	  wr=write(fifo_request,nb,sizeof(uint32_t));

	  free(buf3);
	  printf("titi \n");

	  int i=0;
	  while(i<*nb){
		  buf3=malloc(sizeof(uint64_t));
		  rd=read(fd_reply,&buf3,sizeof(uint64_t));
		  write(fifo_request,buf3,sizeof(uint64_t));
		  free(buf3);
		  
		  buf3=malloc(sizeof(struct timing));
		  rd=read(fd_reply,&buf3,sizeof(struct timing));
		  char *str_buf3=malloc(sizeof(TIMING_TEXT_MIN_BUFFERSIZE+1));
		  timing_string_from_timing(str_buf3,buf3);
		  write(fifo_request,str_buf3,TIMING_TEXT_MIN_BUFFERSIZE);
		  free(buf3);
		  
		  commandline *buf4 = malloc(sizeof(commandline));
		  rd=read(fd_reply,&buf3,sizeof(commandline));
		  for(int i=0;i<buf4->ARGC;i++){
			  for(int y=0;y<buf4->ARGV[i].L;y++){
				 write(fifo_request,buf4->ARGV[i].chaine[y],sizeof(char));
				}
				write(fifo_request," ",sizeof(char));
		  }
		  i++;
		  write(fifo_request,"\n",sizeof(char));
	  }
	  break;
	  
	  
  case CLIENT_REQUEST_CREATE_TASK :
    buf2=malloc(sizeof(uint64_t));
    rd=read(fd_reply,&buf2,sizeof(uint16_t));//REPTYPE
    printf("pas blok");
    //wr=write(STDOUT_FILENO,buf2,sizeof(uint16_t));
    //if(wr<0)goto error;
    rd=read(fd_reply,&buf2,sizeof(uint64_t));//TASKID
    wr=write(STDOUT_FILENO,buf2,sizeof(uint64_t));
    //
    break;
  case CLIENT_REQUEST_REMOVE_TASK:
    buf2=malloc(sizeof(uint16_t));
    rd=read(fd_reply,&buf2,sizeof(uint16_t));//REPTYPE
    //write(STDOUT_FILENO,buf2,sizeof(uint16_t));//
    uint16_t *test=SERVER_REPLY_ERROR;
    uint16_t *hto_buf=htobe16(buf2);
    if(test==hto_buf){
      rd=read(fd_reply,&hto_buf,sizeof(uint16_t));//ERRCODE
      write(STDOUT_FILENO,&hto_buf,sizeof(uint16_t));
    }
    break;
      case CLIENT_REQUEST_GET_STDOUT:
	buf2=malloc(sizeof(uint32_t));
    rd=read(fd_reply,&buf2,sizeof(uint16_t));//REPTYPE
    //write(STDOUT_FILENO,buf2,sizeof(uint16_t));
    printf("\n");
    uint16_t *hto_buf2=htobe16(buf2);
    string *str_buf;
    if(er==hto_buf2){
      rd=read(fd_reply,&hto_buf2,sizeof(uint16_t));//ERRCODE
      write(STDOUT_FILENO,&hto_buf2,sizeof(uint16_t));
      printf("\n - \n");
    } 
    else {
      rd=read(fd_reply,&str_buf,sizeof(string));//<string>
      for(int i=0;i<str_buf->L;i++){
		write(STDOUT_FILENO,&str_buf->chaine[i],sizeof(char));
      }
      printf("\n");
    }
    break;
  case CLIENT_REQUEST_GET_STDERR:
    buf2=malloc(sizeof(uint32_t));
    rd=read(fd_reply,&buf2,sizeof(uint16_t));//REPTYPE
    //write(STDOUT_FILENO,buf2,sizeof(uint16_t));
    
    uint16_t *hto_buf3=htobe16(buf2);
    string *str_buf2;
    if(er==hto_buf3){
      rd=read(fd_reply,&hto_buf3,sizeof(uint16_t));//ERRCODE
      write(STDOUT_FILENO,&hto_buf3,sizeof(uint16_t));
    } 
    else {
      rd=read(fd_reply,&str_buf2,sizeof(string));//<string>
      for(int i=0;i<str_buf2->L;i++){
		write(STDOUT_FILENO,&str_buf2->chaine[i],sizeof(char));
      }
      
    }
    break;
    
  case CLIENT_REQUEST_TERMINATE:
    rd=read(fd_reply,&buf2,sizeof(uint16_t));//REPTYPE
    write(STDOUT_FILENO,buf2,sizeof(uint16_t));
    break;
  }
  printf("fin SW BUFFER =%ju\n",&buf2);
  
  return EXIT_SUCCESS;
  
 error:
  if (errno != 0) perror("main");
  free(pipes_directory);
  pipes_directory = NULL;
  return EXIT_FAILURE;
}

