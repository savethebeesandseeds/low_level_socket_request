#define _GNU_SOURCE
#ifdef __cplusplus
#define _XOPEN_SOURCE_EXTENDED 1
#endif
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>

#define BUFFER_SIZE 10240
#define REQUEST_SIZE 10240
#define LOG_FILE stdout
#define LOG_ERR_FILE stderr

// #define IPv6_FAMILY_ // Define to make ipv6 requests
#define SERVNAME_STR "HTTP/1.1"

#ifdef IPv6_FAMILY_
#define ADDR_FAMILY AF_INET6
#define ADDR_LENGTH INET6_ADDRSTRLEN
#else 
#define ADDR_FAMILY AF_INET
#define ADDR_LENGTH INET_ADDRSTRLEN
#endif

#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>

#define ANSI_COLOR_ERROR "\x1b[41m"
#define ANSI_COLOR_SUCCESS "\x1b[42m"
#define ANSI_COLOR_WARNING "\x1b[43m"
#define ANSI_COLOR_RESET "\x1b[0m"
#define	ANSI_COLOR_Black "\x1b[30m"
#define	ANSI_COLOR_Red "\x1b[31m"
#define	ANSI_COLOR_Green "\x1b[32m"
#define	ANSI_COLOR_Yellow "\x1b[33m"
#define	ANSI_COLOR_Blue "\x1b[34m"
#define	ANSI_COLOR_Magenta "\x1b[35m"
#define	ANSI_COLOR_Cyan "\x1b[36m"
#define	ANSI_COLOR_White "\x1b[37m"
#define	ANSI_COLOR_Bright_Black_Grey "\x1b[90m"
#define	ANSI_COLOR_Bright_Red "\x1b[91m"
#define	ANSI_COLOR_Bright_Green "\x1b[92m"
#define	ANSI_COLOR_Bright_Yellow "\x1b[93m"
#define	ANSI_COLOR_Bright_Blue "\x1b[94m"
#define	ANSI_COLOR_Bright_Magenta "\x1b[95m"
#define	ANSI_COLOR_Bright_Cyan "\x1b[96m"
#define	ANSI_COLOR_Bright_White "\x1b[97m"

pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
/* This log functionality checks if there is a pendding log for the error trasported by the errno.h lib, 
 * WARNING! This functionaly sets the errno=0 if the errno is futher required, this might be not a desired behaviour.
 */
#define wrap_log_sys_err(log_file) {\
  if(errno != 0) {\
    pthread_mutex_lock(&log_mutex);\
    fprintf(log_file,"[%s0x%lX%s]: %sSYS ERRNO%s: ",\
      ANSI_COLOR_Cyan,pthread_self(),ANSI_COLOR_RESET,\
      ANSI_COLOR_ERROR,ANSI_COLOR_RESET);\
    fprintf(log_file,"%s\n", strerror(errno));\
    fflush(log_file);\
    pthread_mutex_unlock(&log_mutex);\
    errno = 0;\
  }\
}
/* This log functionality marks the thread id, so that log messages are linked to the request thread */
#define log(log_file,...) {\
  wrap_log_sys_err(log_file);\
  pthread_mutex_lock(&log_mutex);\
  fprintf(log_file,"[%s0x%lX%s]: ",\
    ANSI_COLOR_Cyan,pthread_self(),ANSI_COLOR_RESET);\
  fprintf(log_file,__VA_ARGS__);\
  fflush(log_file);\
  pthread_mutex_unlock(&log_mutex);\
}
/* This log functionality marks the thread id, so that log messages are linked to the request thread */
#define log_err(log_file,...) {\
  wrap_log_sys_err(log_file);\
  pthread_mutex_lock(&log_mutex);\
  fprintf(log_file,"[%s0x%lX%s]: %sERROR%s: ",\
    ANSI_COLOR_Cyan,pthread_self(),ANSI_COLOR_RESET,\
    ANSI_COLOR_ERROR,ANSI_COLOR_RESET);\
  fprintf(log_file,__VA_ARGS__);\
  fflush(log_file);\
  pthread_mutex_unlock(&log_mutex);\
}
/* This log functionality marks the thread id, so that log messages are linked to the request thread */
#define log_warn(log_file,...) {\
  wrap_log_sys_err(log_file);\
  pthread_mutex_lock(&log_mutex);\
  fprintf(log_file,"[%s0x%lX%s]: %sWARNING%s: ",\
    ANSI_COLOR_Cyan,pthread_self(),ANSI_COLOR_RESET,\
    ANSI_COLOR_WARNING,ANSI_COLOR_RESET);\
  fprintf(log_file,__VA_ARGS__);\
  fflush(log_file);\
  pthread_mutex_unlock(&log_mutex);\
}
/* this function prints boolean values */
#define print_bool(dinput) dinput? "True":"False"

void explain_addrinfo_error(int addr_ret, void *buf, const char *portaddr);

int main(int argc, char **argv){
  
  log(LOG_FILE,"starting socket piaabo...\n");
  /* 
   * DEFINE VARIABLES
   */
  struct addrinfo hints;
  struct addrinfo *dres;
  int sockfd;
  int addr_ret;
  int connect_ret;
  ssize_t send_ret;

  char buff[BUFFER_SIZE];
  ssize_t byte_count;

  const char* method = "GET";
  const char* hostport = "80";
  const char* hostname = "www.example.com";

  char request_str[REQUEST_SIZE];
  sprintf(request_str, "%s / %s\r\nHost: %s\r\n\r\n", method, SERVNAME_STR, hostname);

  fprintf(stdout,"%s\n",request_str);

  /* 
   * INITIALIZE VARIABLES
   */
  log(LOG_FILE,"intializing variables...\n");
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = ADDR_FAMILY;
  hints.ai_socktype = SOCK_STREAM;

  /* 
   * GET ADDRESS INFO 
   */
  log(LOG_FILE,"getting addrinfo...\n");
  addr_ret = getaddrinfo(hostname,hostport,&hints, &dres);
  wrap_log_sys_err(LOG_ERR_FILE);
  if(addr_ret!=0){
    explain_addrinfo_error(addr_ret, (void*)hostname, hostport);
    exit(1);
  }
  /* 
   * MAKE SOCKET 
   */
  log(LOG_FILE,"making socket...\n");
  sockfd = socket(dres->ai_family, dres->ai_socktype, dres->ai_protocol);
  wrap_log_sys_err(LOG_ERR_FILE);
  if(sockfd==-1){
    log_err(LOG_ERR_FILE,"unnable to make socket.\n");
    exit(1);
  }
  /* 
   * CONNECT TO SOCKET 
   */
  log(LOG_FILE,"conecting to socket!...\n");
  connect_ret = connect(sockfd, dres->ai_addr, dres->ai_addrlen);
  wrap_log_sys_err(LOG_ERR_FILE);
  if(connect_ret!=0){
    log_err(LOG_ERR_FILE,"unnable to connect socket.\n");
    exit(1);
  }
  log(LOG_FILE,"connected!...\n");
  /* 
   * MAKE RESTfull request 
   */
  log(LOG_FILE,"making RESTfull: %s / %s %s\n", method, hostname, SERVNAME_STR);
  send_ret = send(sockfd, request_str, strlen(request_str),0);
  wrap_log_sys_err(LOG_ERR_FILE);
  if(send_ret==-1){
    log_err(LOG_ERR_FILE,"unnable to send request.\n");
    exit(1);
  }
  /* 
   * RECEIVE RESTfull response 
   */
  log(LOG_FILE,"receiving response...\n");
  byte_count = recv(sockfd, buff, sizeof(buff)-1, 0);
  wrap_log_sys_err(LOG_ERR_FILE);
  if(byte_count==-1){
    log_err(LOG_ERR_FILE,"unnable to recv request response.\n");
    exit(1);
  }
  buff[byte_count] = 0; // string null terminated

  /*
   * LOG RESPONSE 
  */
  log(LOG_FILE, "RESPONSE: size[%ld], %s\n", byte_count, buff);

  /* 
   * FINALIZE 
   */
  log(LOG_FILE, "Finalizing...\n");
}


void explain_addrinfo_error(int addr_ret, void *buf, const char *portaddr){
  switch (addr_ret)
  {
  case EAI_BADFLAGS:	/* Invalid value for `ai_flags' field.  */
  log_err(LOG_ERR_FILE,"Address info : [Invalid value for 'ai_flags' field.] %s %s\n",(char *)buf, portaddr);break;
  case EAI_NONAME:	/* NAME or SERVICE is unknown.  */
  log_err(LOG_ERR_FILE,"Address info : [NAME or SERVICE is unknown.] %s %s\n",(char *)buf, portaddr);break;
  case EAI_AGAIN:	/* Temporary failure in name resolution.  */
  log_err(LOG_ERR_FILE,"Address info : [Temporary failure in name resolution.] %s %s\n",(char *)buf, portaddr);break;
  case EAI_FAIL:	/* Non-recoverable failure in name res.  */
  log_err(LOG_ERR_FILE,"Address info : [Non-recoverable failure in name res.] %s %s\n",(char *)buf, portaddr);break;
  case EAI_FAMILY:	/* 'ai_family' not supported.  */
  log_err(LOG_ERR_FILE,"Address info : ['ai_family' not supported.] %s %s\n",(char *)buf, portaddr);break;
  case EAI_SOCKTYPE:	/* 'ai_socktype' not supported.  */
  log_err(LOG_ERR_FILE,"Address info : ['ai_socktype' not supported.] %s %s\n",(char *)buf, portaddr);break;
  case EAI_SERVICE:	/* SERVICE not supported for 'ai_socktype'.  */
  log_err(LOG_ERR_FILE,"Address info : [SERVICE not supported for 'ai_socktype'.] %s %s\n",(char *)buf, portaddr);break;
  case EAI_MEMORY:	/* Memory allocation failure.  */
  log_err(LOG_ERR_FILE,"Address info : [Memory allocation failure.] %s %s\n",(char *)buf, portaddr);break;
  case EAI_SYSTEM:	/* System error returned in 'errno'.  */
  log_err(LOG_ERR_FILE,"Address info : [System error returned in 'errno'.] %s %s\n",(char *)buf, portaddr);break;
  case EAI_OVERFLOW:	/* Argument buffer overflow.  */
  log_err(LOG_ERR_FILE,"Address info : [Argument buffer overflow.] %s %s\n",(char *)buf, portaddr);break;
  case EAI_NODATA:	/* No address associated with NAME.  */
  log_err(LOG_ERR_FILE,"Address info : [No address associated with NAME.] %s %s\n",(char *)buf, portaddr);break;
  case EAI_ADDRFAMILY:	/* Address family for NAME not supported.  */
  log_err(LOG_ERR_FILE,"Address info : [Address family for NAME not supported.] %s %s\n",(char *)buf, portaddr);break;
  case EAI_INPROGRESS:	/* Processing request in progress.  */
  log_err(LOG_ERR_FILE,"Address info : [Processing request in progress.] %s %s\n",(char *)buf, portaddr);break;
  case EAI_CANCELED:	/* Request canceled.  */
  log_err(LOG_ERR_FILE,"Address info : [Request canceled.] %s %s\n",(char *)buf, portaddr);break;
  case EAI_NOTCANCELED:	/* Request not canceled.  */
  log_err(LOG_ERR_FILE,"Address info : [Request not canceled.] %s %s\n",(char *)buf, portaddr);break;
  case EAI_ALLDONE:	/* All requests done.  */
  log_err(LOG_ERR_FILE,"Address info : [All requests done.] %s %s\n",(char *)buf, portaddr);break;
  case EAI_INTR:	/* Interrupted by a signal.  */
  log_err(LOG_ERR_FILE,"Address info : [Interrupted by a signal.] %s %s\n",(char *)buf, portaddr);break;
  case EAI_IDN_ENCODE:	/* IDN encoding failed.  */
  log_err(LOG_ERR_FILE,"Address info : [IDN encoding failed.] %s %s\n",(char *)buf, portaddr);break;
  default: /* Not recognized result */
  log_err(LOG_ERR_FILE,"Address info : [Unregocnized ERROR:%d] %s %s\n", addr_ret, (char *)buf, portaddr);break;
  }
}