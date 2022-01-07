#include "utils.h"
#include "saturnd_child.c"

int cpy(char * buf1,char *buf2,size_t size){
  strncpy(buf2,buf1,size);
  buf2[size]='\0';
  return 1;
}

int get_std(int fd_request ,int fd_reply, int type, char *pathtasks){
	uint64_t id;
	char * buf;
	read(fd_request,&id, sizeof(uint64_t));//READ
	id = htobe64(id);
	char * pathtasks_file=malloc(PATH_SIZE);//MALLOC
  strcpy(pathtasks_file,pathtasks);
	snprintf(pathtasks_file + strlen(pathtasks), 100-strlen(pathtasks),"%"PRIu64,id );
	
	DIR * dir = opendir(pathtasks_file);
	if(dir==NULL){
	  buf=malloc(2*sizeof(uint16_t));//MALLOC
    uint16_t uerror= htobe16(SERVER_REPLY_ERROR);
    uint16_t uerror_type=htobe16(SERVER_REPLY_ERROR_NOT_FOUND);
    memcpy(buf,&uerror,sizeof(uint16_t));
    memcpy(buf+sizeof(uint16_t),&uerror_type,sizeof(uint16_t));
    write(fd_reply,buf,2*sizeof(uint16_t));//WRITE
    free(buf);
	}
  else
  {
		char * pathtasks_std=malloc(100);//MALLOC
		strcpy(pathtasks_std,pathtasks_file);
		strcat(pathtasks_std,"/");
		if(type){
			strcat(pathtasks_std,CMD_STDOUT);
		} else {
			strcat(pathtasks_std,CMD_STDERR);
		}
		int fd_std = open(pathtasks_std, O_RDONLY);
		if(fd_std== -1){
			buf=malloc(2*sizeof(uint16_t));//MALLOC
			uint16_t uerror= htobe16(SERVER_REPLY_ERROR);
			uint16_t uerror_type= htobe16(SERVER_REPLY_ERROR_NEVER_RUN);
			memcpy(buf,&uerror,sizeof(uint16_t));
			memcpy(buf+sizeof(uint16_t),&uerror_type,sizeof(uint16_t));
			write(fd_reply,buf,2*sizeof(uint16_t));
			free(buf);
		} else {
			int size_buf = 1024;
			
			char *buffer = malloc(size_buf);
			int taille_buff = size_buf;
			uint32_t taille = 0;
			int rd;
			
			while((rd = read(fd_std, buffer+taille, size_buf))> 0){
				taille = taille+rd;
				if(rd == size_buf){
				  buffer = realloc(buffer, taille_buff + size_buf);
				  taille_buff = taille_buff + size_buf;
				}
			}
			//if(rd==-1)go to
			uint16_t ok = htobe16(SERVER_REPLY_OK);
			uint32_t hto_taille = htobe32(taille);
						
			char *buff_total = malloc(sizeof(uint16_t)+ sizeof(uint32_t)+taille);
			memcpy(buff_total,&ok, sizeof(uint16_t));
			memcpy(buff_total+sizeof(uint16_t), &hto_taille , sizeof(uint32_t));
			strncpy(buff_total+sizeof(uint16_t)+sizeof(uint32_t), buffer, taille);
			write(fd_reply, buff_total,sizeof(uint16_t)+ sizeof(uint32_t)+taille);
		}
	}
}
void handler(int s){
  switch(s) {
    case SIGALRM:
      printf("----------------ALARM------------------\n");
    break;
    case SIGTERM:
      printf("------------------SIGTERM----------------------\n");
      while(waitpid(-1,NULL,WNOHANG)!=-1);
      exit(EXIT_SUCCESS);
    break;
  }
}
int main(){
  char * pipes_directory;
  char * username_str = getlogin();
  
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
    //printf("Mkdir saturn = %s\n",pathtasks_saturnd);
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
  
  char * buftaskmax = malloc(50);//MALLOC
  strcpy(buftaskmax,pathtasks_saturnd);
  strcat(buftaskmax,"/");
  strcat(buftaskmax,"taskid_max");

  pid_t pid = fork();
  //if(pid<0)goto;
  		

  if(pid>0){//GRAND PERE
    //sleep(30);
    //printf("AVANT\n");
    int fd_request=open(fifo_request,O_RDONLY);
    int fd_reply=open(fifo_reply,O_WRONLY);
    int fd_taskmax=open(buftaskmax,O_RDONLY);
    
    int out=0;
    uint16_t u16;
    uint32_t u32;
    uint64_t taskid;
    uint64_t taskmax_name;
    
    if(fd_taskmax!=-1)read(fd_taskmax,&taskmax_name,sizeof(uint64_t));//Cas ou taskid_max est déjà existant, on initialise alors taskmax_name
    
    char * buf;
    int fd_buftask;
    struct dirent *  struct_dir;
    //printf("OUT\n");
    int rd;
    while(out==0){
      rd = read(fd_request,&u16,sizeof(uint16_t));

      if(rd>0){	
        u16=htobe16(u16);

        switch (u16){
          case CLIENT_REQUEST_REMOVE_TASK:
        
          read(fd_request,&taskid,sizeof(uint64_t));//READ du TASKID
          taskid=htobe64(taskid);
        //
        char * pathtasks_id = malloc(100);//MALLOC
        strcpy(pathtasks_id,pathtasks);
        snprintf(pathtasks_id + strlen(pathtasks), 100-strlen(pathtasks),"%"PRIu64"/",taskid );

        DIR * dir=opendir(pathtasks_id);
        if(dir==NULL){
        //printf("RM Dir NULL\n");
          buf=malloc(2*sizeof(uint16_t));
          uint16_t uerror= htobe16(SERVER_REPLY_ERROR);
          uint16_t uerror_type= htobe16(SERVER_REPLY_ERROR_NOT_FOUND);
          memcpy(buf,&uerror,sizeof(uint16_t));
          memcpy(buf+sizeof(uint16_t),&uerror_type,sizeof(uint16_t));
          write(fd_reply,buf,2*sizeof(uint16_t));
          free(buf);
        } else {
        buf =malloc(256);
        strcpy(buf,pathtasks_id);//ou pathtask_id ?// <- ui
        strcat(buf,CMD_ARGV);//argv
        remove(buf);//Remove
        
        strcpy(buf,pathtasks_id);
        strcat(buf,CMD_TEXEC);//time exit
        remove(buf);//Remove
        
        strcpy(buf,pathtasks_id);
        strcat(buf,CMD_EXIT);//exit code
    	remove(buf);//Remove
    	
    	strcpy(buf,pathtasks_id);
        strcat(buf,CMD_STDOUT);//stdout
    	remove(buf);//Remove
    	
    	strcpy(buf,pathtasks_id);
        strcat(buf,CMD_STDERR);//stderr
    	remove(buf);//Remove
    	
    	rmdir(pathtasks_id);
    	
    	uint16_t ok = htobe16(SERVER_REPLY_OK);
    	write(fd_reply,&ok,sizeof(uint16_t));
    	}
        break;
        case CLIENT_REQUEST_CREATE_TASK:
          fd_buftask=open(buftaskmax,O_RDWR | O_TRUNC);//exclusifs je crois O_RDWR
          
          if(fd_buftask==-1){
            taskmax_name = (uint64_t) 1;
            fd_buftask = open(buftaskmax, O_CREAT | O_WRONLY,0777);
            chmod(buftaskmax, 0777);

            write(fd_buftask,&taskmax_name,sizeof(uint64_t));
            close(fd_buftask);
          } else {
            read(fd_buftask,&taskmax_name,sizeof(uint64_t));

            taskmax_name=taskmax_name+(uint64_t)1;
            write(fd_buftask,&taskmax_name,sizeof(uint64_t));
            close(fd_buftask);
          }
          //cr  
          //printf("Value = %lu\n",taskmax_name);

          char * pathtasks_file=malloc(100);//MALLOC
          strcpy(pathtasks_file,pathtasks);
          snprintf(pathtasks_file + strlen(pathtasks), 100-strlen(pathtasks),"%"PRIu64,taskmax_name );
        //  printf("Path = %s\n",pathtasks_file);
          char * pathtasks_timexec=malloc(100);//MALLOC
          strcpy(pathtasks_timexec,pathtasks_file);
          strcat(pathtasks_timexec,"/");
          strcat(pathtasks_timexec,CMD_TEXEC);
            
          char * pathtasks_argv=malloc(100);//MALLOC
          strcpy(pathtasks_argv,pathtasks_file);
          strcat(pathtasks_argv,"/");
          strcat(pathtasks_argv,CMD_ARGV);
            
          char * pathtasks_exit=malloc(100);//MALLOC
          strcpy(pathtasks_exit,pathtasks_file);
          strcat(pathtasks_exit,"/");
          strcat(pathtasks_exit,CMD_EXIT);
          
          mkdir(pathtasks_file,0751);//file
          
          int fd_timexec=open(pathtasks_timexec,O_CREAT | O_WRONLY,0751);//file
          int fd_argv=open(pathtasks_argv,O_CREAT | O_WRONLY, 0751);
          int fd_exit=open(pathtasks_exit,O_CREAT | O_WRONLY, 0751);
          
          char * temps=malloc(sizeof(uint64_t)+sizeof(uint32_t)+sizeof(uint8_t));
          
          read(fd_request,temps,sizeof(uint64_t)+sizeof(uint32_t)+sizeof(uint8_t));//READ
              

          write(fd_timexec,temps,sizeof(uint64_t)+sizeof(uint32_t)+sizeof(uint8_t));//WRITE
            
          //COMMAND
         
          read(fd_request,&u32,sizeof(uint32_t));//READ argc

          u32 = htobe32(u32);
         // printf("Argc = %"PRIu32" ]\n",u32);
          write(fd_argv,&u32,sizeof(uint32_t));

          for(int j=0;j<u32;j++){
	    uint32_t taille;//taille de argv[pos]
	    read(fd_request,&taille,sizeof(uint32_t));//READ argv[i] size
	    taille = htobe32(taille);//htobe32
	    
	    char * str_argv=malloc(sizeof(char)*taille);
	    read(fd_request,str_argv,sizeof(char)*taille);//READ string
	   // printf("Argv[%d] = %.*s , strl = %"PRIu32" \n",j,taille,str_argv,taille);

	    write(fd_argv,&taille,sizeof(uint32_t));
	    write(fd_argv,str_argv,sizeof(char)*taille);
          }
          
          char *buf=malloc(sizeof(uint16_t)+sizeof(uint64_t));
          uint16_t ok_reply= SERVER_REPLY_OK;
          
          taskmax_name = htobe64(taskmax_name);
	  		
          memcpy(buf,&ok_reply,sizeof(uint16_t)); memcpy(buf+sizeof(uint16_t),&taskmax_name,sizeof(uint64_t));
          write(fd_reply,buf,sizeof(uint16_t)+sizeof(uint64_t));
          free(buf);
          taskmax_name = be64toh(taskmax_name);
        break;
        case CLIENT_REQUEST_TERMINATE:
          kill(pid,SIGTERM);
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
		printf("HTO = %"PRIu64"\n",taskid);
		
		pathtasks_file=malloc(100);//MALLOC
		strcpy(pathtasks_file,pathtasks);
               snprintf(pathtasks_file + strlen(pathtasks), 100-strlen(pathtasks),"%"PRIu64,taskid );
		 
		dir = opendir(pathtasks_file);
		printf("Id = %"PRIu64" , PathTasks_file = %s\n",taskid,pathtasks_file);
		if(dir==NULL){
		printf("T - Dir NULL\n");
			buf=malloc(2*sizeof(uint16_t));
			uint16_t uerror= htobe16(SERVER_REPLY_ERROR);
			uint16_t uerror_type= htobe16(SERVER_REPLY_ERROR_NOT_FOUND);
			memcpy(buf,&uerror,sizeof(uint16_t));
			memcpy(buf+sizeof(uint16_t),&uerror_type,sizeof(uint16_t));
			write(fd_reply,buf,2*sizeof(uint16_t));
			free(buf);
		}else{
			printf("T - Else Dir\n");
			char * pathtasks_std=malloc(100);//MALLOC
			strcpy(pathtasks_std,pathtasks_file);
			strcat(pathtasks_std, "/");
			strcat(pathtasks_std, CMD_EXIT);
			
			int fd_std = open(pathtasks_std, O_RDONLY);
			
			int taille_buff = sizeof(int64_t)+sizeof(uint16_t);
			char *buffer = malloc(taille_buff);
			int rd;
			int size_extime = sizeof(int64_t)+sizeof(uint16_t);
				
			uint32_t taille = 0;
				
				while(rd = read(fd_std, buffer+taille*size_extime, size_extime) >0){
					//if(rd==-1)go to
					int64_t hto_timing;
					memcpy(&hto_timing,buffer+taille*size_extime,sizeof(int64_t));
					hto_timing=htobe64(hto_timing);
					memcpy(buffer+taille*size_extime,&hto_timing,sizeof(int64_t));
					
					taille ++;
					buffer = realloc(buffer, taille_buff + size_extime);
					taille_buff = taille_buff + size_extime;
				}
				
				uint16_t ok = htobe16(SERVER_REPLY_OK);
				uint32_t hto_taille = htobe32(taille);
				
				char * buff_total = malloc(sizeof(uint16_t)+ sizeof(uint32_t)+taille*size_extime);
				
				memcpy(buff_total,&ok, sizeof(uint16_t));
				memcpy(buff_total+sizeof(uint16_t), &hto_taille , sizeof(uint32_t));
				memcpy(buff_total+sizeof(uint16_t)+sizeof(uint32_t), buffer, taille*size_extime);

				write(fd_reply, buff_total,sizeof(uint16_t)+ sizeof(uint32_t)+taille*size_extime);
				printf("EX TI WRITE\n");
		}        
        break;
        
        case CLIENT_REQUEST_LIST_TASKS:
        dir = opendir(pathtasks);
        char *buf_all_tasks =malloc(1024);
        int taille_globale = 1024;
        int taille_utilisee = 0;
        uint32_t nb_tasks = 0;
        uint16_t ok = htobe16(SERVER_REPLY_OK);

        while((struct_dir=readdir(dir))){
        if(strcmp(struct_dir->d_name,".")!=0 && strcmp(struct_dir->d_name,"..")!=0){

			char *buf_inter = malloc(sizeof(uint64_t)+sizeof(uint64_t)+ sizeof(uint32_t)+sizeof(uint8_t));
			taskid = strtoull(struct_dir->d_name, NULL, 0);

			taskid=htobe64(taskid);
			
			memcpy(buf_inter, &taskid, sizeof(uint64_t));
			
			char * pathtasks_exec=malloc(100);//MALLOC
			strcpy(pathtasks_exec,pathtasks);
			strcat(pathtasks_exec,struct_dir->d_name);
			strcat(pathtasks_exec,"/");
			strcat(pathtasks_exec,CMD_TEXEC);

			int fd_time_exec = open(pathtasks_exec , O_RDONLY);

			read(fd_time_exec,buf_inter+sizeof(uint64_t) , sizeof(uint64_t)+ sizeof(uint32_t)+sizeof(uint8_t));
			    perror("read8");

		    char * pathtasks_argv=malloc(100);//MALLOC
		        strcpy(pathtasks_argv,pathtasks);
			strcat(pathtasks_argv,struct_dir->d_name);
			strcat(pathtasks_argv,"/");
			strcat(pathtasks_argv,CMD_ARGV);

			int fd_argv = open(pathtasks_argv, O_RDONLY);

			uint32_t argc;
			read(fd_argv, &argc, sizeof(uint32_t));
			    perror("read9");

			
			char *buf_cmd  = malloc(sizeof(uint32_t));
			size_t taille_buff = sizeof(uint32_t);
			
			int rd;
			uint32_t hto_argc = htobe32(argc);
			memcpy(buf_cmd, &hto_argc, sizeof(uint32_t));
			
			
			for(int i=0;i<argc;i++){
				uint32_t taille_str;
				rd=read(fd_argv,&taille_str,sizeof(uint32_t));//READ L
				uint32_t hto_taille_str=htobe32(taille_str);

						   
				char * str=malloc(taille_str);
				rd=read(fd_argv,str,sizeof(char)*taille_str);//READ char *
				
				buf_cmd = realloc(buf_cmd, taille_buff+sizeof(uint32_t)+taille_str);
				
				memcpy(buf_cmd+taille_buff, &hto_taille_str, sizeof(uint32_t));
				taille_buff+=sizeof(uint32_t);
				//strncpy(buf_cmd+taille_buff,&s
				memcpy(buf_cmd+taille_buff, str, taille_str);				
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
		}
		nb_tasks=htobe32(nb_tasks);
		
		char *buf_total = malloc(sizeof(uint16_t)+ sizeof(uint32_t) + taille_utilisee);
		memcpy(buf_total,&ok,sizeof(uint16_t));
		memcpy(buf_total+sizeof(uint16_t),&nb_tasks,sizeof(uint32_t));
		memcpy(buf_total+sizeof(uint16_t)+sizeof(uint32_t),buf_all_tasks,taille_utilisee);
		write(fd_reply, buf_total,sizeof(uint16_t)+ sizeof(uint32_t) + taille_utilisee);
        break;
      }
		}
		
    }
	
    printf("A écrit\n");
    wait(NULL);//ce wait pour le terminate
    uint16_t ok = SERVER_REPLY_OK;
    write(fd_reply,&ok,sizeof(uint16_t));
    close(fd_reply);
    close(fd_request);
  }//FIN GRAND PERE
  else
  {//DEBUT "PERE"
    
    
    time_t time2 = time(NULL);
    uint16_t code;
    struct sigaction s;
    s.sa_handler = handler;
    
    while(1){
      printf("ErrNo = %s\n",strerror(errno));
      pid_t pid2=fork();
      if(pid2>0){//PERE
      sigaction(SIGALRM,&s, NULL);
      sigaction(SIGTERM,&s, NULL);
      alarm(60);
      pause();
      waitpid(pid2,NULL,WNOHANG);
      time2=time(NULL);
      }//FIN PERE
      else
      {//PETIT FILS
        execute(pid2,time2);
      }//FIN PETIT FILS
    }
    while (wait(NULL) > 0);
    
    exit(EXIT_SUCCESS);
  }
}
