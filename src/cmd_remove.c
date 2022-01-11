//Répertorie plusieurs fonctions pour la gestion de suppression d'une tâche ou plusieurss
//Et le cas de gestion d'erreur ou l'on doit envoyer la réponse erreur, et erreur not found
     
//Cette méthode prend le chemin vers le répertoire d'tâche, et le descripteur du tube de réponse
//Et permet de supprimer les fichiers que l'on a créer dans la tâche, et le répertoire de la tâche spécifiée
int remove_task(char *pathtasks_id, int fd_reply){
	DIR * dir=opendir(pathtasks_id);//POINTEUR
	char *buf;
	int exit_code=EXIT_SUCCESS;
    if(dir==NULL){ 
      not_found(fd_reply);   
	  return EXIT_FAILURE;  
    }
    int r;
    
    buf = path_constr (buf, pathtasks_id,CMD_ARGV);//argv
    r=remove(buf);//Remove

    buf = path_constr (buf, pathtasks_id,CMD_TEXEC);//time exit
    r=remove(buf);//Remove

    buf = path_constr (buf, pathtasks_id,CMD_EXIT);//exit code
    r=remove(buf);//Remove

    buf = path_constr (buf, pathtasks_id,CMD_STDOUT);//stdout
    r=remove(buf);//Remove

    buf = path_constr (buf, pathtasks_id,CMD_STDERR);//stdout
    r=remove(buf);//Remove
    
    r=rmdir(pathtasks_id);
    return exit_code;
}

//Ici c'est ce qui nous permettra de gérer la suppression d'une tâche, et d'envoyer la bonne réponse
//* à cassini
int cmd_remove_task(int fd_request, int fd_reply ,char *pathtasks, uint64_t taskid ){
	read(fd_request,&taskid,sizeof(uint64_t));//READ du TASKID
    taskid=htobe64(taskid);
            
	char * pathtasks_id = malloc(PATH_SIZE);//MALLOC
	strcpy(pathtasks_id,pathtasks);
    snprintf(pathtasks_id + strlen(pathtasks), PATH_SIZE-strlen(pathtasks),"%"PRIu64,taskid);

	if(remove_task(pathtasks_id,fd_reply)==EXIT_SUCCESS){
	  uint16_t ok = htobe16(SERVER_REPLY_OK);
          int w=write(fd_reply,&ok,sizeof(uint16_t));
	  return EXIT_SUCCESS;
	}
	return EXIT_FAILURE;
}

//Cette méthode nous permettra de supprimer toutes les tâches créées jusqu'ici
//Elle parcours notre répertoire contenant toutes nos tâches et applique nos fonction de suppression d'une tâche
int remove_all(char *pathtasks, int fd_reply,char *pathtasks_saturnd){
	DIR * dir = opendir(pathtasks);
	struct dirent *struct_dir;
	uint32_t nb_tasks = 0;
	
	while((struct_dir=readdir(dir))){
		if(strcmp(struct_dir->d_name,".")!=0 && strcmp(struct_dir->d_name,"..")!=0){
			
			char *path_to_task = malloc(strlen(pathtasks)+strlen(struct_dir->d_name)+1);
			assert(path_to_task!=NULL);
			strcpy(path_to_task, pathtasks);
			strcat(path_to_task, struct_dir->d_name);
		
			remove_task(path_to_task, fd_reply);
			nb_tasks++;
			free(path_to_task);
		}
			
	} 

	nb_tasks = htobe32(nb_tasks);
	
	//Suppression du file taskmax:
	//char* path_to_taskmax = path_constr(path_to_taskmax,pathtasks_saturnd, "taskid_max" );
	//remove(path_to_taskmax);
	uint16_t ok = htobe16(SERVER_REPLY_OK);
	char *buff = malloc(sizeof(uint16_t)+sizeof(uint32_t));
	assert(buff!=NULL);
	memcpy(buff,&ok,sizeof(uint16_t));
	memcpy(buff+sizeof(uint16_t),&nb_tasks,sizeof(uint32_t));
	write(fd_reply,buff,sizeof(uint16_t)+sizeof(uint32_t));
	
	//FREE
	free(buff);
	//free(path_to_taskmax);
	
}
