//Cette fonction nous permettra de gérer la requête CREATE envoyé par cassini au démon
//Elle va permettre la création d'une nouvelle tâche dans un répertoire correspondant à la nouvelle tâche
//Et créer des fichiers l'un contenant sa structure timing, et l'autre son commandline

int cmd_create (int fd_request,int fd_reply,char * buftaskmax,char * pathtasks){
  int fd_buftask=open(buftaskmax,O_RDWR);//Nom attribué au fichier contenant la valeur de la dernière tâche créée
  uint64_t taskmax_name;//La valeur uint64_t attribué à la taskid
  uint32_t u32;
  int exit_status = EXIT_SUCCESS;

  if(fd_buftask==-1){//Dans le cas ou se fichier n'existe pas, c'est la toute première tâche que l'on crée
    taskmax_name = (uint64_t) 1;
    fd_buftask = open(buftaskmax, O_CREAT | O_WRONLY ,0751);//On crée le fichier

    if(write(fd_buftask,&taskmax_name,sizeof(uint64_t))==-1){//Et on écrit la valeur de la tâche que l'on va créer
        exit_status=EXIT_FAILURE;
        goto error_tsk;
    }
  } else {
    //Sinon, c'est que le fichier existe déjà
    if(read(fd_buftask,&taskmax_name,sizeof(uint64_t))==-1){//On lit la valeur de la dernière tâche créée
        exit_status=EXIT_FAILURE;
        goto error_tsk;
    }
    //Et ici on va incrémenter la valeur présente dans taskmax_name
    close(fd_buftask);
    fd_buftask=open(buftaskmax,O_RDWR | O_TRUNC);
    
    taskmax_name=taskmax_name+(uint64_t)1;
    
    if(write(fd_buftask,&taskmax_name,sizeof(uint64_t))==-1){
        exit_status=EXIT_FAILURE;
        goto error_tsk;
    }
  }
  //Après avoir récupérer la valeur de l'id que l'on va créer

  //cr
  //On va créer son répertoire correspondant  
  char * pathtasks_file=malloc(PATH_SIZE);//MALLOC
  assert(pathtasks_file!=NULL);
  strcpy(pathtasks_file,pathtasks);
  snprintf(pathtasks_file + strlen(pathtasks), PATH_SIZE-strlen(pathtasks),"%"PRIu64,taskmax_name );
  
  //Et créer les fichiers contenant le temps d'execution, le command line et l'exit code
  char * pathtasks_timexec=path_constr(pathtasks_timexec,pathtasks_file,CMD_TEXEC);//MALLOC
  char * pathtasks_argv=path_constr(pathtasks_argv,pathtasks_file,CMD_ARGV);//MALLOC          
  char * pathtasks_exit=path_constr(pathtasks_exit,pathtasks_file,CMD_EXIT);//MALLOC
  assert(pathtasks_timexec!=NULL);
  assert(pathtasks_argv!=NULL);
  assert(pathtasks_exit!=NULL);

  if(mkdir(pathtasks_file,0751)==-1){
    exit_status=EXIT_FAILURE;
        goto error_kdir;
  }//file
  int fd_timexec=open(pathtasks_timexec,O_CREAT | O_WRONLY,0751);//TIME_EXIT
  int fd_argv=open(pathtasks_argv,O_CREAT | O_WRONLY, 0751);//ARGV
  int fd_exit=open(pathtasks_exit,O_CREAT | O_WRONLY, 0751);//EXIT_CODE
  if(fd_timexec == -1 || fd_argv == -1 || fd_exit == -1){
      exit_status=EXIT_FAILURE;
      goto error_cmd;
  }

  //Après création de nos fichier, il faut récupérer les données envoyés par cassini
  //Afin de les mettre dans nos fichiers
  char * temps=malloc(sizeof(uint64_t)+sizeof(uint32_t)+sizeof(uint8_t));
  assert(temps!=NULL);
  
  if(read(fd_request,temps,sizeof(uint64_t)+sizeof(uint32_t)+sizeof(uint8_t))==-1){
      exit_status=EXIT_FAILURE;
      goto error_cmd;//READ
  }
  uint64_t hto_min;
  uint32_t hto_heure;
  uint8_t hto_jour;

  //Time en HTOBE
  memcpy(&hto_min,temps,sizeof(uint64_t));//MINUTES
  hto_min = htobe64(hto_min);
  memcpy(temps,&hto_min,sizeof(uint64_t));//HTOBE MINUTES

  memcpy(&hto_heure,temps+sizeof(uint64_t),sizeof(uint32_t));//HEURES
  hto_heure = htobe32(hto_heure);
  memcpy(temps+sizeof(uint64_t),&hto_heure,sizeof(uint32_t));//HTOBE HEURES
  //

  if(write(fd_timexec,temps,sizeof(uint64_t)+sizeof(uint32_t)+sizeof(uint8_t))==-1){
      exit_status=EXIT_FAILURE;
      goto error_cmd;//WRITE
  }

  //COMMANDLINE
  //ARGC * <string>
  if(read(fd_request,&u32,sizeof(uint32_t))==-1){
      exit_status=EXIT_FAILURE;
      goto error_cmd;//READ argc
  }
  u32 = htobe32(u32);
  if(write(fd_argv,&u32,sizeof(uint32_t))==-1){
      exit_status=EXIT_FAILURE;
      goto error_cmd;//WRITE
  }
  //Ici parcours de la structure string, on récupère L et le char *
  for(int j=0;j<u32;j++){
    uint32_t taille;//taille de argv[pos]
    if(read(fd_request,&taille,sizeof(uint32_t))==-1){
        exit_status=EXIT_FAILURE;
        goto error_cmd;//READ argv[i] size
    }
    taille = htobe32(taille);//htobe32
        
    char * str_argv=malloc(sizeof(char)*taille);//MALLOC
    assert(str_argv!=NULL);

    if(read(fd_request,str_argv,sizeof(char)*taille)==-1){
        exit_status=EXIT_FAILURE;
        goto error_cmd;//READ string
    }
    if(write(fd_argv,&taille,sizeof(uint32_t))==-1){
        exit_status=EXIT_FAILURE;
        goto error_cmd;//WRITE
    }
    if(write(fd_argv,str_argv,sizeof(char)*taille)==-1){
        exit_status=EXIT_FAILURE;
        goto error_cmd;//WRITE
    }
    free(str_argv);
  }

  //On envoie ensuite à cassini notre réponse
  char *buf=malloc(sizeof(uint16_t)+sizeof(uint64_t));//MALLOC
  assert(buf!=NULL);
  uint16_t ok_reply= htobe16(SERVER_REPLY_OK);//OK
  
  taskmax_name = htobe64(taskmax_name);//Numéro de la taskid

  memcpy(buf,&ok_reply,sizeof(uint16_t));
  memcpy(buf+sizeof(uint16_t),&taskmax_name,sizeof(uint64_t));
  if(write(fd_reply,buf,sizeof(uint16_t)+sizeof(uint64_t))==-1)exit_status=EXIT_FAILURE;//WRITE
  
  taskmax_name = be64toh(taskmax_name);

  error_cmd:
    close(fd_timexec);
    close(fd_argv);
    close(fd_exit);
    free(buf);
    free(temps);
  error_kdir:
    free(pathtasks_file);
    free(pathtasks_timexec);
    free(pathtasks_argv);
    free(pathtasks_exit);
  error_tsk:
    close(fd_buftask);
  return exit_status;
}
