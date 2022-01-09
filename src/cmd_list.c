/* Principe: On va parcourir toutes les taches, recuperer leurs information dans un buffer <buf_inter>, en recuperant la commande line dans un buffer intermediaire <buff_cmd>,
puis on ecrit tout les <buf_inter> un à un dans <buf_all_tasks> et apres avoir parcourus toutes les taches on va alors creer notre buffer final contenant ce qu'on envoie dans le pipe:<buf_total> */
//Return value: Toujours succes sauf si assert fait stopper l'execution (du a une erreur critique de gestion de la mémoire => malloc memcpy ect...)
//              Mais des taches peuvent être omise si on a une erreur de traitement de ces dernieres.
int cmd_list(int fd_reply,char * pathtasks,dirent struct_dir){ //fonction envoyant le protocole d'envoie de la task -LIST comme indiqué dans protocole.md
    size_t TAILLE_TIMING=sizeof(uint64_t)+ sizeof(uint32_t)+sizeof(uint8_t);

    dir = opendir(pathtasks); //On ouvre le directory contenant toutes les taches
    assert(dir==NULL);
    char *buf_all_tasks =malloc(1024);//Creation de notre buffer contenant l'information requise pour toutes les taches
    assert(buf_all_tasks==NULL);
    //On garde en mémoire la taille du buffer que ce soit global ou utilisé pour moduler avec des realloc
    int taille_globale = 1024;
    int taille_utilisee = 0;
    //Nombre de tasks parcourus, variable incrémenté et requise pour l'envoie
    uint32_t nb_tasks = 0;

    //On parcours toutes les taches du files
    while((struct_dir=readdir(dir))){
    if(strcmp(struct_dir->d_name,".")!=0 && strcmp(struct_dir->d_name,"..")!=0){ // On verifie biensur de ne pas avoir ouvert les files . et ..
        nb_tasks++;//On increment le nombre de task total car on en a trouvé une nouvelle

        char * path_to_task=malloc(PATH_SIZE);//Creation du chemin de l'executable jusqu'à la tache
        assert(path_to_task==NULL);

        strcpy(path_to_task,pathtasks);
        strcat(path_to_task,struct_dir->d_name);

        char *buf_inter = malloc(sizeof(uint64_t)+TAILLE_TIMING);//Buffer qui va recuperer toutes les information de la tasks actuel
        assert(buf_inter==NULL);
        taskid = strtoull(struct_dir->d_name, NULL, 0);//On recupere l'ID de la tasks, le nom de dossier afin de pouvoir la mettre dans le pipe

        taskid=htobe64(taskid);//htobe necessaire car les fonction strtoull ne formate pas dans le bon sens les bits

        int mem=memcpy(buf_inter, &taskid, sizeof(uint64_t));//On copie le tasks ID dans notre buffer_inter
        assert(mem==NULL);
        //On va ensuite creer le path du fichier contenant le temps d'execution de la tache
        char * pathtasks_exec=path_constr(pathtasks_exec,path_to_task,CMD_TEXEC);

        int fd_time_exec = open(pathtasks_exec , O_RDONLY);//On ouvre le fichier avec le chemin creer
        if(fd_time_exec==-1)goto error_time_op;

        int rd=read(fd_time_exec,buf_inter+sizeof(uint64_t) , TAILLE_TIMING);//On lit et écrit dans buffer la structure timing qui y est stocké
        if(rd==-1)goto error_time_read;

       //On va ensuite creer le path du fichier contenant la commandline  de la tache
        char * pathtasks_argv=path_constr(pathtasks_argv,path_to_task,CMD_ARGV);

        int fd_argv = open(pathtasks_argv, O_RDONLY);//On ouvre le fichier avec le chemin creer
        if(fd_argv==-1)goto error_argv_op;
        //On va lire la commandline et la stocker dans buffinter

        //On initialise le buffer qui va recuperer la commandline
        char *buf_cmd  = malloc(sizeof(uint32_t));
        assert(buf_cmd==NULL);

        uint32_t argc;
        rd=read(fd_argv, &argc, sizeof(uint32_t)); //On va d'abord lire le nombre d'argument de la commande line
        if(rd==-1)goto error_argv_rd;


        size_t taille_buff = sizeof(uint32_t);

        //On htobe argc et on l'écrit dans notre buffer car il doit être envoyer à cassini
        uint32_t hto_argc = htobe32(argc);
        memcpy(buf_cmd, &hto_argc, sizeof(uint32_t));
        assert(buf_cmd==NULL);
        //On va ensuite parcourir les arguments
        for(int i=0;i<argc;i++){
            //On list et htobe la taille du string de l'argument
            uint32_t taille_str;
            int rd=read(fd_argv,&taille_str,sizeof(uint32_t));
            if(rd==-1)goto error_argv_rd;

            uint32_t hto_taille_str=htobe32(taille_str);

            //On va alors lire le bon nombre d'argument correspondant au string qu'on stock dans str
            char * str=malloc(taille_str);
            assert(str==NULL);
            rd=read(fd_argv,str,sizeof(char)*taille_str);//READ char *
            if(rd==-1)free(str);goto error_argv_rd;
            //On realloc buf_cmd à sa taille actuel+la taille de notre nouvelle argument
            buf_cmd = realloc(buf_cmd, taille_buff+sizeof(uint32_t)+taille_str);
            assert(buf_cmd==NULL);
            //On copie sa taille puis notre argument dans notre buffer comme un string
            //On update aussi les taille du buffer pour les memcpy future
            memcpy(buf_cmd+taille_buff, &hto_taille_str, sizeof(uint32_t));
            assert(buf_cmd==NULL);
            taille_buff+=sizeof(uint32_t);
            memcpy(buf_cmd+taille_buff, str, taille_str);
            assert(buf_cmd==NULL);
            taille_buff+=taille_str;

            //free du malloc contenant l'argument
            free(str);
          }

        //On va alors ajouter notre buffer de task à notre buff_inter
        size_t TAILLE_BUFF_INTER_FINAL=sizeof(uint64_t)+TAILLE_TIMING+taille_buff;
        buf_inter = realloc(buf_inter, TAILLE_BUFF_INTER_FINAL);//On agrandit buf_inter pour qu'il contiennent notre buffer de commandeline en plus du taskID et du timing
        assert(buf_inter==NULL);
        memcpy(buf_inter+sizeof(uint64_t)+TAILLE_TIMING, buf_cmd,taille_buff);//On copie le buffer de commande dans buff inter
        assert(buf_inter==NULL);
        //While pour agrandir la taille de façon constante tant que la taille de buff_all_tasks ne permet pas la copie de buff_inter en elle
        while(taille_globale<taille_utilisee+ TAILLE_BUFF_INTER_FINAL){
            buf_all_tasks = realloc(buf_all_tasks, taille_globale+1024);//On realloc en ajoutant 1024
            assert(buf_all_tasks==NULL);
            taille_globale+=1024;//On update la taille global
        }
        //Lorsque la taille le permet on ajoute le buff_inter
        memcpy(buf_all_tasks + taille_utilisee, buf_inter,TAILLE_BUFF_INTER_FINAL);
        assert(buf_all_tasks==NULL);
        taille_utilisee+=TAILLE_BUFF_INTER_FINAL;//et on update la taille utilisé de notre buffer_all_tasks
        //FREE

        free(buf_cmd);
        error_argv_rd:
            close(fd_argv);
        error_argv_op:
           free(pathtasks_argv);
        error_time_read:
            close(fd_time_exec);
        error_time_op :
            free(pathtasks_exec);
            free(buf_inter);
            free(path_to_task);
        }
    }
    nb_tasks=htobe32(nb_tasks);
    uint16_t ok = htobe16(SERVER_REPLY_OK);
    //On écrit OK et notre nombre de task dans le buf_total
    char *buf_total = malloc(sizeof(uint16_t)+ sizeof(uint32_t) + taille_utilisee);
    memcpy(buf_total,&ok,sizeof(uint16_t));
    memcpy(buf_total+sizeof(uint16_t),&nb_tasks,sizeof(uint32_t));
    //Puis on écrit notre buffer_all_tasks dans notre buf_total
    memcpy(buf_total+sizeof(uint16_t)+sizeof(uint32_t),buf_all_tasks,taille_utilisee);
    //On écrit la totalité de buf_total le pipe fd_reply
    write(fd_reply, buf_total,sizeof(uint16_t)+ sizeof(uint32_t) + taille_utilisee);

    free(buf_all_tasks);
    free(buf_total);
    close(dir);
    exit(EXIT_SUCCESS);
}
