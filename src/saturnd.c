#include "utils.c"
#include "cmd_create.c"
#include "cmd_list.c"
#include "cmd_time_exit_code.c"
#include "cmd_std.c"
#include "cmd_remove.c"
#include "cmd_switch_time.c"
#include "cmd_set_cmd_task.c"
#include "cmd_exec_task.c"
#include "cmd_reset_taskmax.c"
#include "saturnd_child.c"

void handler(int s){
//Handler de signal du fils, 
//permettant d'attendre toutes les pulsations zombies et de quitter lorsque son père envoie SIGTERM
  switch(s){
    case SIGALRM: 
    break;
    case SIGTERM:
      while(waitpid(-1,NULL,WNOHANG)>0);
      exit(EXIT_SUCCESS);
    break;
  }
}

int main(){
  //On créer l'arborescence et le repertoire de tache si ces derniers ne sont pas présent déjà
  char * pipes_directory;
  char * username_str = getlogin();
  
  //creation du répertoire /tmp/$login
  pipes_directory = malloc(PATH_SIZE);
  strcpy(pipes_directory, "/tmp/");
  strcat(pipes_directory, username_str);
  mkdir(pipes_directory,0751);
  
  //creation du répertoire "saturnd"
  strcat(pipes_directory, "/saturnd");
  mkdir(pipes_directory,0751);
  
  //cration du répertoire /pipes dans tmp/$login
  strcat(pipes_directory, "/pipes");
  mkdir(pipes_directory,0751);
  
  //creation des chemins des FIFO
  char *fifo_request=malloc(PATH_SIZE);
  strcpy(fifo_request,pipes_directory);
  strcat(fifo_request,"/saturnd-request-pipe");
  
  char *fifo_reply = malloc(PATH_SIZE);
  strcpy(fifo_reply,pipes_directory);
  strcat(fifo_reply,"/saturnd-reply-pipe");  
  
  
  char * pathtasks_saturnd = malloc(PATH_SIZE);
  strcpy(pathtasks_saturnd,"./saturnd_dir");
  mkdir(pathtasks_saturnd, 0751);//MKDIR
     
  char * pathtasks = malloc(PATH_SIZE);//MALLOC
  strcpy(pathtasks,"./saturnd_dir/tasks/");
    
  mkdir(pathtasks,0751);//MKDIR
  struct stat stat_request;
  struct stat stat_reply;
  
  //Si les pipes existent déjà, alors ils ne sont pas recrée
  if(stat(fifo_request,&stat_request)==-1 || S_ISFIFO(stat_request.st_mode)==0){
    mkfifo(fifo_request,0751);//MKFIFO
  }
  if(stat(fifo_reply,&stat_reply)==-1 || S_ISFIFO(stat_reply.st_mode)==0){
    mkfifo(fifo_reply,0751);//MKFIFO
  }
  
  //Buffer servant de chemin de la taskmax lors du create
  char * buftaskmax;
  buftaskmax = path_constr(buftaskmax,pathtasks_saturnd,"taskid_max");//MALLOC
   
  free(pipes_directory);

  //creation d'un fils
  pid_t pid = fork();//FORK
  assert(pid!=-1);
  if(pid>0){
    //ouverture des FIFO
    int fd_request=open(fifo_request,O_RDONLY);//OPEN
    int fd_reply=open(fifo_reply,O_WRONLY);//OPEN

    free(fifo_request);
    free(fifo_reply);

    //Variable servant de buffer lors de la lecture de la commande id et de la task id
    uint16_t u16;
    uint64_t taskid;

    char * buf;
    int fd_buftask;
    
    int rd;
    time_t time2;
    
    int out=0; //permet de sortir du while lors du terminate (nous évite de faire un double break)
    while(out==0){ 
      //On lit l'u16 (la commande id) que cassini a envoyé qu'on match afin de faire la bonne commande
      rd = read(fd_request,&u16,sizeof(uint16_t));
      if(rd>0){  
        u16=htobe16(u16);
    
        switch (u16){                         
          case CLIENT_REQUEST_DELETE_ALL: 
            remove_all(pathtasks,fd_reply,pathtasks_saturnd);
          break;  
          case CLIENT_REQUEST_SW_TIME: 
            cmd_switch_time(fd_reply,fd_request,pathtasks); 
          break;
          case CLIENT_REQUEST_EXEC_TASK:
            time2 = time(NULL);
            to_execute(fd_reply,fd_request,pathtasks,pid,time2);
          break;
          case CLIENT_REQUEST_REMOVE_TASK:
            cmd_remove_task(fd_request,fd_reply ,pathtasks,taskid);
          break;
          case CLIENT_REQUEST_CREATE_TASK: 
            cmd_create (fd_request,fd_reply,buftaskmax,pathtasks);
          break;
          case CLIENT_REQUEST_SET_CMD:
            cmd_set_cmd_task(fd_request,fd_reply,pathtasks);
          break;  
          case CLIENT_REQUEST_TERMINATE:
	    //OUT =1 pour sorti du while et on envoie SIGTERM a notre fils  
            out=1;
	    kill(pid,SIGTERM); 
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
          case CLIENT_REQUEST_RESET_TASKMAX:
            reset_taskmax(fd_reply,buftaskmax,pathtasks);
          break; 
        }
		  } 
    } 

    wait(NULL);//On attends le retour du fils
    //On va ecrire OK à cassini et se stopper   
    uint16_t ok= SERVER_REPLY_OK;
    write(fd_reply,&ok,sizeof(uint16_t));
    close(fd_request);
    close(fd_reply);
    
  }//FIN GRAND PERE
  else
  {
    //on initialise une variable temps, qui sera transmisse au plusation
    time_t time2 = time(NULL);
    uint16_t code;
    //On initialise le sigaction et on lui relie le handler écrit plus haut
    struct sigaction s;
    s.sa_handler = handler; 
    
    while(1){
      //On va crée une pulsation puis rester bloqué 60sec avant de relancer le while et d'en créer une nouvelle
      pid_t pid2=fork();
      if(pid2>0){
	//On met ici le sigaction pour éviter que nos pulsation hérite aussi du gestionnaire handler
        sigaction(SIGALRM,&s, NULL);
        sigaction(SIGTERM,&s, NULL);
        //Le fils se reveil toutes les 60 sec (ou plus tôt si il y a SIGTERM) en ignorant SIGALARM
	alarm(60);
        pause(); 
        waitpid(pid2,NULL,WNOHANG);//Il attend un potentiel fils zombie
        time2=time(NULL);//Il réinitialise son temps pour le transmettre à son fils
      }
      else
      {//PETIT FILS 
	//La pulsation va faire son travail
        executioner_pulse(pid2,time2);
      }
    }
    //Si le pere nous a ordonné de terminé On attend la totalité de ses fils zombie puis on exit
    while (wait(NULL) > 0);
    free(pathtasks_saturnd);
    exit(EXIT_SUCCESS);
  }
}
