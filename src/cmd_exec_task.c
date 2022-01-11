//EXTENSION
//Cette fonction permettra après la requête de cassini
//De directement permettre l'exécution d'une tâche spécifié
//Pour cela, on récupère le numéro de la tâche spécifié par cassini,
//Dans le cas ou celle-ci n'existe pas

//cassini envoie après l'opération la taskid et c'est celle-ci que l'on récupère
int to_execute(int fd_reply,int fd_request,char * pathtasks,pid_t pid2,time_t time2){
  uint64_t taskid;
  read(fd_request,&taskid,sizeof(uint64_t));
  taskid=htobe64(taskid);//On attribu le numéro de tâche spécifié par cassini

  //EXECUTION
  char *path_to_task = malloc(PATH_SIZE);//MALLOC
  strcpy(path_to_task,pathtasks);
  snprintf(path_to_task + strlen(pathtasks), PATH_SIZE-strlen(pathtasks),"%"PRIu64,taskid);
  //Ici on va ouvrir le répertoire correspondant à au numéro de tâche spécifié
  //On vérifie ensuite si celle-ci est existante
  int fd_task;
  if(fd_task=open(path_to_task,O_RDONLY)==-1 ){
    not_found(fd_reply);
    goto error;
  } else {
    close(fd_task);
  }
  //Dans le cas ou celle-ci existe on va exécuter nos fonctions d'execution d'une tâche
  //Les fonctions prennent le chemin vers la tâche et execute la tâche
  // * puis mettent à jour le stdout , le stderr et l'exit code 
  pid_t pid3 = fork();
  
  if(pid3==0){
    pid_t pid=fork();
    if(pid==0){//FILS          
      //fonction qui va executer la tache en question        
      execute_a_task(path_to_task,NULL);
    }
    else
    {//GESTION fichier TIME-EXIT_CODE
      wait(NULL);
      write_exit_code(path_to_task,pid2,time2);//ecrit l'exitcode de son fils qui lance l'exec et le temps d'exec dans le bon file
      //FREE
      free(path_to_task);
    }
  } else {
    //FREE
    wait(NULL);
    //Ok si tout s'est bien déroulé
    uint16_t ok = htobe16(SERVER_REPLY_OK);
    write(fd_reply,&ok,sizeof(uint16_t));

    free(path_to_task);
    return EXIT_SUCCESS;
  }
  error:
  return EXIT_FAILURE;
}
