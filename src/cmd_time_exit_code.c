int do_time_exit_code(char * pathtasks_file,int fd_reply){	// Donne a cassini l'ensemble des temps d'execution de la tache pathtasks_file avec leurs code de sortie
	
	char * pathtasks_std=path_constr(pathtasks_std,pathtasks_file,CMD_EXIT);	// Chemins du fichier contenant les temps d'execution et les codes de sortie
    assert(pathtasks_std!=NULL);
    int fd_std = open(pathtasks_std, O_RDONLY);//OPEN : Ouverture du fichier contenant les temps d'execution et les sorties de pathtasks_file
    //if(fd_std<0)goto error;          
    int taille_buff = sizeof(int64_t)+sizeof(uint16_t);	// Taille de base d'un couple temps d'execution/codes de sortie
    char *buffer = malloc(taille_buff); //MALLOC : buffer va contenir la liste des couples temps d'execution/codes de sortie
    assert(buffer!=NULL);
    int rd;
    int size_extime = sizeof(int64_t)+sizeof(uint16_t);
                
    uint32_t taille = 0;
                
    while(rd = read(fd_std, buffer+taille*size_extime, size_extime) >0){
       int64_t hto_timing;
       memcpy(&hto_timing,buffer+taille*size_extime,sizeof(int64_t));	
       hto_timing=htobe64(hto_timing);
       memcpy(buffer+taille*size_extime,&hto_timing,sizeof(int64_t)); //Copie du temps dans buffer
                  
       taille ++;
       buffer = realloc(buffer, taille_buff + size_extime);//REALLOC : Augmentation de la taille de buffer pour ajouter d'autres ouples temps d'execution/code
       
       taille_buff = taille_buff + size_extime; //On garde la taille de buffer à jour
    }
                
    uint16_t ok = htobe16(SERVER_REPLY_OK);
    uint32_t hto_taille = htobe32(taille);
                
    char * buff_total = malloc(sizeof(uint16_t)+ sizeof(uint32_t)+taille*size_extime);//MALLOC : buff_total va contenir la reponse 
    assert(buff_total!=NULL);
           
           
    memcpy(buff_total,&ok, sizeof(uint16_t));	// OK ajoute a la reponse
    memcpy(buff_total+sizeof(uint16_t), &hto_taille , sizeof(uint32_t));	// Nombre d'executions de la tache ajoutee a la reponse
    memcpy(buff_total+sizeof(uint16_t)+sizeof(uint32_t), buffer, taille*size_extime);

    write(fd_reply, buff_total,sizeof(uint16_t)+ sizeof(uint32_t)+taille*size_extime);//WRITE : Ecriture de la reponse a cassini
    //FREE
    close(fd_std);
    free(buffer);
    free(buff_total);
    free(pathtasks_std);
    
    error:
       close(fd_std);
       free(pathtasks_std);

    
}

int error_not_found(int fd_reply){	// Dans le cas ou la tache n'a pas encore ete executee
	 char *buf=malloc(2*sizeof(uint16_t));
	 assert(buf!=NULL);
     uint16_t uerror= htobe16(SERVER_REPLY_ERROR);	//
     uint16_t uerror_type= htobe16(SERVER_REPLY_ERROR_NOT_FOUND);
     memcpy(buf,&uerror,sizeof(uint16_t));
     memcpy(buf+sizeof(uint16_t),&uerror_type,sizeof(uint16_t));
     write(fd_reply,buf,2*sizeof(uint16_t));//WRITE : Ecriture de l'erreur et du code d'erreur a cassini
     //FREE
     free(buf);	
}

int cmd_time_exit_code(int fd_request,int fd_reply, uint64_t taskid, char *pathtasks_file, char *pathtasks){
	DIR * dir;
	read(fd_request,&taskid, sizeof(uint64_t));//READ
    taskid = htobe64(taskid);
            
    pathtasks_file=malloc(PATH_SIZE);//MALLOC
	assert(pathtasks_file!=NULL);
    strcpy(pathtasks_file,pathtasks);
    snprintf(pathtasks_file + strlen(pathtasks), 100-strlen(pathtasks),"%"PRIu64,taskid );
            
    dir = opendir(pathtasks_file);//POINTEUR
    if(dir==NULL){	// Si la tache n'existe pas...
      error_not_found(fd_reply);	// On repond une erreur a cassini
    } else {
      do_time_exit_code(pathtasks_file,fd_reply);
    }        

}


			
