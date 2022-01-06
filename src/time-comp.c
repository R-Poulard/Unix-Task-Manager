
int parsec (char global [],char* subside,char del){
  int last_one=-1;
  int i=0;
  while(global[i]!='\0'){
	    printf("PARSEC 1\n");

	  if(global[i]==del){
      last_one=i;
    }
    i++;    
  }
  if(last_one==-1){
    return 0;
  }
  else{
  printf("PARSEC 2\n");

    //subside=malloc(sizeof(char)*strlen(global+last_one+1));
    printf("LAST One : %d \n", last_one);
    printf("SUBSIDE One : %d \n", strlen(subside));
    printf("SUBSIDE One : %s \n", subside);
   
    printf("STRELEN  : %d \n", strlen(global+last_one));
    strncpy(subside,global+last_one+1,strlen(global+last_one));
     printf("SUBSIDE APRES CPY : %s \n", subside);
  printf("PARSEC 3\n");
    global[last_one]='\0';
    return 1;
  }
  
}

int compaire_cron(int NOSMINUTES,char* minutes){
	//printf("minutes : %s \n", minutes);
	//printf("NOS MIN : %d \n", NOSMINUTES);
  char *current=malloc(500);
  if(strchr(minutes,'*')!=NULL)return 1;
  while(parsec(minutes,current,',')!=0){
	    
    char* siunder = malloc(256);
    int y = parsec(current,siunder,'-');

    if(y==0){

      if(atoi(current)==NOSMINUTES){
        printf("EQUALS\n");
        free(current);
        return 1;
      }
    }
    else{
      int petit =atoi(current);
      int grand=atoi(siunder);
      
      if(petit<=NOSMINUTES && NOSMINUTES <= grand){
        free(current);
        printf("Tirets TRUE\n");
        return 1;
      }
    }
  }
  char* siunder=malloc(500);
      
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
  
  return 0;
}
