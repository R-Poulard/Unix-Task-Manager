#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "timing.h"
#include "timing-text-io.c"
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

int cpy(char * buf1,char *buf2,size_t size);
int parsec (char global [],char* subside,char del);
int compaire_cron(int NOSMINUTES,char* minutes);
char * path_constr(char * final_path, char *old_path, char *file);
