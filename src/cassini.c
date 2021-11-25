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
#include <time.h>

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

int main(int argc, char * argv[]) {
  errno = 0;
  
  char * minutes_str = "*";
  char * hours_str = "*";
  char * daysofweek_str = "*";
  char * pipes_directory = "./run/pipes/";
  
  int d_in=0;//bool if daysofweek is declared
  int m_in=0;//bool if minutes is declared
  int h_in=0;//bool if hours is declared

  uint16_t operation;//operation
  uint16_t conv;//htob16(operation) //in write request
  uint64_t taskid;//id de la tâche //in write request
  uint64_t conversion;//htobe(taskid) //in write request
  
  struct timing struct_timing;//timing //in request
  int t;//timing from strings value
  int i;//indice to commandline
  
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
  
   char *str_request = malloc(strlen(pipes_directory)+strlen(fifo_request)+1);
  str_request[strlen(pipes_directory)+strlen(fifo_request)]='\0';
  strcat(str_request,pipes_directory);
  strcat(str_request,fifo_request);
  
  int fd_request=open(str_request, O_CREAT | O_WRONLY);//OPEN
  free(str_request);
  if(fd_request<0){
	perror("open");
	goto error;
  }
  
  //GESTION REQUETE
  conv = htobe16(operation);//valeur htobe16(operation)
  uint32_t conv32;//valeur htobe32(uint32_t)
  uint64_t conv64;//valeur htobe64(uint64_t)
  commandline command;
  switch(operation){
  case (CLIENT_REQUEST_LIST_TASKS)://'LS'
    if(write(fd_request,&conv,sizeof(uint16_t))<0){
       goto error_request;
    }

    break;
    
  case (CLIENT_REQUEST_CREATE_TASK)://'CR'
    if(write(fd_request,&conv,sizeof(uint16_t))<0){
       goto error_request;
    }       t=timing_from_strings(&struct_timing,minutes_str,hours_str,daysofweek_str);//initialise struct_timing
    if(t<0)goto error_request;
    
    conv64 = htobe64(struct_timing.minutes);
    conv32 = htobe32(struct_timing.hours);

    if(write(fd_request,&conv64,sizeof(uint64_t))<0){
       goto error_request;
    } 
    if(write(fd_request,&conv32,sizeof(uint32_t))<0){
       goto error_request;
    } 
    if(write(fd_request,&(struct_timing.daysofweek),sizeof(uint8_t))<0){
       goto error_request;
    }    
    
    i=1;//In case -m -h -d option are defined
    if(d_in!=0)i+=2;
    if(h_in!=0)i+=2;
    if(m_in!=0)i+=2;
    
    if(argc-1<1) goto error_request;
    conv32 = htobe32(argc-i-3);
    if(write(fd_request, &conv32, sizeof(uint32_t))<0)goto error_request;
    
    for(int j=0;j<argc-i-3;j++){
	uint32_t taille = strlen(argv[j+i+3]);//taille de argv[pos]
	conv32 = htobe32(taille);//htobe32
	if(write(fd_request,&conv32, sizeof(uint32_t))<0)goto error_request;//ecriture de la taille du string
	if(write(fd_request,argv[j+i+3],taille)<0)goto error_request;//ecriture du string
    }
    break;
  case CLIENT_REQUEST_TERMINATE://'TM' *
   
    if(write(fd_request,&conv,sizeof(uint16_t))<0){//OPERATION
      goto error_request;
    }
    break;
  case CLIENT_REQUEST_REMOVE_TASK://'RM' *
    conversion = htobe64(taskid);
    if(write(fd_request,&conv,sizeof(uint16_t))<0){//OPERATION
       goto error_request;
    }
    if(write(fd_request,&conversion,sizeof(uint64_t))<0){//TASKID
       goto error_request;
    }
    break;
  case CLIENT_REQUEST_GET_TIMES_AND_EXITCODES://'TX' *
     conversion = htobe64(taskid);
     if(write(fd_request,&conv,sizeof(uint16_t))<0){//OPERATION
       goto error_request;
    }
    if(write(fd_request,&conversion,sizeof(uint64_t))<0){//TASKID
       goto error_request;
    }
    break;
  case CLIENT_REQUEST_GET_STDOUT://'SO' *
    conversion = htobe64(taskid);
    if(write(fd_request,&conv,sizeof(uint16_t))<0){//OPERATION
       goto error_request;
    }
    if(write(fd_request,&conversion,sizeof(uint64_t))<0){//TASKID
       goto error_request;
    }
    break;
  case CLIENT_REQUEST_GET_STDERR://'SE' *
    conversion = htobe64(taskid);
    if(write(fd_request,&conv,sizeof(uint16_t))<0){//OPERATION
       goto error_request;
    }
    if(write(fd_request,&conversion,sizeof(uint64_t))<0){//TASKID
       goto error_request;
    }
    break;
  }
  ///////////////////////::JARRIVE PAS ////////////////////
  int fd_reply=open("./run/pipes/saturnd-reply-pipe",O_RDONLY);
  
  //free(str_reply) <-pour free la chaine passé en argument
  if(fd_reply<0){
     goto error_request;
  }
  //NE PAS ENLEVER LE IF
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
        
		rd=read(fd_reply,&command.ARGV[k].chaine,sizeof(char)*size_str);
		if(rd<0)goto error_reply;

		printf("%.*s",(sizeof(char)*size_str),&command.ARGV[k].chaine);//PRINT on STDOUT //nom de la commande
        	if(k<size_str-1)printf(" ");//ESPACE a chaque commandes
        }
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
      printf("Erreur lors de la requete -r:");
      write(STDOUT_FILENO,&buf16tmp,sizeof(uint16_t));//PRINT on STDOUT //ERRCODE
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
      printf("Erreur lors de la requete -o:");
      write(STDOUT_FILENO,&buf16tmp,sizeof(uint16_t));//PRINT on STDOUT //ERRCODE
      goto error_reply;
    } 
    else {
      rd=read(fd_reply,&buf32tmp,sizeof(uint32_t));//STRING.LENGTH
      if(rd<0)goto error_reply;
      buf32tmp=htobe32(buf32tmp);
      	
      cmd_name = malloc(buf32tmp);//STRING

      rd=read(fd_reply,cmd_name,sizeof(char)*buf32tmp);//ecrit STRING dans 
      if(rd<0)goto error_reply;

      printf("%.*s",(sizeof(char)*buf32tmp),cmd_name);
    }
    free(cmd_name);
    break;
    
  case CLIENT_REQUEST_GET_STDERR:
    rd=read(fd_reply,&buf16tmp,sizeof(uint16_t));//REPTYPE
    if(rd<0)goto error_reply;
    buf16tmp=htobe16(buf16tmp);//htobe16
    
    if(SERVER_REPLY_ERROR==buf16tmp){//ERRNO
      rd=read(fd_reply,&buf16tmp,sizeof(uint16_t));//ERRCODE
      if(rd<0)goto error_reply;
      buf16tmp=htobe16(buf16tmp);//htobe16
      printf("Erreur lors de la requete -e:");
      write(STDOUT_FILENO,&buf16tmp,sizeof(uint16_t));//PRINT on STDOUT //ERRCODE
      goto error_reply;
    } 
    else {
      rd=read(fd_reply,&buf32tmp,sizeof(uint32_t));//STRING.LENGTH
      if(rd<0)goto error_reply;
      buf32tmp=htobe32(buf32tmp);//htobe32
	
      cmd_name = malloc(buf32tmp);//STRING
       
      rd=read(fd_reply,cmd_name,sizeof(char)*buf32tmp);
      if(rd<0)goto error_reply;
      printf("%.*s",(sizeof(char)*buf32tmp),cmd_name);//PRINT on STDOUT //nom de la commande
    }
    free(cmd_name);
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
    
	rd=read(fd_reply,&buf16tmp,sizeof(uint16_t));//TASKID
	if(rd<0)goto error_reply;
	buf16tmp=htobe16(buf16tmp);
	printf("%hu\n",buf16tmp);
	i++;//INCREMENTATION
	}
    }
    break;
  }
  free(time_buf);
  close(fd_reply);
  close(fd_request);
  
  //MANQUE EXIT CODE
  
  //faudrait aussi faire des goto error au lieu des exit direct
  return EXIT_SUCCESS;
  
 error_reply:
  free(time_buf);
  close(fd_reply);
 error_request:
  close(fd_request);
 error:
  if (errno != 0) perror("main");
  free(pipes_directory);
  pipes_directory = NULL;
  return EXIT_FAILURE;
}
