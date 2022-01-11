#include "utils.h"
//Fichier contenant des fonction utilitaires plutôt générique et sans effet de bord important

// Copie size octet de buf1 dans buf2 en rajoutant '\0' à la fin
int cpy(char * buf1,char *buf2,size_t size){
  strncpy(buf2,buf1,size);
  buf2[size]='\0';
  return 1;
}

//Permet de copier les char apres le dernier "del" de global dans subside

int parsec (char global [],char* subside,char del){
  int last_one=-1;//retiens la position du dernier char de type 'del'
  int i=0;
  while(global[i]!='\0'){//On attend jusqu'à arrive à la fin du file
    if(global[i]==del){ // On verifie la valeur du char
      last_one=i;
    }
    i++;    
  }
  if(last_one==-1){ //Cas ou on a pas trouver d'occurence de del on return 0 et on ne change pas subside et general
    return 0;
  }
  else{
    //subside=malloc(sizeof(char)*strlen(global+last_one+1));
    strncpy(subside,global+last_one+1,strlen(global+last_one)); // On copie dans subside l'apres"last_one"
    global[last_one]='\0'; //On rajoute '\0' à la place de last_one afin de redefinir la taille de global
    return 1;
  }
  
}
//Compaire l'expression time Cron en delimitant d'abord les , puis les potentiels -
//return 1 si true et 0 si false

int compaire_cron(int NOSMINUTES,char* minutes){
  char *current=malloc(500);
  assert(current!=NULL);
  if(strchr(minutes,'*')!=NULL)return 1;//Si on a un * alors c'est true forcement
  while(parsec(minutes,current,',')!=0){ //tant que on a des ,
	    
    char* siunder = malloc(256);
    assert(siunder!=NULL);
    int y = parsec(current,siunder,'-'); // On parsec sur le potentiel -

    if(y==0){//On a pas de - alors c'est une valeur numerique que l'on compare

      if(atoi(current)==NOSMINUTES){
        free(current);
        return 1; // On stop des qu'on a un true
      }
    }
    else{ // Si on a un - on recupere l'intervalle
      int petit =atoi(current);
      int grand=atoi(siunder);
      //On regarde si notre temps est compris dans l'intervalle
      if(petit<=NOSMINUTES && NOSMINUTES <= grand){
        free(current);
        return 1;
      }
    }
  }
//Ceci permet de traité la dernier itération (derniere ,)
//On utilise le meme algorithme que précedement
  char* siunder=malloc(500);
  assert(siunder!=NULL);
    if(parsec(minutes,siunder,'-')==0){
      if(atoi(minutes)==NOSMINUTES){
        free(current);
        return 1;
      }
    }
    else{
      int petit =atoi(minutes);
      int grand=atoi(siunder);
      if(petit<=NOSMINUTES && NOSMINUTES <= grand){
        free(current);
        free(siunder);
        return 1;
      }
     }
  free(current);
  free(siunder);
  
  return 0; //return false si on a pas de true apres toutes les comparaison
}
//Construit final_path dans le pattern, old_path/file

char* path_constr(char * final_path, char *old_path, char *file){
    final_path = malloc(100);
    assert(final_path!=NULL);
    strcpy(final_path, old_path);
    strcat(final_path,"/");
    strcat(final_path, file);
    return final_path;
}

//Ecrit par block de BUFFER_BLOCK dans le pipe le buffer jusqu'à que tout soit écrit
//Exit avec l'exit code adequat si jamais un erreur se produit
//Permet de ne pas faire deborder le pipe

int write_in_block(int fd_reply,char* buffer,size_t buffersize){
	size_t written=0;//acc permettant de garder compte du nbr d'octet d'écrit
	int w;
	while(written!=buffersize){//tant qu'on a pas tout écrit
		if(written+BUFFER_BLOCK<=buffersize){
			//Si on peut encore écrit qqchose de la taille de BUFFER_BLOCK dans le pipe
			w=write(fd_reply,buffer+written,BUFFER_BLOCK);
		}
		else{
			//Si non alors on écrit la suite
			w=write(fd_reply,buffer+written,buffersize-written);
		}
		if(w==-1)return EXIT_FAILURE;
		written+=w;
	}
	return EXIT_SUCCESS;
}


//Cette fonction prend en argument le descripteur de fichier correspondant au tube réponse
//Elle n'est à utilisé seulement lorsque l'on veut envoyer une erreur (ici 2 uint16_t)
int not_found(int fd_reply){//potentiellement cette fonction dans exit code
	char * buf=malloc(2*sizeof(uint16_t));
    uint16_t uerror= htobe16(SERVER_REPLY_ERROR);
    uint16_t uerror_type= htobe16(SERVER_REPLY_ERROR_NOT_FOUND);
    memcpy(buf,&uerror,sizeof(uint16_t));
    memcpy(buf+sizeof(uint16_t),&uerror_type,sizeof(uint16_t));
    write(fd_reply,buf,2*sizeof(uint16_t));
    free(buf);	
}  
