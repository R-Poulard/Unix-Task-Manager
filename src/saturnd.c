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
#include <dirent.h>
#include <errno.h>
#include <sys/wait.h>

int parsec (char *global,char* subside,char del){
     int last_one=-1;
     int i=0;
     while(global[i]!='\0'){
	if(global[i]==del){
        last_one=i;
        }
        i++;    
     }
     if (last_one==-1){
        exit(1);
     }
     else{
       subside=malloc(sizeof(char)*strlen(global+last_one+1));
       strncpy(subside,global+last_one+1,strlen(global+last_one+1));
       global[last_one]='\0';
       exit(0);
     }
}

int compaire_cron(int NOSMINUTES,char* minutes){
    minutes;
     char *current;
     while(parsec(minutes,current,',')!=0){
       char* siunder;      
       if(parsec(current,siunder,'-')!=0){
          if(atoi(current)==NOSMINUTES){
             free(current);
             free(minutes);
             return 1;
          }
       }
       else{
        int petit =atoi(current);
        int grand=atoi(siunder);
        if(petit<=NOSMINUTES && NOSMINUTES <= grand){
           free(current);
           free(minutes);
           free(siunder);
           return 1;
         }
       }
    }
    return 0;
}

//if(compaire_cron(minutes,cron_min) && compaire_cron(heures,cron_heur) && compaire_cron(day,cron_day)){}//A METTRE DANS LE CODE




int main(){
  char * pipes_directory;
  char * username_str = getlogin();
  
  pipes_directory = malloc(256);//creation of the default PIPES_DIR//MALLOC
  strcpy(pipes_directory, "/tmp/");
  strcat(pipes_directory, username_str);
  strcat(pipes_directory, "/saturnd/pipes");
  
  char *fifo_request=malloc(256);//MALLOC
  strcat(fifo_request,pipes_directory);
  strcat(fifo_request,"/saturnd-request-pipe");
  
  char *fifo_reply = malloc(256);//MALLOC
  strcat(fifo_reply,pipes_directory);
  strcat(fifo_reply,"/saturnd-reply-pipe");  
  
  int fd_request = mkfifo(fifo_request,S_IRWXU);
  int fd_reply = mkfifo(fifo_reply, S_IRWXU);
  
  int pip[2];
  pipe(pip);
  
  int taskmax=open("taskid_max",O_CREAT | O_EXCL | O_RDWR );
  //if(taskmax<0 && errno!=EEXIST)goto;
  
  pid_t pid = fork();
  //if(pid<0)goto;
  if(pid>0){//PÃ¨re
  
  
  
  
  
  
  }
  
  
  else
  {
  //Fils
  char * pathtasks = malloc(256);//MALLOC
  strcat(pathtasks,"/tmp/");
  strcat(pathtasks,username_str);
  strcat(pathtasks,"/saturnd/tasks");
  
  mkdir(pathtasks,O_CREAT | O_EXCL | O_RDWR);
  
  DIR * dir=opendir(pathtasks);
  struct dirent * struct_dir;
  
  time_t time2 = time(NULL);
  uint16_t code;
  while(read(pip[0],&code,sizeof(uint16_t))>0){
  
  pid_t pid2=fork();
  
  if(pid2>0){
    sleep(60);
    time_t time3=time(NULL);
  } else {
  while((struct_dir=readdir(dir))){
    DIR * taskdir = opendir(struct_dir->d_name);
    char * pathtime = malloc(256);//MALLOC
    strcat(pathtime,pathtasks);
    strcat(pathtime,"/time_exec");
    
    int fd_time=open(pathtime,O_RDONLY);//OPEN
    uint64_t minutes;
    uint32_t heures;
    uint8_t jours;
    
    int rd=read(fd_time,&minutes,sizeof(uint64_t));//READ
    rd=read(fd_time,&heures,sizeof(uint32_t));//READ
    rd=read(fd_time,&jours,sizeof(uint8_t));//READ
    
    char * buf_minutes=malloc(TIMING_TEXT_MIN_BUFFERSIZE);//MALLOC
    char * buf_heures=malloc(TIMING_TEXT_MIN_BUFFERSIZE);//MALLOC
    char * buf_jours=malloc(TIMING_TEXT_MIN_BUFFERSIZE);//MALLOC
    
    struct timing timing;
    //Minutes
    timing_string_from_field(buf_minutes, 0, 59, &minutes);
    // Hours
    timing_string_from_field(buf_heures, 0, 23, &heures);
    // Days of the week
    timing_string_from_field(buf_jours, 0, 6, &jours);
    
    char * buf_avant=malloc(TIMING_TEXT_MIN_BUFFERSIZE);
    int i=0;
    int y=0;
    int to_exec=0;
    while(buf_minutes[i]!='\0'){//
        
    	if(buf_minutes[i]=','){
    	  char * buf_avant2=malloc(TIMING_TEXT_MIN_BUFFERSIZE);
          int i=0;
          int y=0;
          while(buf_minutes[i]!='\0'){
    	if(buf_minutes[i]=','){
    	  
    	}
    	else {
    	buf_avant[y]=buf_minutes[i];
    	y++;
    	}
    	i++;
    }
    	}
    	else {
    	buf_avant[y]=buf_minutes[i];
    	y++;
    	}
    	i++;
    }
    if(to_exec){
      char * to_cmd = malloc(50);//MALLOC
      strcat(to_cmd,pathtasks);
      strcat(to_cmd,"argv");

      pid_t pid=fork();
      
      if(pid==0){
        int fd_cmd=open(to_cmd,O_RDONLY);
        char * cmd;
        uint32_t cmd_argc;
        uint32_t * buf32;
        char ** cmd_argv;
      
        int rd=read(fd_cmd,&cmd_argc,sizeof(uint32_t));//READ
        cmd_argc=htobe32(cmd_argc);
        cmd_argv=malloc(cmd_argc*sizeof(char *));
        int x=0;
      
        rd=read(fd_cmd,buf32,sizeof(uint32_t));//READ
        cmd=malloc(buf32+1);
        
        rd=read(fd_cmd,cmd,buf32);//READ
        cmd[*(buf32)]='\0';
      
        for(int i=0;i<cmd_argc-1;i++){
          rd=read(fd_cmd,buf32,sizeof(uint32_t));//READ
          cmd_argv[i]=malloc(buf32+1);
        
          rd=read(fd_cmd,cmd_argv[i],buf32);//READ
          cmd_argv[*(buf32)]='\0';
        }
        cmd_argv[cmd_argc-1]=NULL;
        close(fd_cmd);
        
        char * cmd_out = malloc(50);//MALLOC
        strcat(cmd_out,pathtasks);
        strcat(cmd_out,"stdout");
        
        char * cmd_err = malloc(50);//MALLOC
        strcat(cmd_err,pathtasks);
        strcat(cmd_err,"error");
        
        char * cmd_time = malloc(50);//MALLOC
        strcat(cmd_time,pathtasks);
        strcat(cmd_time,"time");
        
        int fd_out=open(cmd_out,O_WRONLY);//OPEN
        int fd_err=open(cmd_err,O_WRONLY);//OPEN
        int fd_time=open(cmd_time,O_WRONLY);//OPEN
        
        write(fd_time,&time2,sizeof(int64_t));//WRITE
        dup2(fd_out,STDOUT_FILENO);//DUP
        dup2(fd_err,STDERR_FILENO);//DUP
        
        execvp(cmd,cmd_argv);//EXEC
        } else {
          wait(NULL);
        }
     }
     }
  }
  }
  while (wait(NULL) > 0);
  closedir(dir);
  free(pathtasks);
  exit(EXIT_SUCCESS);
  }
}