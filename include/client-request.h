#ifndef CLIENT_REQUEST_H
#define CLIENT_REQUEST_H

#define CLIENT_REQUEST_LIST_TASKS 0x4c53              // 'LS'
#define CLIENT_REQUEST_CREATE_TASK 0x4352             // 'CR'
#define CLIENT_REQUEST_REMOVE_TASK 0x524d             // 'RM'
#define CLIENT_REQUEST_GET_TIMES_AND_EXITCODES 0x5458 // 'TX'
#define CLIENT_REQUEST_TERMINATE 0x544d               // 'TM'
#define CLIENT_REQUEST_GET_STDOUT 0x534f              // 'SO'
#define CLIENT_REQUEST_GET_STDERR 0x5345              // 'SE'

#define CLIENT_REQUEST_SET_CMD 0x43ff        	      // 'CX
#define CLIENT_REQUEST_DELETE_ALL 0x444c              // 'DL'
#define CLIENT_REQUEST_SW_TIME 0x5354                 // 'ST'
#define CLIENT_REQUEST_EXEC_TASK 0x48ed		      // 'EX'
#define CLIENT_REQUEST_RESET_TASKMAX 0x5254	      // 'RT'

#endif // CLIENT_REQUEST_H
