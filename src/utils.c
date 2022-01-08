#include "utils.h"

int cpy(char * buf1,char *buf2,size_t size){
  strncpy(buf2,buf1,size);
  buf2[size]='\0';
  return 1;
}

int parsec (char global [],char* subside,char del){
  int last_one=-1;
  int i=0;
  while(global[i]!='\0'){
    if(global[i]==del){
      last_one=i;
    }
    i++;    
  }
  if(last_one==-1){
    return 0;
  }
  else{
    //subside=malloc(sizeof(char)*strlen(global+last_one+1));
    strncpy(subside,global+last_one+1,strlen(global+last_one));
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
        free(current);
        return 1;
      }
    }
    else{
      int petit =atoi(current);
      int grand=atoi(siunder);
      
      if(petit<=NOSMINUTES && NOSMINUTES <= grand){
        free(current);
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

char* path_constr(char * final_path, char *old_path, char *file){
    final_path = malloc(100);
    strcpy(final_path, old_path);
    strcat(final_path,"/");
    strcat(final_path, file);
    return final_path;
}


