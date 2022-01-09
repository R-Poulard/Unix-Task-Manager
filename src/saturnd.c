#include "utils.c"
#include "cmd_create.c"
#include "cmd_list.c"
#include "cmd_time_exit_code.c"
#include "saturnd_child.c"

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
		char * pathtasks_std;
		if(type){
			pathtasks_std = path_constr(pathtasks_std,pathtasks_file,CMD_STDOUT);
		} else {
			pathtasks_std = path_constr(pathtasks_std,pathtasks_file,CMD_STDERR);
		}

		int fd_std = open(pathtasks_std, O_RDONLY);//OPEN
		if(fd_std== -1){
			buf=malloc(2*sizeof(uint16_t));//MALLOC
			uint16_t uerror= htobe16(SERVER_REPLY_ERROR);
			uint16_t uerror_type= htobe16(SERVER_REPLY_ERROR_NEVER_RUN);
			memcpy(buf,&uerror,sizeof(uint16_t));
			memcpy(buf+sizeof(uint16_t),&uerror_type,sizeof(uint16_t));
			write(fd_reply,buf,2*sizeof(uint16_t));//WRITE
			free(buf);
		} else {
			int size_buf = 1024; 
			
			char *buffer = malloc(size_buf);//MALLOC
			int taille_buff = size_buf;
			uint32_t taille = 0;
			int rd;
			
			while((rd = read(fd_std, buffer+taille, size_buf))> 0){
				taille = taille+rd;
				if(rd == size_buf){
				  buffer = realloc(buffer, taille_buff + size_buf);
				  taille_buff = taille_buff + size_buf;
				}
			}//READ
			//if(rd==-1)go to
			uint16_t ok = htobe16(SERVER_REPLY_OK);
			uint32_t hto_taille = htobe32(taille);
						
			char *buff_total = malloc(sizeof(uint16_t)+ sizeof(uint32_t)+taille);//MALLOC
			memcpy(buff_total,&ok, sizeof(uint16_t));
			memcpy(buff_total+sizeof(uint16_t), &hto_taille , sizeof(uint32_t));
			strncpy(buff_total+sizeof(uint16_t)+sizeof(uint32_t), buffer, taille);
			write(fd_reply, buff_total,sizeof(uint16_t)+ sizeof(uint32_t)+taille);//WRITE

      free(buffer);
		  close(fd_std);
    }
    free(pathtasks_std);
	}
  free(pathtasks_file);
} 
void handler(int s){
  switch(s) {
    case SIGALRM:
    break;
    case SIGTERM:
      while(waitpid(-1,NULL,WNOHANG)>0);
      exit(EXIT_SUCCESS);
    break;
  }
}
int main(){
  char * pipes_directory;
  char * username_str = getlogin();
  
  pipes_directory = malloc(PATH_SIZE);//creation of the default PIPES_DIR//MALLOC
  strcpy(pipes_directory, "/tmp/");
  strcat(pipes_directory, username_str);
  mkdir(pipes_directory,0751);//MKDIR
  
  strcat(pipes_directory, "/saturnd");
  mkdir(pipes_directory,0751);//MKDIR
  
  strcat(pipes_directory, "/pipes");
  mkdir(pipes_directory,0751);//MKDIR
  
  char *fifo_request=malloc(PATH_SIZE);//MALLOC
  strcpy(fifo_request,pipes_directory);
  strcat(fifo_request,"/saturnd-request-pipe");
  
  char *fifo_reply = malloc(PATH_SIZE);//MALLOC
  strcpy(fifo_reply,pipes_directory);
  strcat(fifo_reply,"/saturnd-reply-pipe");  
  
  //PATHTASKS
  char * pathtasks_saturnd = malloc(PATH_SIZE);//MALLOC
  strcpy(pathtasks_saturnd,"./saturnd_dir");
  mkdir(pathtasks_saturnd, 0751);//MKDIR
     
  char * pathtasks = malloc(PATH_SIZE);//MALLOC
  strcpy(pathtasks,"./saturnd_dir/tasks/");
    
  mkdir(pathtasks,0751);//MKDIR
  struct stat stat_request;
  struct stat stat_reply;
  
  if(stat(fifo_request,&stat_request)==-1 || S_ISFIFO(stat_request.st_mode)==0){
    mkfifo(fifo_request,0751);//MKFIFO
  }
  if(stat(fifo_reply,&stat_reply)==-1 || S_ISFIFO(stat_reply.st_mode)==0){
    mkfifo(fifo_reply,0751);//MKFIFO
  }
  
  char * buftaskmax;
  
  free(pipes_directory);

  pid_t pid = fork();//FORK
  //if(pid<0)goto;
  
  if(pid>0){//GRAND PERE
    int fd_request=open(fifo_request,O_RDONLY);//OPEN
    int fd_reply=open(fifo_reply,O_WRONLY);//OPEN

    free(fifo_request);
    free(fifo_reply);

    int out=0;
    uint16_t u16;
    uint64_t taskid;

    char * buf;
    int fd_buftask;
    
    int rd; 
    while(out==0){
      rd = read(fd_request,&u16,sizeof(uint16_t));
      if(rd>0){  
        u16=htobe16(u16);
 
        switch (u16){
          case CLIENT_REQUEST_REMOVE_TASK:
            read(fd_request,&taskid,sizeof(uint64_t));//READ du TASKID
            taskid=htobe64(taskid);
            
            char * pathtasks_id = malloc(PATH_SIZE);//MALLOC
            strcpy(pathtasks_id,pathtasks);
            snprintf(pathtasks_id + strlen(pathtasks), PATH_SIZE-strlen(pathtasks),"%"PRIu64"/",taskid);

            DIR * dir=opendir(pathtasks_id);//POINTEUR
            if(dir==NULL){ 
              buf=malloc(2*sizeof(uint16_t));
              uint16_t uerror= htobe16(SERVER_REPLY_ERROR);
              uint16_t uerror_type= htobe16(SERVER_REPLY_ERROR_NOT_FOUND);
              memcpy(buf,&uerror,sizeof(uint16_t));
              memcpy(buf+sizeof(uint16_t),&uerror_type,sizeof(uint16_t));
              write(fd_reply,buf,2*sizeof(uint16_t));
              free(buf);
            } else {
              buf = path_constr (buf, pathtasks_id,CMD_ARGV);//argv
              remove(buf);//Remove
              
              buf = path_constr (buf, pathtasks_id,CMD_TEXEC);//time exit
              remove(buf);//Remove
              
              buf = path_constr (buf, pathtasks_id,CMD_EXIT);//exit code
              remove(buf);//Remove
            
              buf = path_constr (buf, pathtasks_id,CMD_STDOUT);//stdout
              remove(buf);//Remove
            
              buf = path_constr (buf, pathtasks_id,CMD_STDERR);//stdout
              remove(buf);//Remove
            
              rmdir(pathtasks_id);
            
              uint16_t ok = htobe16(SERVER_REPLY_OK);
              write(fd_reply,&ok,sizeof(uint16_t));
            }
          break;
          case CLIENT_REQUEST_CREATE_TASK:
            buftaskmax = path_constr(buftaskmax,pathtasks_saturnd,"taskid_max");//MALLOC
            cmd_create (fd_request,fd_reply,buftaskmax,pathtasks);
             
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
            cmd_time_exit_code(fd_request,fd_reply,taskid,pathtasks); 
          break;
          case CLIENT_REQUEST_LIST_TASKS:
            cmd_list(fd_reply,pathtasks);
          break;
        }
		  }
    }
	
    wait(NULL);//ce wait pour le terminate
    uint16_t ok = SERVER_REPLY_OK;
    write(fd_reply,&ok,sizeof(uint16_t));
    close(fd_request);
    close(fd_reply);
    
  }//FIN GRAND PERE
  else
  {//DEBUT "PERE"

    time_t time2 = time(NULL);
    uint16_t code;
    struct sigaction s;
    s.sa_handler = handler;
    
    while(1){ 
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
        executioner_pulse(pid2,time2);
      }//FIN PETIT FILS
    }
    while (wait(NULL) > 0);
    free(pathtasks_saturnd);
    exit(EXIT_SUCCESS);
  }
}
