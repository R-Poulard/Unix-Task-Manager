#include "../include/cmd_std.h"
//Fichier contenant les fonctions servant à l execution des tache GET STDOUT et GET STDERR

int get_std(int fd_request ,int fd_reply, int type, char *pathtasks){
	uint64_t id;
	char * buf;
	read(fd_request,&id, sizeof(uint64_t));//on read le taskid de la tache sujette de la commande
	id = htobe64(id);
	char * pathtasks_file=malloc(PATH_SIZE);//MALLOC
  strcpy(pathtasks_file,pathtasks);
	snprintf(pathtasks_file + strlen(pathtasks), 100-strlen(pathtasks),"%"PRIu64,id );

	DIR * dir = opendir(pathtasks_file); // Après avoir récuperé le chemin on open le directory
	if(dir==NULL){
	  not_found(fd_reply); 
	// Si le répertoire ne s'est pas ouvert alors il n'existe pas on renvoie donc l'erreur (TASK NOT FOUND)
	}
  	else
  	{
	//Dans le cas contraire on ouvre quand même argv pour verifié que la taches n'est pas zombie 
	//(expliqué dans le rapport plus en détail)
	  	char * pathtasks_argv = path_constr(pathtasks_argv,pathtasks_file,CMD_ARGV);
		int fd_argv = open(pathtasks_argv, O_RDONLY);
		if(fd_argv==-1){
			not_found(fd_reply);//On renvoie l'erreur dans ce cas là aussi
			return EXIT_SUCCESS;
		} else {
			close(fd_argv);
		}
		//On va ensuite créer le path en fonction de si on veut le stdout ou le stderr de la tache
		char * pathtasks_std;
		if(type){
			pathtasks_std = path_constr(pathtasks_std,pathtasks_file,CMD_STDOUT);
		} else {
			pathtasks_std = path_constr(pathtasks_std,pathtasks_file,CMD_STDERR);
		}
		//Puis on va ouvrir le file correspondant le notre tache
		int fd_std = open(pathtasks_std, O_RDONLY);//OPEN
		if(fd_std== -1){
			//Si le file ne s'ouvre pas alors la tache n'a jamais été executé, on renvoie donc l'erreur correspondant
			buf=malloc(2*sizeof(uint16_t));//MALLOC
			uint16_t uerror= htobe16(SERVER_REPLY_ERROR);
			uint16_t uerror_type= htobe16(SERVER_REPLY_ERROR_NEVER_RUN);
			memcpy(buf,&uerror,sizeof(uint16_t));
			memcpy(buf+sizeof(uint16_t),&uerror_type,sizeof(uint16_t));
			write(fd_reply,buf,2*sizeof(uint16_t));//WRITE
			free(buf);
		} else {
			int size_buf = BUFFER_BLOCK;

			char *buffer = malloc(size_buf);//On va créer un buffer pour read la taille du file
			int taille_buff = size_buf;
			uint32_t taille = 0;//On garde en mémoire la taille de ce que l'on veut lire
			int rd;

			while((rd = read(fd_std, buffer+taille, size_buf))> 0){
			//tant que on a pas atteint une EOF  on va lire et si on s'apprête à dépassé alors on realloc
				taille = taille+rd;
				if(rd == size_buf){
				  buffer = realloc(buffer, taille_buff + size_buf);
				  taille_buff = taille_buff + size_buf;
				}
			}
			//On va ensuite créer le buffer final qui va contenir la réponse final
			uint16_t ok = htobe16(SERVER_REPLY_OK);
			uint32_t hto_taille = htobe32(taille);

			char *buff_total = malloc(sizeof(uint16_t)+ sizeof(uint32_t)+taille);//MALLOC du buffer
			memcpy(buff_total,&ok, sizeof(uint16_t));//On ajoute "ok" et la taille du string
			memcpy(buff_total+sizeof(uint16_t), &hto_taille , sizeof(uint32_t));
			//Puis on copie le buffer de char dans notre buffer final
			strncpy(buff_total+sizeof(uint16_t)+sizeof(uint32_t), buffer, taille);
			write_in_block(fd_reply,buff_total,sizeof(uint16_t)+ sizeof(uint32_t)+taille);//On write la réponse dans le tube


      free(buffer);
		  close(fd_std);
    }
    free(pathtasks_std);
	}
  free(pathtasks_file);
}
