#include "cassini.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "timing.h"
#include "timing-text-io.c"
#include <endian.h>
#include <time.h>
#include <inttypes.h>

#define BUFSIZE 1024

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
    uint32_t ARGC;
    string *ARGV;
  }commandline;
//FIN STRUCTURES
int wr_type_taskid(int fd_request,uint64_t taskid, char * buffer,unsigned int memindex){
  memcpy(buffer+memindex,&taskid,sizeof(uint64_t));//TASKID
  memindex+=sizeof(uint64_t);
  if(write(fd_request,buffer,memindex)<0){//WRITE BUFFER
     return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

int read_operation(int argc,char **argv,uint64_t taskid,uint16_t operation,int fd_request,char * minutes_str, char * hours_str,char * daysofweek_str, int p_in, int d_in , int h_in,int m_in){
  //GESTION REQUETE
  
  uint64_t conversion;//htobe(taskid) //in write request
  
  uint16_t conv = htobe16(operation);//value htobe16(operation)
  uint32_t conv32;//value htobe32(uint32_t)
  uint64_t conv64;//value htobe64(uint64_t)
  
  commandline command;
  struct timing struct_timing;//timing //in request
    
  int t;//timing from strings value
  int i;//indice to commandline
  
  char *buffer = malloc(BUFSIZE);
  memcpy(buffer,&conv,sizeof(uint16_t));//copy of the OPERATION in buffer
  unsigned int memindex=sizeof(uint16_t);//size of what we wrote in buffer
  
switch(operation){
  case (CLIENT_REQUEST_LIST_TASKS)://'LS'
    if(write(fd_request,buffer,memindex)<0){
       goto error_request;
    }
    break;
    
  case (CLIENT_REQUEST_CREATE_TASK)://'CR'
    t=timing_from_strings(&struct_timing,minutes_str,hours_str,daysofweek_str);//initialise struct_timing
    if(t<0)goto error_request;
    
    conv64 = htobe64(struct_timing.minutes);
    conv32 = htobe32(struct_timing.hours);

    memcpy(buffer+memindex,&conv64,sizeof(uint64_t));//copy minutes
    memindex+=sizeof(uint64_t);
    memcpy(buffer+memindex,&conv32,sizeof(uint32_t));//copy hours
    memindex+=sizeof(uint32_t);
    memcpy(buffer+memindex,&(struct_timing.daysofweek),sizeof(uint8_t));//copy days
    memindex+=sizeof(uint8_t);
    
    i=1;//In case -m -h -d option are defined
    if(p_in!=0)i+=2;
    if(d_in!=0)i+=2;
    if(h_in!=0)i+=2;
    if(m_in!=0)i+=2;
    if(argc-1<1) goto error_request;//If there is not enough arguments

    //COMMAND
    conv32 = htobe32(argc-i-1);
    
    memcpy(buffer+memindex,&conv32,sizeof(uint32_t));//copy number of arguments (of command)
    memindex+=sizeof(uint32_t);
    
    for(int j=0;j<argc-i-1;j++){
	uint32_t taille = strlen(argv[j+i+1]);//taille de argv[pos]
	conv32 = htobe32(taille);//htobe32
	memcpy(buffer+memindex,&conv32,sizeof(uint32_t));//copy size of argument at index
	memindex+=sizeof(uint32_t);
	memcpy(buffer+memindex,argv[j+i+1],taille);//copy argument at index
	memindex+=taille;
    }
    if(write(fd_request,buffer,memindex)<0){//WRITE BUFFER
      goto error_request;
    }
    break;
  case CLIENT_REQUEST_TERMINATE://'TM' *
    if(write(fd_request,buffer,memindex)<0){//WRITE BUFFER
      goto error_request;
    }
    break;
  case CLIENT_REQUEST_REMOVE_TASK://'RM' *
    if(wr_type_taskid(fd_request,htobe64(taskid),buffer,memindex)==EXIT_FAILURE)goto error_request;
    break;
  case CLIENT_REQUEST_GET_TIMES_AND_EXITCODES://'TX' *
    if(wr_type_taskid(fd_request,htobe64(taskid),buffer,memindex)==EXIT_FAILURE)goto error_request;
    break;
  case CLIENT_REQUEST_GET_STDOUT://'SO' *
    if(wr_type_taskid(fd_request,htobe64(taskid),buffer,memindex)==EXIT_FAILURE)goto error_request;
    break;
  case CLIENT_REQUEST_GET_STDERR://'SE' *
    if(wr_type_taskid(fd_request,htobe64(taskid),buffer,memindex)==EXIT_FAILURE)goto error_request;
    break;
  }
  free(buffer);
  return EXIT_SUCCESS;
  error_request:
  close(fd_request);
  error:
  if (errno != 0) perror("main1");
  
  free(buffer);
  return EXIT_FAILURE;
}

int write_operation(uint16_t operation,int fd_reply){
  
  struct timing tmps;
  char *time_buf=malloc(TIMING_TEXT_MIN_BUFFERSIZE);
  if(time_buf==NULL)goto error_reply;
  
  uint64_t buf64tmp;
  uint32_t buf32tmp;
  uint16_t buf16tmp;

  uint16_t hto_buf2;
  
  char *cmd_name;

  int rd;//READ value
  int wr;//WRITE value
  
  commandline command;
  
  switch(operation){	  
  case (CLIENT_REQUEST_LIST_TASKS):
    rd=read(fd_reply,&buf16tmp,sizeof(uint16_t));//REPTYPE
    if(rd<0)goto error_reply;
	buf16tmp=htobe16(buf16tmp);//htobe16
	
	rd=read(fd_reply,&buf32tmp,sizeof(uint32_t));//NBTASKS
	
	if(rd<0)goto error_reply;
	buf32tmp=htobe32(buf32tmp);//htobe32
	
	int i=0;
	while(i<buf32tmp){
	  rd=read(fd_reply,&buf64tmp,sizeof(uint64_t));//TASKID
	  if(rd<0)goto error_reply;
	  buf64tmp=htobe64(buf64tmp);
	  printf("%lu: ",buf64tmp);//PRINT on STDOUT // uint64_t
	  
	  uint64_t min;//MINUTES
	  uint32_t heure;//HEURES
	  uint8_t jours;//JOURS
	  
	  rd=read(fd_reply,&min,sizeof(uint64_t));//ecrit dans MINUTES 
	  if(rd<0)goto error_reply;
	  rd=read(fd_reply,&heure,sizeof(uint32_t));//ecrit dans HEURES
	  if(rd<0)goto error_reply;
	  rd=read(fd_reply,&jours,sizeof(uint8_t));//ecrit dans JOURS
	  if(rd<0)goto error_reply;
	  
	  min=htobe64(min);//htobe64
	  heure=htobe32(heure);//htobe32
	  
	  tmps.minutes=min;
	  tmps.hours=heure;
	  tmps.daysofweek=jours;
	  
	  int sz=timing_string_from_timing(time_buf,&tmps);
	  printf("%.*s ",sz,time_buf);//PRINT on STDOUT //time_buf
	  
	  uint32_t size;//ARGC
	  uint32_t size_str;//STRING.LENGTH
	  
	  rd=read(fd_reply,&size,sizeof(uint32_t));//ARGC ecrit dans size
	  
	  if(rd<0)goto error_reply;
	  size=htobe32(size);
	  command.ARGC=size;
	  
	  command.ARGV=malloc(size*sizeof(string));
	  
	  for(int k=0;k<size;k++){
	    rd=read(fd_reply,&size_str,sizeof(uint32_t));//STRING.LENGTH ecrit dans size_str
	    if(rd<0)goto error_reply;
	    size_str=htobe32(size_str);

	    command.ARGV[k].L=size_str;
	    command.ARGV[k].chaine=malloc(size_str*sizeof(char));
	    
	    rd=read(fd_reply,(&command.ARGV[k])->chaine,sizeof(char)*size_str);
	    if(rd<0)goto error_reply;
	    
	    printf("%.*s",(int)(sizeof(char)*size_str),(&command.ARGV[k])->chaine);//PRINT on STDOUT //nom de la commande
	    printf(" ");//ESPACE a chaque commandes
	    free((&command.ARGV[k])->chaine);
	    
	  }
	  
	  free(command.ARGV);
	  i++;//INCREMENTE
	  printf("\n");//SAUT DE LIGNE
	}
	break;	  
	
  case (CLIENT_REQUEST_CREATE_TASK) :
    rd=read(fd_reply,&buf16tmp,sizeof(uint16_t));//REPTYPE
    if(rd<0)goto error_reply;
    buf16tmp=htobe16(buf16tmp);//htobe16
    
    rd=read(fd_reply,&buf64tmp,sizeof(uint64_t));//TASKID
    if(rd<0)goto error_reply;
    
    buf64tmp = htobe64(buf64tmp);//htobe
    printf("%lu\n",buf64tmp);//PRINT on STDOUT //TASKID
    break;
    
  case CLIENT_REQUEST_REMOVE_TASK:
    rd=read(fd_reply,&buf16tmp,sizeof(uint16_t));//REPTYPE
    if(rd<0)goto error_reply;
    buf16tmp=htobe16(buf16tmp);//htobe16
    
    if(SERVER_REPLY_ERROR==buf16tmp){//ERRNO
      rd=read(fd_reply,&buf16tmp,sizeof(uint16_t));//ERRCODE
      if(rd<0)goto error_reply;
      buf16tmp=htobe16(buf16tmp);//htobe16
      printf("Erreur lors de la requete -r: %hu",buf16tmp);//PRINT on STDOUT //ERRCODE
      goto error_reply;
    }
    break;
    
  case CLIENT_REQUEST_GET_STDOUT:
    rd=read(fd_reply,&buf16tmp,sizeof(uint16_t));//REPTYPE
    if(rd<0)goto error_reply;
    buf16tmp=htobe16(buf16tmp);//htobe16
    
    if(SERVER_REPLY_ERROR==buf16tmp){//ERRNO
      rd=read(fd_reply,&buf16tmp,sizeof(uint16_t));//ERRCODE
      if(rd<0)goto error_reply;
      buf16tmp=htobe16(buf16tmp);//htobe16
      printf("Erreur lors de la requete -o: %hu",buf16tmp);//PRINT on STDOUT //ERRCODE
      goto error_reply;
    } 
    else {
      rd=read(fd_reply,&buf32tmp,sizeof(uint32_t));//STRING.LENGTH
      if(rd<0)goto error_reply;
      buf32tmp=htobe32(buf32tmp);
      
      cmd_name = malloc(buf32tmp);//STRING
      
      rd=read(fd_reply,cmd_name,sizeof(char)*buf32tmp);//ecrit STRING dans 
      if(rd<0)goto error_reply;
      
      printf("%.*s",(int)(sizeof(char)*buf32tmp),cmd_name);
      free(cmd_name);
    }
    
    break;
    
  case CLIENT_REQUEST_GET_STDERR:
    rd=read(fd_reply,&buf16tmp,sizeof(uint16_t));//REPTYPE
    if(rd<0)goto error_reply;
    buf16tmp=htobe16(buf16tmp);//htobe16
    
    if(SERVER_REPLY_ERROR==buf16tmp){//ERRNO
      rd=read(fd_reply,&buf16tmp,sizeof(uint16_t));//ERRCODE
      if(rd<0)goto error_reply;
      buf16tmp=htobe16(buf16tmp);//htobe16
      printf("Erreur lors de la requete -e: %hu",buf16tmp);//PRINT on STDOUT //ERRCODE
      goto error_reply;
    } 
    else {
      rd=read(fd_reply,&buf32tmp,sizeof(uint32_t));//STRING.LENGTH
      if(rd<0)goto error_reply;
      buf32tmp=htobe32(buf32tmp);//htobe32
      
      cmd_name = malloc(buf32tmp);//STRING
      
      rd=read(fd_reply,cmd_name,sizeof(char)*buf32tmp);
      if(rd<0)goto error_reply;
      printf("%.*s",(int)(sizeof(char)*buf32tmp),cmd_name);//PRINT on STDOUT //nom de la commande
      free(cmd_name);
    }
    
    break;
    
  case CLIENT_REQUEST_TERMINATE:
    rd=read(fd_reply,&buf16tmp,sizeof(uint16_t));//REPTYPE
    if(rd<0)goto error_reply;
    buf16tmp=htobe16(buf16tmp);//htobe16	
    break;
    
  case CLIENT_REQUEST_GET_TIMES_AND_EXITCODES:
    rd=read(fd_reply,&buf16tmp,sizeof(uint16_t));//REPTYPE
    if(rd<0)goto error_reply;
    buf16tmp=htobe16(buf16tmp);//htobe16
    if(SERVER_REPLY_ERROR==buf16tmp){//ERRNO
      rd=read(fd_reply,&buf16tmp,sizeof(uint16_t));//ERRCODE
      if(rd<0)goto error_reply;
      printf("Erreur dans la requete -x: %hu",buf16tmp);
      goto error_reply;
    } else {
      rd=read(fd_reply,&buf32tmp,sizeof(uint32_t));//NBTASKS
      if(rd<0)goto error_reply;
      buf32tmp=htobe32(buf32tmp);//NBTASKS
      
      
      
      i=0;
      while(i<buf32tmp){
	int64_t timing;
	rd=read(fd_reply,&timing,sizeof(int64_t));//TIME
	if(rd<0)goto error_reply;
	
	timing=htobe64(timing);//htobe64
	time_t realtime= (time_t)(timing/1000000);//
	
        time_t timestamp = time(NULL);
        struct tm * pTime = localtime(&timing);
	
        char buffer[50];
        
        strftime( buffer, 50, "%Y-%m-%d %H:%M:%S", pTime );
        printf("%s ", buffer );
	
	rd=read(fd_reply,&buf16tmp,sizeof(uint16_t));//ERRCODE
	if(rd<0)goto error_reply;
	buf16tmp=htobe16(buf16tmp);
	printf("%hu\n",buf16tmp);
	
	i++;//INCREMENTATION
      }
    }
    break;
  }
  free(time_buf);
  return EXIT_SUCCESS;
  error_reply:
  free(time_buf);
  return EXIT_FAILURE;
}
int main(int argc, char * argv[]) {
  errno = 0;
  
  char * minutes_str = "*";
  char * hours_str = "*";
  char * daysofweek_str = "*";
  char * username_str = getlogin();
  char * pipes_directory = NULL;


  int p_in=0;//bool if pipe is declared
  int d_in=0;//bool if daysofweek is declared
  int m_in=0;//bool if minutes is declared
  int h_in=0;//bool if hours is declared

  uint16_t operation;//operation
  uint64_t taskid;//id de la tâche //in write request
  
  char *fifo_request = "/saturnd-request-pipe";
  char *fifo_reply = "/saturnd-reply-pipe";
  
  int opt;
  char * strtoull_endp;
  //INITIALISATION
  while ((opt = getopt(argc, argv, "hlcqm:H:d:p:r:x:o:e:")) != -1) {
    switch (opt) {
    case 'm':
      minutes_str = optarg;
      m_in=1;
      break;
    case 'H':
      hours_str = optarg;
      h_in=1;
      break;
    case 'd':
      daysofweek_str = optarg;
      d_in=1;
      break;
    case 'p':
      pipes_directory = strdup(optarg);
      if (pipes_directory == NULL) goto error;
      p_in=2;
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

  if(pipes_directory==NULL){//In case the directory is not defined by option -p
    pipes_directory = malloc(256);//creation of the default PIPES_DIR
    strcpy(pipes_directory, "/tmp/");
    strcat(pipes_directory, username_str);
    strcat(pipes_directory, "/saturnd/pipes");
        

  }
  
  char *str_request = malloc(strlen(pipes_directory)+strlen(fifo_request)+1);
  strcpy(str_request,pipes_directory);
  strcat(str_request,fifo_request);

  
  int fd_request=open(str_request,  O_WRONLY);//OPEN
  free(str_request);
  if(fd_request<0){
	perror("open");
	goto error;
  }

  //GESTION REQUETE
  if(read_operation(argc,argv,taskid,operation,fd_request,minutes_str,hours_str,daysofweek_str,p_in,d_in,h_in,m_in)==EXIT_FAILURE)
  goto error_request;
  
  char *str_reply = malloc(strlen(pipes_directory)+strlen(fifo_reply)+1);
  strcpy(str_reply,pipes_directory);
  strcat(str_reply,fifo_reply);
  
  int fd_reply=open(str_reply,  O_RDONLY);//OPEN

  free(str_reply); //free la chaine passé en argument
  if(fd_reply<0){
     goto error_request;
  }
  
  if(write_operation(operation,fd_reply)==EXIT_FAILURE)goto error_reply;
  
  //FREE
  free(pipes_directory);
  
  close(fd_reply);
  close(fd_request);
  
  //MANQUE EXIT CODE
  return EXIT_SUCCESS;
  
 error_reply:
  close(fd_reply);
 error_request:
  close(fd_request);
 error:
  if (errno != 0) perror("main");
  free(pipes_directory);
  pipes_directory = NULL;
  return EXIT_FAILURE;
}
