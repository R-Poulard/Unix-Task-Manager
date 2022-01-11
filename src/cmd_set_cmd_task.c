//EXTENSION
/*
Après récupération de la requête correspondante à l'exécution de cette méthode
  cette fonction nous servira à modifier la commande à exécuter par une tâche spécifiée
  pour cela, on récupère notre numéro de tâche et la commande spécifiée par le client
*/
int cmd_set_cmd_task(int fd_request,int fd_reply,char *pathtasks){
    uint64_t id;
    uint32_t u32;
    char * buf;
    //Ici on récupère le numéro de la tâche spécifiée
    int rd=read(fd_request,&id, sizeof(uint64_t));//TASKID
    if(rd==-1)goto error0;
    id = htobe64(id);

    //On établit le chemin vers notre tâche afin de pouvoir modifier le fichier correspondant à la commande à exécuter
    char * pathtasks_file=malloc(PATH_SIZE);
    assert(pathtasks_file!=NULL);

    strcpy(pathtasks_file,pathtasks);
    snprintf(pathtasks_file + strlen(pathtasks), PATH_SIZE-strlen(pathtasks),"%"PRIu64,id );
     
    DIR * dir = opendir(pathtasks_file);
    //Ici on vérifie que le chemin vers noter répertoire est existant
    if(dir==NULL){//Dans ce cas, c'est qu'il n'existe pas de tâche avec ce numéro de tâche
        buf=malloc(2*sizeof(uint16_t));//MALLOC
        assert(buf!=NULL);
            
        uint16_t uerror= htobe16(SERVER_REPLY_ERROR);
        uint16_t uerror_type=htobe16(SERVER_REPLY_ERROR_NOT_FOUND);
        memcpy(buf,&uerror,sizeof(uint16_t));
        memcpy(buf+sizeof(uint16_t),&uerror_type,sizeof(uint16_t));
        write(fd_reply,buf,2*sizeof(uint16_t));
        free(buf);
        if(fd_reply==-1)goto error1;
        return EXIT_FAILURE;
    }
    //Dans le cas ou notre répertoire vers notre tâche existe
    // on va manipuler son fichier comportant la commande à exécuter si celui-ci existe (sinon renvoi une erreur)
    char * pathtasks_argv = path_constr(pathtasks_argv,pathtasks_file,CMD_ARGV);
    int fd_argv = open(pathtasks_argv,O_WRONLY | O_TRUNC);
    if(fd_argv==-1){
        goto error2;
    }
    //COMMAND
    //On va alors lire les arguments nécessaires, dans la requête envoyé par cassini
    //soit ARGC, et les <string> (L = uint32_t et chaine = char *)
    //Et on va écrire tout ça dans notre fichier

    if(read(fd_request,&u32,sizeof(uint32_t))==-1){
      goto error3;//READ argc
    }
    u32 = htobe32(u32);
    if(write(fd_argv,&u32,sizeof(uint32_t))==-1){
      goto error3;//WRITE
    }
    for(int j=0;j<u32;j++){
      uint32_t taille;//taille de argv[pos]
      if(read(fd_request,&taille,sizeof(uint32_t))==-1){
        goto error3;//READ argv[i] size
      }
      taille = htobe32(taille);//htobe32
            
      char * str_argv=malloc(sizeof(char)*taille);//MALLOC
      assert(str_argv!=NULL);

      if(read(fd_request,str_argv,sizeof(char)*taille)==-1){
        goto error3;//READ string
      }
      if(write(fd_argv,&taille,sizeof(uint32_t))==-1){
        goto error3;//WRITE
      }
      if(write(fd_argv,str_argv,sizeof(char)*taille)==-1){
        goto error3;//WRITE
      }
      free(str_argv);
    }
    buf=malloc(sizeof(uint16_t));//MALLOC
    assert(buf!=NULL);
    
    //réponse ok
    uint16_t ok_reply= SERVER_REPLY_OK;    
    if(write(fd_reply,&ok_reply,sizeof(uint16_t))==-1)goto error4;
    
    free(pathtasks_file);
    closedir(dir);
    free(buf);
    free(pathtasks_argv);
    close(fd_argv);
    return EXIT_SUCCESS;
    
    error4:
        free(buf);
    error3:
        free(pathtasks_argv);
        close(fd_argv);
    error2:
        free(pathtasks_argv);
    error1:
        free(pathtasks_file);
        closedir(dir);
    error0:
    return EXIT_FAILURE; 
}
