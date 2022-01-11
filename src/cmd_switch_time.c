//Après réception de la requête pour effectuer cette commande
//Cette commande nous permettra de modifier le temps à laquelle la commande associé à une tâche
// doit être exécuter, pour cela le numéro de la tâche est récupérer, et on récupère dans le fifo de requête
// les nouveaux temps attribués afin de l'écrire dans le fichier correspondant au temps ou la commande peut être exécutée
int cmd_switch_time(int fd_reply,int fd_request,char *pathtasks){
    uint64_t id;
    char * buf;
    int rd=read(fd_request,&id, sizeof(uint64_t));//TASKID
    if(rd==-1)goto error0;
    id = htobe64(id);

    char * pathtasks_file=malloc(PATH_SIZE);
    assert(pathtasks_file!=NULL);

    strcpy(pathtasks_file,pathtasks);
    snprintf(pathtasks_file + strlen(pathtasks), PATH_SIZE-strlen(pathtasks),"%"PRIu64,id );
     
    DIR * dir = opendir(pathtasks_file);
    if(dir==NULL){
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

    char * pathtasks_timing = path_constr(pathtasks_timing,pathtasks_file,CMD_TEXEC);
    remove(pathtasks_timing);
    int fd_time = open(pathtasks_timing, O_CREAT | O_WRONLY,0751);
    if(fd_time==-1){
        goto error2;
    }
    
    char * temps=malloc(sizeof(uint64_t)+sizeof(uint32_t)+sizeof(uint8_t));
  assert(temps!=NULL);

    if(read(fd_request,temps,sizeof(uint64_t)+sizeof(uint32_t)+sizeof(uint8_t))==-1){
        goto error3;
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

    memcpy(&hto_jour,temps+sizeof(uint64_t)+sizeof(uint32_t),sizeof(uint8_t));//HEURES

    if(write(fd_time,temps,sizeof(uint64_t)+sizeof(uint32_t)+sizeof(uint8_t))==-1){
      goto error3;
    }      
    buf=malloc(sizeof(uint16_t));//MALLOC
    assert(buf!=NULL);
    
    //Réponse à cassini
    uint16_t ok_reply= SERVER_REPLY_OK;    
    if(write(fd_reply,&ok_reply,sizeof(uint16_t))==-1)goto error4;
    
    free(pathtasks_file);
    closedir(dir);
    free(buf);
    free(pathtasks_timing);
    free(temps);
    close(fd_time);
    return EXIT_SUCCESS;
    
    error4:
        free(buf);
    error3:
        free(temps);
        close(fd_time);
    error2:
        free(pathtasks_timing);
    error1:
        free(pathtasks_file);
        closedir(dir);
    error0:
    return EXIT_FAILURE; 
}
