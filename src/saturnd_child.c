int execute(pid_t pid2, time_t time2){
  char * pathtasks = "./saturnd_dir/tasks/";
  struct dirent * struct_dir;

  struct tm *ptime = localtime(&time2); //POINTEUR
  DIR * dir=opendir(pathtasks); //POINTEUR
  while((struct_dir=readdir(dir))){//WHILE TASKS
    if (strcmp(struct_dir->d_name,".")!=0 && strcmp(struct_dir->d_name,"..")!=0){
                  
      char *path_to_task = malloc(PATH_SIZE);//MALLOC
      strcpy(path_to_task,pathtasks);
      strcat(path_to_task,struct_dir->d_name);
          
      DIR * taskdir = opendir(path_to_task); //POINTEUR
      char * pathtime = malloc(PATH_SIZE);//MALLOC
      strcpy(pathtime,path_to_task);
      strcat(pathtime,"/");
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
            
      timing_string_from_field(buf_minutes, 0, 59, minutes);//Minutes
      timing_string_from_field(buf_heures, 0, 23, heures);// Hours
      timing_string_from_field(buf_jours, 0, 6, jours);// Jours
            
      if(compaire_cron(ptime->tm_min,buf_minutes) && compaire_cron(ptime->tm_hour,buf_heures) && compaire_cron(ptime->tm_wday,buf_jours)){
        char * to_cmd = malloc(PATH_SIZE);//MALLOC
        strcpy(to_cmd,path_to_task);     
        strcat(to_cmd,"/");
        strcat(to_cmd,CMD_ARGV);

        pid_t pid=fork();
        if(pid==0){//FILS
          uint32_t cmd_argc;
          int fd_cmd = open(to_cmd, O_RDONLY);
          int rd=read(fd_cmd,&cmd_argc,sizeof(uint32_t));//READ

          int tab_size = sizeof(char *) * cmd_argc+1;
          char ** tab = malloc(tab_size);//MALLOC
          assert(tab!=NULL);
          
          for(int i=0;i<cmd_argc;i++){               
            uint32_t taille_str;
            rd=read(fd_cmd,&taille_str,sizeof(uint32_t));//READ L
      
            char * str=malloc(taille_str);//MALLOC
            rd=read(fd_cmd,str,taille_str);//READ char *

            char * str_final=malloc(taille_str+1);//MALLOC
            cpy(str,str_final,taille_str);

            tab[i]=malloc(sizeof(char*)*taille_str+1);//MALLOC
            strncpy(tab[i],str_final,taille_str+1);
          }
          tab[tab_size-1]=NULL;
    
          char *path_to_out = malloc(PATH_SIZE);//MALLOC
          strcpy(path_to_out,path_to_task);
          strcat(path_to_out,"/");
          strcat(path_to_out,CMD_STDOUT);
          
          char *path_to_err = malloc(PATH_SIZE);//MALLOC
          strcpy(path_to_err,path_to_task);
          strcat(path_to_err,"/");
          strcat(path_to_err,CMD_STDERR);
                  
          int fd_out=open(path_to_out,O_CREAT|O_WRONLY | O_TRUNC, 0666);//OPEN
          int fd_err=open(path_to_err,O_CREAT|O_WRONLY | O_TRUNC,0666);//OPEN

          dup2(fd_out,STDOUT_FILENO);//DUP
          dup2(fd_err,STDERR_FILENO);//DUP
          
          execvp(tab[0],tab);//EXEC 
          exit(0);
        }
        else
        {//GESTION fichier TIME-EXIT_CODE
          char * cmd_exit = malloc(PATH_SIZE);//MALLOC
          strcpy(cmd_exit,path_to_task);
          strcat(cmd_exit,"/");
          strcat(cmd_exit,CMD_EXIT);
                
          int fd_exit=open(cmd_exit,O_CREAT | O_WRONLY | O_APPEND,0751);//OPEN
                
          int status;
          waitpid(pid2,&status,0);//WAITPID
                
          int w=write(fd_exit,(int64_t)&time2,sizeof(int64_t));//WRITE
                
          if(WIFEXITED(status)){
            int wstatus = be16toh(WEXITSTATUS(status));//exit code        
            w=write(fd_exit,&wstatus,sizeof(uint16_t));//WRITE
          }
          else
          {
            uint16_t ex=NO_EXIT_CODE;
            write(fd_exit,&ex,sizeof(uint16_t));//WRITE
          }
        }
      }
	  }        
  }//FIN WHILE TASKS
  closedir(dir);
  exit(1);
}