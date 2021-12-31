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
#include "server-reply.h"
#include "time-comp.c"
#include "assert.h"
#include "client-request.h"

#define CMD_ARGV "argv"
#define CMD_TEXEC "time_exec"
#define CMD_EXIT "exit"
#define CMD_STDOUT "stdout"
#define CMD_STDERR "stderr"

int main(){
  char * pipes_directory;
  char * username_str = getlogin();
  
  printf("Username = %s\n",username_str);
  pipes_directory = malloc(256);//creation of the default PIPES_DIR//MALLOC
  strcpy(pipes_directory, "/tmp/");
  strcat(pipes_directory, username_str);
  mkdir(pipes_directory,0751);//MKDIR
  
  strcat(pipes_directory, "/saturnd");
  mkdir(pipes_directory,0751);//MKDIR
  
  strcat(pipes_directory, "/pipes");
  mkdir(pipes_directory,0751);//MKDIR
  
  char *fifo_request=malloc(256);//MALLOC
  strcpy(fifo_request,pipes_directory);
  strcat(fifo_request,"/saturnd-request-pipe");
  
  char *fifo_reply = malloc(256);//MALLOC
  strcpy(fifo_reply,pipes_directory);
  strcat(fifo_reply,"/saturnd-reply-pipe");  
  
  //PATHTASKS
  char * pathtasks_saturnd = malloc(256);//MALLOC
    strcpy(pathtasks_saturnd,"./saturnd_dir");
    printf("Mkdir saturn = %s\n",pathtasks_saturnd);
    mkdir(pathtasks_saturnd, 0751);//MKDIR
    
    char * pathtasks = malloc(256);//MALLOC
    strcpy(pathtasks,"./saturnd_dir/tasks/");
    
    mkdir(pathtasks,0751);//MKDIR
    //
  struct stat stat_request;
  struct stat stat_reply;
  
  if(stat(fifo_request,&stat_request)==-1 || S_ISFIFO(stat_request.st_mode)==0){
    mkfifo(fifo_request,0751);//MKFIFO
  }
  if(stat(fifo_reply,&stat_reply)==-1 || S_ISFIFO(stat_reply.st_mode)==0){
    mkfifo(fifo_reply,0751);//MKFIFO
  }
  int pip[2];
  pipe2(pip,O_NONBLOCK);//PIPE
  
  char * buftaskmax = malloc(50);//MALLOC
  strcpy(buftaskmax,pathtasks_saturnd);
  strcat(buftaskmax,"taskid_max");
  int taskmax=open(buftaskmax,O_CREAT | O_EXCL | O_RDWR );
  //if(taskmax<0 && errno!=EEXIST)goto;

  pid_t pid = fork();
  //if(pid<0)goto;
  if(pid>0){//GRAND PERE
    sleep(30);
    
    int fd_request=open(fifo_request,O_RDONLY);
    int fd_reply=open(fifo_reply,O_WRONLY);
    
    int out=0;
    uint16_t u16;
    uint32_t u32;
    uint64_t taskid;
    uint64_t taskmax_name;
    
    char * buf;
    int fd_buftask;
    
    while(read(fd_request,&u16,sizeof(uint16_t))==0 && out==0){
      u16=htobe16(u16);
      switch (u16){
        case CLIENT_REQUEST_REMOVE_TASK:
        
        read(fd_request,&taskid,sizeof(uint64_t));
        taskid=htobe64(taskid);
        //
        char * pathtasks_id = malloc(256);//MALLOC
        strcpy(pathtasks_id,pathtasks);
        strcat(pathtasks_id,taskid);
          
        DIR * dir=opendir(pathtasks);
        if(dir==NULL){
          buf=malloc(2*sizeof(uint16_t));
          uint16_t uerror= SERVER_REPLY_ERROR;
          uint16_t uerror_type= SERVER_REPLY_ERROR_NOT_FOUND;
          memcpy(buf,&uerror,sizeof(uint16_t));
          memcpy(buf+sizeof(uint16_t),&uerror_type,sizeof(uint16_t));
          write(fd_reply,buf,2*sizeof(uint16_t));
          free(buf);
        } else {
        
        buf =malloc(256);
        strcpy(buf,pathtasks);
        strcat(buf,CMD_ARGV);//argv
        remove(buf);//Remove
        
        strcpy(buf,pathtasks);
        strcat(buf,CMD_TEXEC);//time exit
        remove(buf);//Remove
        
        strcpy(buf,pathtasks);
        strcat(buf,CMD_EXIT);//exit code
    	remove(buf);//Remove
    	
    	strcpy(buf,pathtasks);
        strcat(buf,CMD_STDOUT);//stdout
    	remove(buf);//Remove
    	
    	strcpy(buf,pathtasks);
        strcat(buf,CMD_STDERR);//stderr
    	remove(buf);//Remove
    	
    	uint16_t ok = SERVER_REPLY_OK;
    	write(fd_reply,&ok,sizeof(uint16_t));
    	}
        break;
        case CLIENT_REQUEST_CREATE_TASK:
          fd_buftask=open(buftaskmax,O_RDONLY | O_WRONLY);
          
          if(fd_buftask==-1){
            taskmax_name = (unsigned long) 1;
            open(buftaskmax, O_CREAT | O_WRONLY);
            write(fd_buftask,&taskmax_name,sizeof(uint64_t));
          } else {
            read(fd_buftask,&taskmax_name,sizeof(uint64_t));
            taskmax_name=taskmax_name+(unsigned long) 1;
            write(fd_buftask,&taskmax_name,sizeof(uint64_t));
          }
          //cr
          char * pathtasks_file=malloc(100);//MALLOC
          strcpy(pathtasks_file,pathtasks);
          strcat(pathtasks_file,taskmax_name);
            
          char * pathtasks_timexec=malloc(100);//MALLOC
          strcpy(pathtasks_timexec,pathtasks_file);
          strcat(pathtasks_timexec,CMD_TEXEC);
            
          char * pathtasks_argv=malloc(100);//MALLOC
          strcpy(pathtasks_argv,pathtasks_file);
          strcat(pathtasks_argv,CMD_ARGV);
            
          mkdir(pathtasks_file,0751);//file
          int fd_timexec=open(pathtasks_timexec,O_CREAT | O_WRONLY);//file
          int fd_argv=open(pathtasks_argv,O_CREAT | O_WRONLY);

          char * temps=malloc(sizeof(uint64_t)+sizeof(uint32_t)+sizeof(uint8_t));
            
          read(fd_request,temps,sizeof(uint64_t)+sizeof(uint32_t)+sizeof(uint8_t));//READ
          write(fd_timexec,temps,sizeof(uint64_t)+sizeof(uint32_t)+sizeof(uint8_t));//WRITE
            
          //COMMAND
            
          read(fd_request,&u32,sizeof(uint32_t));//READ argc
          u32 = htobe32(u32);
          write(fd_argv,&u32,sizeof(uint32_t));

          for(int j=0;j<u32;j++){
	    uint32_t taille;//taille de argv[pos]
	    read(fd_request,&taille,sizeof(uint32_t));//READ argv[i] size
	    taille = htobe32(taille);//htobe32
	      
	    char * str_argv=malloc(sizeof(char)*taille);
	    read(fd_request,str_argv,sizeof(char)*taille);//READ argv[i] size
	      
	    write(fd_argv,&taille,sizeof(uint32_t));
	    write(fd_argv,str_argv,sizeof(char)*taille);
          }
          
          char *buf=malloc(sizeof(uint16_t)+sizeof(uint64_t));
          uint16_t ok_reply= SERVER_REPLY_OK;
          memcpy(buf,&ok_reply,sizeof(uint16_t));
          memcpy(buf+sizeof(uint16_t),&taskmax_name,sizeof(uint64_t));
          write(fd_reply,buf,sizeof(uint16_t)+sizeof(uint64_t));
          free(buf);
        break;
        case CLIENT_REQUEST_TERMINATE:
          u16=NO_EXIT_CODE;
          write(pip[1],&u16,sizeof(uint16_t));
          out=1;
        break;
      }
    }
    printf("A écrit\n");
    wait(NULL);
    uint16_t ok = SERVER_REPLY_OK;
    write(fd_reply,&ok,sizeof(uint16_t));
    close(fd_reply);
    close(fd_request);
  }//FIN GRAND PERE
  else
  {//DEBUT "PERE"
    
    DIR * dir=opendir(pathtasks);
    
    struct dirent * struct_dir;

    time_t time2 = time(NULL);
    uint16_t code;
    
    while(read(pip[0],&code,sizeof(uint16_t))<=0 && errno==EAGAIN ){
      printf("ErrNo = %s\n",strerror(errno));
      pid_t pid2=fork();
      if(pid2>0){//PERE
        sleep(10);
        waitpid(pid2,NULL,WNOHANG);
        time2=time(NULL);
        printf("Coucou3\n");
      }//FIN PERE
      else
      {//PETIT FILS
      	
        struct tm *ptime = localtime(&time2); 
        printf("PETIT FILS chemin = %s\n",pathtasks);
        
        while((struct_dir=readdir(dir)) && strcmp(struct_dir->d_name,".")!=0 && strcmp(struct_dir->d_name,"..")!=0){//WHILE TASKS
          printf("PETIT FILS dans WHILE TASKS\n");
          DIR * taskdir = opendir(struct_dir->d_name);
          char * pathtime = malloc(256);//MALLOC
          strcpy(pathtime,pathtasks);
          strcat(pathtime,CMD_TEXEC);
    
          int fd_time=open(pathtime,O_RDONLY);//OPEN
          uint64_t minutes;
          uint32_t heures;
          uint8_t jours;
          lseek(fd_time,  0,  SEEK_SET);//Tête de lecture au début du fichier pour lire time_exec
          
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
    
          if(compaire_cron(ptime->tm_min,buf_minutes) && compaire_cron(ptime->tm_hour,buf_heures) && compaire_cron(ptime->tm_wday,buf_jours)){
            char * to_cmd = malloc(50);//MALLOC
            strcpy(to_cmd,pathtasks);
            strcat(to_cmd,"argv");

            pid_t pid=fork();
            if(pid==0){

               printf("Debut lecture command\n");
  	lseek(to_cmd,  0,  SEEK_SET);//Tête de lecture au début du fichier pour lire cmd_argc
            uint32_t cmd_argc;

  char ** cmd_argv;
  printf("Avant read fd_cmd\n");
              
  int rd=read(to_cmd,&cmd_argc,sizeof(uint32_t));//READ
  perror("read");
  printf("argc av htobe :%d\n", cmd_argc);
  cmd_argc=htobe32(cmd_argc);

  char ** tab;//=malloc(sizeof(char *) * cmd_argc);//MALLOC
  int tmp = sizeof(char *) * cmd_argc;
  tab = malloc(tmp+1);
  assert(tab!=NULL);
             /* printf("CALC : %d\n",tmp);
              printf("taille char* :%d\n", sizeof(char*));
              printf("taille totale* :%d\n", sizeof(tab));
              printf("argc apr htobe :%d\n", cmd_argc);
              printf("apres tab\n");
            */  
  for(int i=0;i<cmd_argc;i++){
    printf("i=%d\n",i);
               
    uint32_t taille_str;
    rd=read(to_cmd,&taille_str,sizeof(uint32_t));//READ L
    taille_str = htobe32(taille_str);
    printf("at i taille=%d\n",taille_str);
    // printf("at i rd=%d\n",rd);
               
    char * str=malloc(taille_str);
    rd=read(to_cmd,str,sizeof(char)*taille_str);//READ char *
    printf("J'ai lu %s de taille %d!\n",str,taille_str);
    //printf("at i rd=%d\n",rd);
    char * str_final=malloc(taille_str+1);
    cpy(str,str_final,taille_str);
    tab[i]=malloc(sizeof(char*)*taille_str+1);
    strncpy(tab[i],str_final,taille_str+1);
    printf("tab[%d]=%s\n",i,tab[i]);
  }
  tab[tmp]=NULL;
  if(tab[tmp]==NULL)printf("\nNULL\n");         
  //tab contient tout les argv
              
  int fd_out=open(CMD_STDOUT,O_CREAT|O_WRONLY | O_TRUNC, 0666);//OPEN
  int fd_err=open(CMD_STDERR,O_CREAT|O_WRONLY | O_TRUNC,0666);//OPEN
  dup2(fd_out,STDOUT_FILENO);//DUP
  dup2(fd_err,STDERR_FILENO);//DUP
              
  execvp(tab[0],tab);//EXEC   
            }
            else
            {
              char * cmd_exit = malloc(50);//MALLOC
              strcat(cmd_exit,pathtasks);
              strcat(cmd_exit,CMD_EXIT);
              
              int fd_exit=open(cmd_exit,O_CREAT | O_WRONLY | O_APPEND);//OPEN
              
              printf("Coucou6\n");
              int status;
              waitpid(pid2,&status,0);
              if(WIFEXITED(status)){
                int wstatus = WEXITSTATUS(status);//exit code
                write(fd_exit,(uint16_t)&wstatus,sizeof(uint16_t));//WRITE
              } else {
                uint16_t ex=NO_EXIT_CODE;
                write(fd_exit,&ex,sizeof(uint16_t));//WRITE
              }
              
              printf("Coucou7\n");
            }
          }
        }//FIN WHILE TASKS
        printf("PETIT FILS _FIN WHILE\n");
        exit(1);
      }//FIN PETIT FILS
    }
    printf("Je suis sorti du while =3\n");
    while (wait(NULL) > 0){
      printf("Wait child\n");
    }
    printf("Coucou8\n");
    closedir(dir);
    free(pathtasks);
    exit(EXIT_SUCCESS);
  }
}
