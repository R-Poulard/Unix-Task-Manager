#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "timing.h"
#include "../src/timing-text-io.c"
#include <endian.h>
#include <time.h>
#include <dirent.h>
#include <errno.h>
#include <sys/wait.h>
#include "server-reply.h"
#include "assert.h"
#include "client-request.h"
#include <inttypes.h>

#define CMD_ARGV "argv"
#define CMD_TEXEC "time_exec"
#define CMD_EXIT "exit"
#define CMD_STDOUT "stdout"
#define CMD_STDERR "stderr"

#define PATH_SIZE 256
#define BUFFER_BLOCK 512

int cpy(char * buf1,char *buf2,size_t size);
int parsec (char global [],char* subside,char del);
int compaire_cron(int NOSMINUTES,char* minutes);
char * path_constr(char * final_path, char *old_path, char *file);
int write_in_block(int fd_reply,char* buffer,size_t buffersize);
int not_found(int fd_reply);
int execute_a_task(char *path_to_task,DIR* dir);
int write_exit_code(char *path_to_task,pid_t pid2,time_t time2);
