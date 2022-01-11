int execute_a_task(char *path_to_task,DIR* dir){ //Execute la task
    //ON RECUPERE LA COMMANDE ET SES ARGUMENTS
    char * to_cmd = path_constr(to_cmd,path_to_task,CMD_ARGV);
    uint32_t cmd_argc;

    int fd_cmd = open(to_cmd, O_RDONLY);//On ouvre le file contenant la commandline écrit pas le pere lors de -c
    if(fd_cmd==-1){
      goto err_cmd;
    }
    int rd=read(fd_cmd,&cmd_argc,sizeof(uint32_t));//On va read le nombre d'argument que possède la structur COMMANDELINE
    if(rd==-1){
      goto err_cmd;
    }
    int tab_size = sizeof(char *) * (cmd_argc+1);//On initialise le double tableau qui sera passer en argument du exec()
    char ** tab = malloc(tab_size);
    assert(tab!=NULL);
    for(int i=0;i<cmd_argc;i++){//Pour chaque arguments qu'il y a
        uint32_t taille_str;
        rd=read(fd_cmd,&taille_str,sizeof(uint32_t));//On read sa taille

        char * str=malloc(taille_str);//On va alors recupere la chaine de caractere correspondant au nombre d'argument
        assert(str!=NULL);
        rd=read(fd_cmd,str,taille_str);
        //On ajoute le '\0'
        char * str_final=malloc(taille_str+1);//On copie notre chaine dans une taille alloué de +1
        assert(str_final!=NULL);
        cpy(str,str_final,taille_str);
        
        tab[i]=malloc(sizeof(char*)*taille_str+1);//Puis on rajoute à la derniere position de la chaine le '\0'
        assert(tab[i]!=NULL);

        strncpy(tab[i],str_final,taille_str+1);//On copie notre chaine au bon endroit de notre double tableau
        free(str);
        free(str_final);
    }
    tab[cmd_argc]=NULL;//On rajoute NULL à la derniere case comme requis par exec
    
    //On va construire les chemins vers nos fichier stockant les valeurs de STDOUT et STDERR
    char * path_to_out = path_constr(path_to_out,path_to_task,CMD_STDOUT);
    char * path_to_err = path_constr(path_to_err,path_to_task,CMD_STDERR);
    assert(path_to_out!=NULL);
    assert(path_to_err!=NULL);

    //On les ouvres et on réalise les dup2 correspondant pour que l'exec écrive lui même dans les fichier
    //On ajoute O_TRUNC car on veut recuperer la valeurs de la derniere execution à chaque fois et
    // le O_CREATE car ils sont créer lors de la 1er execution ce qui nous permet de voir facilement si la tache a été executé ou non par les peres lors de -e ou -o
    int fd_out=open(path_to_out,O_CREAT | O_WRONLY | O_TRUNC, 0666);//OPEN
    int fd_err=open(path_to_err,O_CREAT | O_WRONLY | O_TRUNC,0666);//OPEN
    if(fd_out==-1 || fd_err == -1)goto err_out;
    //On realise les dup2
    int d1=dup2(fd_out,STDOUT_FILENO);
    int d2=dup2(fd_err,STDERR_FILENO);
    if(d1==-1 || d2==-1)goto err_out;

    //On ferme et free tous avant le exec car il ne le fera pas pour nous
    close(fd_cmd);
    close(fd_out);
    close(fd_err);
    
    free(path_to_err);
    free(path_to_out);
    free(path_to_task);
    if(dir!=NULL)closedir(dir);
    
    execvp(tab[0],tab);//On execute avec tab[0] et tab conformement au man exec()
    exit(EXIT_FAILURE);//On atteint se cas si l'exec n'a pas pu être fait ce qui n'est pas normal

    err_out:
      close(fd_out);
      close(fd_err);
      free(path_to_err);
      free(path_to_out);
      free(path_to_task);
      if(dir!=NULL)closedir(dir);
    err_cmd:
      free(to_cmd);
      close(fd_cmd);
      exit(EXIT_FAILURE);
}

int write_exit_code(char * path_to_task,pid_t pid2,time_t time2){//Ecrit ptime et l'exit code du pid2 dans le file de la task de path_to_task
    char * pathtasks_argv = path_constr(pathtasks_argv,path_to_task,CMD_ARGV);
		assert(pathtasks_argv!=NULL);
    int fd_argv = open(pathtasks_argv, O_RDONLY);
		if(fd_argv==-1){
      free(pathtasks_argv);
      exit(EXIT_FAILURE);
		} else {
      free(pathtasks_argv);
			close(fd_argv);
		}

    char * cmd_exit= path_constr(cmd_exit,path_to_task,CMD_EXIT);//On creer le chemin vers le fils CMD_EXIT
    assert(cmd_exit!=NULL);

    int fd_exit=open(cmd_exit,O_CREAT | O_WRONLY | O_APPEND,0751);//on l'open
    if(fd_exit==-1){
      goto failure2;
    }
    int status;
    waitpid(pid2,&status,0);//On va attendre que le fils termine son exec() en returnant le statut
    int w=write(fd_exit,(int64_t)&time2,sizeof(int64_t));//On ecrit le timing
    if(w==-1){
      goto failure3;
    }
    //On écrit le statut
    if(WIFEXITED(status)){//Si on a fini avec une valeur de exit code
        int wstatus = be16toh(WEXITSTATUS(status));//On la recupere
        w=write(fd_exit,&wstatus,sizeof(uint16_t));//Puis on la write dans le format de protocole.md
        if(w==-1)goto failure3;
    }
    else{
        uint16_t ex=NO_EXIT_CODE;//Sinon on ecrit le format NO_EXIT_CODE
        if(write(fd_exit,&ex,sizeof(uint16_t))==-1)goto failure3;
    }
    //FREE
    free(cmd_exit);
    close(fd_exit);
    exit(EXIT_SUCCESS);//Sans importance

    failure3:
      close(fd_exit);
    failure2:
      free(cmd_exit);
      exit(EXIT_FAILURE);
}

int executioner_pulse(pid_t pid2, time_t time2){

  char * pathtasks = "./saturnd_dir/tasks/";
  struct dirent * struct_dir;

  struct tm *ptime = localtime(&time2); //On transforme notre local time en structure pour separer les minutes,heures et jours
  DIR * dir=opendir(pathtasks); //On ouvre le fils

  while((struct_dir=readdir(dir))){//On parcours tout les tasks de notre executeur
    if (strcmp(struct_dir->d_name,".")!=0 && strcmp(struct_dir->d_name,"..")!=0){//Verifie qu'on ne lit pas dans un . ou .. qui ne sont pas des tasks

      // On creer le chemin absolu de l'executable jusqu'au repertoir de la tache lue
      char *path_to_task = malloc(PATH_SIZE);//MALLOC
      assert(path_to_task!=NULL);
      strcpy(path_to_task,pathtasks);
      strcat(path_to_task,struct_dir->d_name);

      //
      char * pathtasks_argv = path_constr(pathtasks_argv,path_to_task,CMD_ARGV);
      int fd_argv=open(pathtasks_argv,O_RDONLY);
      if(fd_argv==-1){
        free(pathtasks_argv);
        continue;
      } else {
        free(pathtasks_argv);
        close(fd_argv);
      }

      //--------------On decide si il est temps d'executer la tache---------------
      char * pathtime = path_constr(pathtime,path_to_task,CMD_TEXEC);
      assert(pathtime!=NULL);

      //On lis la structure timing de notre task

      int fd_time=open(pathtime,O_RDONLY);//OPEN
      if(fd_time==-1){
        free(path_to_task);
        free(pathtime);
        continue;
      }
      uint64_t minutes;
      uint32_t heures;
      uint8_t jours;
      lseek(fd_time,  0,  SEEK_SET);//Tête de lecture au début du fichier pour lire time_exec

      int rd=read(fd_time,&minutes,sizeof(uint64_t));//READ
      rd=read(fd_time,&heures,sizeof(uint32_t));//READ
      rd=read(fd_time,&jours,sizeof(uint8_t));//READ
      
      free(pathtime);
      close(fd_time);

      //On convertie les uint en field string pour les comparer qu'on recupere dans nos buffers
      char * buf_minutes=malloc(TIMING_TEXT_MIN_BUFFERSIZE);//MALLOC
      assert(buf_minutes!=NULL);
      char * buf_heures=malloc(TIMING_TEXT_MIN_BUFFERSIZE);//MALLOC
      assert(buf_heures!=NULL);
      char * buf_jours=malloc(TIMING_TEXT_MIN_BUFFERSIZE);//MALLOC
      assert(buf_jours!=NULL);

      timing_string_from_field(buf_minutes, 0, 59, minutes);//Minutes
      timing_string_from_field(buf_heures, 0, 23, heures);// Hours
      timing_string_from_field(buf_jours, 0, 6, jours);// Jours

      //On test si les minutes et les heures et les jours de notre structure ptime sont compris dans l'expression crones de la fonction
      if(compaire_cron(ptime->tm_min,buf_minutes) && compaire_cron(ptime->tm_hour,buf_heures) && compaire_cron(ptime->tm_wday,buf_jours)){
        //FREE times
        free(buf_minutes);
        free(buf_heures);
        free(buf_jours);
        
        pid_t pid3 = fork();
        if(pid3==0){
          pid_t pid=fork();
          if(pid==0){//FILS          
            //fonction qui va executer la tache en question
            execute_a_task(path_to_task,dir);
          }
          else
          {//GESTION fichier TIME-EXIT_CODE
            write_exit_code(path_to_task,pid2,time2);//ecrit l'exitcode de son fils qui lance l'exec et le temps d'exec dans le bon file
            //FREE
            free(path_to_task);
          }
        } else {
          //FREE
          free(path_to_task);
        }
      }
    }
  }
  while(wait(NULL)!=-1);
  free(pathtasks);
  closedir(dir);
  exit(EXIT_SUCCESS);
}
