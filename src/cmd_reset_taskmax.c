//EXTENSION
//Cette commande nous permettra après réception de la requête de cassini, de :
// remettre à jour le numéro de notre tâche à la dernière tâche qui est accessible,
// pour cela on regarde le numéro de la dernière tâche,
// et si notre compteur est supérieur au numéro de la plus grande tâche existante alors on met à jour notre compteur 
int reset_taskmax(int fd_reply,char * buftaskmax,char * pathtasks){
    DIR * dir = opendir(pathtasks); //On ouvre le directory contenant toutes les taches
    assert(dir!=NULL);
    struct dirent *  struct_dir;
    int w;

    uint64_t task_maxi=(uint64_t)0;//Valeur qui correspondra à notre plus grande tâche créée
    while((struct_dir=readdir(dir))){
      if(strcmp(struct_dir->d_name,".")!=0 && strcmp(struct_dir->d_name,"..")!=0){ // On verifie biensur de ne pas avoir ouvert les files . et ..
        uint64_t taskid = strtoull(struct_dir->d_name, NULL, 0);//On recupere l'ID de la tasks, le nom de dossier afin de pouvoir la mettre dans le pipe

        taskid=htobe64(taskid);//htobe necessaire car les fonction strtoull ne formate pas dans le bon sens les bits
        if(taskid>task_maxi){
          task_maxi=taskid;
        }
      }
    }
    int fd_buftask;
    if((fd_buftask=open(buftaskmax,O_RDWR | O_TRUNC))!=-1){//Ici on va mettre notre valeur task_maxi dans notre fichier
        task_maxi = htobe64(task_maxi);

        if(w = write(fd_buftask,&task_maxi,sizeof(uint64_t))==-1){
            goto error_tsk;
        }
        close(fd_buftask);

        //réponse à cassini
        uint16_t ok = htobe16(SERVER_REPLY_OK);

        char *buf_total = malloc(sizeof(uint16_t)+ sizeof(uint64_t));
        memcpy(buf_total,&ok,sizeof(uint16_t));
        memcpy(buf_total+sizeof(uint16_t),&task_maxi,sizeof(uint64_t));

        w=write(fd_reply, buf_total,sizeof(uint16_t)+ sizeof(uint64_t));

    }
    else{//Si notre compteur de tâche n'existait pas, on renvoie une erreur 
        //réponse erreur à cassini
        uint16_t err = htobe16(SERVER_REPLY_ERROR);
        uint16_t type=htobe16(SERVER_REPLY_NO_TASK_ERROR);
		
        char *buf_total = malloc(sizeof(uint16_t)*2);
        memcpy(buf_total,&err,sizeof(uint16_t));
        memcpy(buf_total+sizeof(uint16_t),&type,sizeof(uint16_t));

        w=write(fd_reply, buf_total,sizeof(uint16_t)*2);

    }
    error_tsk:
    if(w==-1){
        closedir(dir);
        return EXIT_FAILURE;
    }
    else{
        closedir(dir);
        return EXIT_SUCCESS;
    }
  }

