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


int cpy(char * buf1,char *buf2,size_t size){
  strncpy(buf2,buf1,size);
  buf2[size]='\0';
  return 1;
}


int get_std(int fd_request ,int fd_reply, int type, char *pathtasks){
	
	uint64_t id;
	char * buf;
	read(fd_request,&id, sizeof(uint64_t));
	id = htobe64(id);
	
	char * pathtasks_file=malloc(100);//MALLOC
    strcpy(pathtasks_file,pathtasks);
	strcat(pathtasks_file,id);
	
	DIR * dir = opendir(pathtasks_file);
	if(dir==NULL){
		buf=malloc(2*sizeof(uint16_t));
        uint16_t uerror= SERVER_REPLY_ERROR;
        uint16_t uerror_type= SERVER_REPLY_ERROR_NOT_FOUND;
        memcpy(buf,&uerror,sizeof(uint16_t));
        memcpy(buf+sizeof(uint16_t),&uerror_type,sizeof(uint16_t));
        write(fd_reply,buf,2*sizeof(uint16_t));
        free(buf);
	}else{
		
		char * pathtasks_std=malloc(100);//MALLOC
		strcpy(pathtasks_std,pathtasks_file);
		if(type){
			strcat(pathtasks_std,CMD_STDOUT);
		}else{
			strcat(pathtasks_std,CMD_STDERR);
		}
		
		int fd_std = open(pathtasks_std, O_RDONLY);
		if(fd_std== -1){
			buf=malloc(2*sizeof(uint16_t));
			uint16_t uerror= SERVER_REPLY_ERROR;
			uint16_t uerror_type= SERVER_REPLY_ERROR_NEVER_RUN;
			memcpy(buf,&uerror,sizeof(uint16_t));
			memcpy(buf+sizeof(uint16_t),&uerror_type,sizeof(uint16_t));
			write(fd_reply,buf,2*sizeof(uint16_t));
			free(buf);
		}else{
			
			char *buffer = malloc(1024);
			int rd;
			int taille_buff = 1024;
			uint32_t taille = 0;
			while(rd = read(fd_std, buffer+taille_buff - 1024, 1024) !=0 ){
				//if(rd==-1)go to
				taille = taille+rd;
				if(rd == taille_buff){
					
					buffer = realloc(buffer, taille_buff + 1024);
					taille_buff = taille_buff + 1024;
				}
			}
			
			taille = htobe32(taille);
			uint16_t ok = SERVER_REPLY_OK;
			
			char *buff_total = malloc(sizeof(uint16_t)+ sizeof(uint32_t)+taille);
			memcpy(buff_total,&ok, sizeof(uint16_t));
			memcpy(buff_total+sizeof(uint16_t), &taille , sizeof(uint32_t));
			mempcpy(buff_total+sizeof(uint16_t)+sizeof(uint32_t), buffer, taille);
			write(fd_reply, buff_total,sizeof(uint16_t)+ sizeof(uint32_t)+taille);
			
		}
		
		
	}
	
}

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
  strcat(buftaskmax,"/");
  strcat(buftaskmax,"taskid_max");
  int taskmax=open(buftaskmax,O_CREAT | O_EXCL | O_RDWR );
  //if(taskmax<0 && errno!=EEXIST)goto;

  pid_t pid = fork();
  //if(pid<0)goto;
  		

  if(pid>0){//GRAND PERE
    //sleep(30);
    printf("AVANT\n");
    int fd_request=open(fifo_request,O_RDONLY);
    int fd_reply=open(fifo_reply,O_WRONLY);
    
    int out=0;
    uint16_t u16;
    uint32_t u32;
    uint64_t taskid;
    uint64_t taskmax_name;
    
    char * buf;
    int fd_buftask;
    struct dirent *  struct_dir;
    printf("OUT\n");
    int rd;
    while(out==0){
		rd = read(fd_request,&u16,sizeof(uint16_t));
		if(rd>0){
			printf("IN\n");
		printf("rd %d\n", rd);
		
      u16=htobe16(u16);
      switch (u16){
        case CLIENT_REQUEST_REMOVE_TASK:
        
        read(fd_request,&taskid,sizeof(uint64_t));//READ du TASKID
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
        strcpy(buf,pathtasks);//ou pathtask_id ?
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
    	
    	//remove potentiellement le repertoire tasks/task_id ?:
    	//rmdir(pathtasks_id);
    	
    	uint16_t ok = SERVER_REPLY_OK;
    	write(fd_reply,&ok,sizeof(uint16_t));
    	}
        break;
        case CLIENT_REQUEST_CREATE_TASK:
          fd_buftask=open(buftaskmax,O_RDWR);//exclusifs je crois O_RDWR
          
          if(fd_buftask==-1){
            taskmax_name = (unsigned long) 1;
            printf("ICI \n");
            fd_buftask = open(buftaskmax, O_CREAT | O_WRONLY,0777);
            chmod(buftaskmax, 0777);
                        printf("IF av av :%lu\n", taskmax_name);

            write(fd_buftask,&taskmax_name,sizeof(uint64_t));
            close(fd_buftask);
          } else {
            read(fd_buftask,&taskmax_name,sizeof(uint64_t));
            printf("ELSE av av :%lu\n", taskmax_name);
            
            printf("ELSE av :%lu\n", taskmax_name);
            taskmax_name=taskmax_name+(unsigned long) 1;
            

            
            printf("ELSE  ap:%lu\n", taskmax_name);
            write(fd_buftask,&taskmax_name,sizeof(uint64_t));
            close(fd_buftask);
          }
          //cr
          
          printf("%lu",taskmax_name);

          char * pathtasks_file=malloc(100);//MALLOC
          
          //ulltoa(taskmax_name, task_char,2);
          strcpy(pathtasks_file,pathtasks);
          snprintf(pathtasks_file + strlen(pathtasks), 100,"%lu",taskmax_name );

            
          char * pathtasks_timexec=malloc(100);//MALLOC
          strcpy(pathtasks_timexec,pathtasks_file);
          strcat(pathtasks_timexec,"/");
          strcat(pathtasks_timexec,CMD_TEXEC);
            
          char * pathtasks_argv=malloc(100);//MALLOC
          strcpy(pathtasks_argv,pathtasks_file);
          strcat(pathtasks_argv,"/");
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
          taskmax_name = htobe64(taskmax_name);
			
          memcpy(buf,&ok_reply,sizeof(uint16_t));
                    printf("4\n");

          memcpy(buf+sizeof(uint16_t),&taskmax_name,sizeof(uint64_t));
                    printf("5\n");

          write(fd_reply,buf,sizeof(uint16_t)+sizeof(uint64_t));
                    printf("6\n");

          free(buf);
                    printf("7\n");

        break;
        case CLIENT_REQUEST_TERMINATE:
          u16=NO_EXIT_CODE;
          write(pip[1],&u16,sizeof(uint16_t));
          out=1;
        break;
        
        case CLIENT_REQUEST_GET_STDOUT:
		  get_std(fd_request ,fd_reply, 1,pathtasks);
        break;
        
        case CLIENT_REQUEST_GET_STDERR: 
          get_std(fd_request ,fd_reply, 0,pathtasks);
        break;
        
        case CLIENT_REQUEST_GET_TIMES_AND_EXITCODES:
		
		read(fd_request,&taskid, sizeof(uint64_t));
		taskid = htobe64(taskid);
		
		pathtasks_file=malloc(100);//MALLOC
		strcpy(pathtasks_file,pathtasks);
		strcat(pathtasks_file,taskid);
		
		dir = opendir(pathtasks_file);
		if(dir==NULL){
			buf=malloc(2*sizeof(uint16_t));
			uint16_t uerror= SERVER_REPLY_ERROR;
			uint16_t uerror_type= SERVER_REPLY_ERROR_NOT_FOUND;
			memcpy(buf,&uerror,sizeof(uint16_t));
			memcpy(buf+sizeof(uint16_t),&uerror_type,sizeof(uint16_t));
			write(fd_reply,buf,2*sizeof(uint16_t));
			free(buf);
		}else{
			
			char * pathtasks_std=malloc(100);//MALLOC
			strcpy(pathtasks_std,pathtasks_file);
			strcat(pathtasks_std, "/");
			strcat(pathtasks_std, CMD_EXIT);
			
			int fd_std = open(pathtasks_std, O_RDONLY);
			
			char *buffer = malloc(1024);
				int rd;
				int taille_buff = sizeof(int64_t)+sizeof(uint16_t);
				uint32_t taille = 0;
				
				while(rd = read(fd_std, buffer+taille_buff, sizeof(int64_t)+sizeof(uint16_t) !=0 )){
					//if(rd==-1)go to
					taille ++;
					buffer = realloc(buffer, taille_buff + sizeof(int64_t)+sizeof(uint16_t));
					taille_buff = taille_buff + sizeof(int64_t)+sizeof(uint16_t);
					
				}
				taille = htobe32(taille);
				uint16_t ok = SERVER_REPLY_OK;
				
				char *buff_total = malloc(sizeof(uint16_t)+ sizeof(uint32_t)+taille * (sizeof(int64_t)+sizeof(uint16_t)));
				memcpy(buff_total,&ok, sizeof(uint16_t));
				memcpy(buff_total+sizeof(uint16_t), &taille , sizeof(uint32_t));
				
				mempcpy(buff_total+sizeof(uint16_t)+sizeof(uint32_t), buffer, taille * (sizeof(int64_t)+sizeof(uint16_t)));
				
				write(fd_reply, buff_total,sizeof(uint16_t)+ sizeof(uint32_t)+taille * (sizeof(int64_t)+sizeof(uint16_t)));
		}        
        break;
        
        case CLIENT_REQUEST_LIST_TASKS:
        
        dir = opendir(pathtasks);
        char *buf_all_tasks =malloc(1024);
        int taille_globale = 1024;
        int taille_utilisee = 0;
        uint32_t nb_tasks = 0;
        uint16_t ok = SERVER_REPLY_OK;
        
        while((struct_dir=readdir(dir)) && strcmp(struct_dir->d_name,".")!=0 && strcmp(struct_dir->d_name,"..")!=0){
			char *buf_inter = malloc(sizeof(uint64_t)+sizeof(uint64_t)+ sizeof(uint32_t)+sizeof(uint8_t));
			taskid = strtoull(struct_dir->d_name, NULL, 0);
			
			memcpy(buf_inter, &taskid, sizeof(uint64_t));
			
			char * pathtasks_exec=malloc(100);//MALLOC
			strcpy(pathtasks_exec,struct_dir->d_name);
			strcat(pathtasks_exec,"/");
			strcat(pathtasks_exec,CMD_TEXEC);
			int fd_time_exec = open(pathtasks_exec , O_RDONLY);
			
			read(fd_time_exec,buf_inter+sizeof(uint64_t) , sizeof(uint64_t)+ sizeof(uint32_t)+sizeof(uint8_t));
			
		    char * pathtasks_argv=malloc(100);//MALLOC
			strcpy(pathtasks_argv,struct_dir->d_name);
			strcat(pathtasks_argv,"/");
			strcat(pathtasks_argv,CMD_ARGV);
			
			int fd_argv = open(pathtasks_argv, O_RDONLY);
			
			uint32_t argc;
			read(fd_argv, &argc, sizeof(uint32_t));
			argc = htobe32(argc);
			char *buf_cmd  = malloc(sizeof(uint32_t));
			size_t taille_buff = sizeof(uint32_t);
			
			int rd;
			uint32_t count;
			memcpy(buf_cmd, &argc, sizeof(uint32_t));
			
			for(int i=0;i<argc;i++){
				uint32_t taille_str;
				rd=read(fd_argv,&taille_str,sizeof(uint32_t));//READ L
				taille_str = htobe32(taille_str);
				count+= taille_str;
				printf("at i taille=%d\n",taille_str);
				// printf("at i rd=%d\n",rd);
						   
				char * str=malloc(taille_str);
				rd=read(fd_argv,str,sizeof(char)*taille_str);//READ char *
				printf("J'ai lu %s de taille %d!\n",str,taille_str);
				
				buf_cmd = realloc(buf_cmd, taille_buff+ sizeof(uint32_t)+ taille_str);
				memcpy(buf_cmd+taille_buff, &taille_str, sizeof(uint32_t));
				taille_buff+=sizeof(uint32_t);
				memcpy(buf_cmd+taille_buff, &str, taille_str);
				taille_buff+= taille_str;
			  }
			nb_tasks++;	
			buf_inter = realloc(buf_inter, sizeof(uint64_t)+sizeof(uint64_t)+ sizeof(uint32_t)+sizeof(uint8_t)+taille_buff);	
			memcpy(buf_inter+sizeof(uint64_t)+sizeof(uint64_t)+ sizeof(uint32_t)+sizeof(uint8_t), buf_cmd,taille_buff);		
			
			while(taille_globale<taille_utilisee+ sizeof(uint64_t)+sizeof(uint64_t)+ sizeof(uint32_t)+sizeof(uint8_t)+taille_buff){
							buf_all_tasks = realloc(buf_all_tasks, taille_globale+1024);
							taille_globale+=1024;
			}		
			memcpy(buf_all_tasks + taille_utilisee, buf_inter, sizeof(uint64_t)+sizeof(uint64_t)+ sizeof(uint32_t)+sizeof(uint8_t)+taille_buff);
			taille_utilisee+=sizeof(uint64_t)+sizeof(uint64_t)+ sizeof(uint32_t)+sizeof(uint8_t)+taille_buff;
		}
		
		char *buf_total = malloc(sizeof(uint16_t)+ sizeof(uint32_t) + taille_utilisee);
		memcpy(buf_total,&ok,sizeof(uint16_t));
		memcpy(buf_total+sizeof(uint16_t),&nb_tasks,sizeof(uint32_t));
		memcpy(buf_total+sizeof(uint16_t)+sizeof(uint32_t),buf_all_tasks,taille_utilisee);
		
		write(fd_reply, buf_total,sizeof(uint16_t)+ sizeof(uint32_t) + taille_utilisee);
        break;
        
      }
		}
		
    }
    printf("RD : %d\n",rd);
	
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
        sleep(60);
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
              write(fd_exit,time2,sizeof(int64_t));//WRITE
              if(WIFEXITED(status)){
                int wstatus = WEXITSTATUS(status);//exit code                
                write(fd_exit,(uint16_t)&wstatus,sizeof(int16_t));//WRITE
               
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
