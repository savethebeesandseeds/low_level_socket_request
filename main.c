#define _GNU_SOURCE
#ifdef __cplusplus
#define _XOPEN_SOURCE_EXTENDED 1
#endif
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>

#define BUFFER_SIZE 10240
#define REQUEST_SIZE 10240

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

#define LOG_FILE stdout
#define LOG_ERR_FILE stderr
#define LOG_WARN_FILE sdtout

#define ANSI_COLOR_ERROR "\x1b[41m" 
#define ANSI_COLOR_SUCCESS "\x1b[42m" 
#define ANSI_COLOR_WARNING "\x1b[43m" 
#define ANSI_COLOR_RESET "\x1b[0m" 
#define ANSI_COLOR_Black "\x1b[30m" 
#define ANSI_COLOR_Red "\x1b[31m"     
#define ANSI_COLOR_Green "\x1b[32m"   
#define ANSI_COLOR_Yellow "\x1b[33m" 
#define ANSI_COLOR_Blue "\x1b[34m"  
#define ANSI_COLOR_Magenta "\x1b[35m" 
#define ANSI_COLOR_Cyan "\x1b[36m"    
#define ANSI_COLOR_White "\x1b[37m" 
#define ANSI_COLOR_Bright_Black_Grey "\x1b[90m" 
#define ANSI_COLOR_Bright_Red "\x1b[91m" 
#define ANSI_COLOR_Bright_Green "\x1b[92m" 
#define ANSI_COLOR_Bright_Yellow "\x1b[93m"     
#define ANSI_COLOR_Bright_Blue "\x1b[94m" 
#define ANSI_COLOR_Bright_Magenta "\x1b[95m" 
#define ANSI_COLOR_Bright_Cyan "\x1b[96m"       
#define ANSI_COLOR_Bright_White "\x1b[97m"

pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
/* This log functionality checks if there is a pendding log for the error trasported by the errno.h lib, 
 * WARNING! This functionaly sets the errno=0 if the errno is futher required, this might be not a desired behaviour.
 */
#define wrap_log_sys_err() {\
  if(errno != 0) {\
    pthread_mutex_lock(&log_mutex);\
    fprintf(LOG_ERR_FILE,"[%s0x%lX%s]: %sSYS ERRNO%s: ",\
      ANSI_COLOR_Cyan,pthread_self(),ANSI_COLOR_RESET,\
      ANSI_COLOR_ERROR,ANSI_COLOR_RESET);\
    fprintf(LOG_ERR_FILE,"%s\n", strerror(errno));\
    fflush(LOG_ERR_FILE);\
    pthread_mutex_unlock(&log_mutex);\
    errno = 0;\
  }\
}
/* This log functionality marks the thread id, so that log messages are linked to the request thread */
#define log(...) {\
  wrap_log_sys_err();\
  pthread_mutex_lock(&log_mutex);\
  fprintf(LOG_FILE,"[%s0x%lX%s]: ",\
    ANSI_COLOR_Cyan,pthread_self(),ANSI_COLOR_RESET);\
  fprintf(LOG_FILE,__VA_ARGS__);\
  fflush(LOG_FILE);\
  pthread_mutex_unlock(&log_mutex);\
}
/* This log functionality marks the thread id, so that log messages are linked to the request thread */
#define log_err(...) {\
  wrap_log_sys_err();\
  pthread_mutex_lock(&log_mutex);\
  fprintf(LOG_ERR_FILE,"[%s0x%lX%s]: %sERROR%s: ",\
    ANSI_COLOR_Cyan,pthread_self(),ANSI_COLOR_RESET,\
    ANSI_COLOR_ERROR,ANSI_COLOR_RESET);\
  fprintf(LOG_ERR_FILE,__VA_ARGS__);\
  fflush(LOG_ERR_FILE);\
  pthread_mutex_unlock(&log_mutex);\
}
/* This log functionality marks the thread id, so that log messages are linked to the request thread */
#define log_warn(...) {\
  wrap_log_sys_err();\
  pthread_mutex_lock(&log_mutex);\
  fprintf(LOG_WARN_FILE,"[%s0x%lX%s]: %sWARNING%s: ",\
    ANSI_COLOR_Cyan,pthread_self(),ANSI_COLOR_RESET,\
    ANSI_COLOR_WARNING,ANSI_COLOR_RESET);\
  fprintf(LOG_WARN_FILE,__VA_ARGS__);\
  fflush(LOG_WARN_FILE);\
  pthread_mutex_unlock(&log_mutex);\
}
/* this function prints boolean values */
#define print_bool(dinput) dinput? "True":"False"

int main(int argc, char **argv){
  
  log("starting socket piaabo...\n");
  /* 
   * DEFINE VARIABLES
   */
  struct addrinfo hints;
  struct addrinfo *dres;
  struct addrinfo *temp_dres;
  int sockfd;
  int addr_ret;
  int gname_ret;
  int connect_ret;
  ssize_t send_ret;

  char dhostname[NI_MAXHOST]="";
  char buff[BUFFER_SIZE];
  ssize_t byte_count;

  const char* method = "GET";
  const char* hostport = "80";
  const char* hostname = "www.example.com";

  char request_str[REQUEST_SIZE];
  sprintf(request_str, "%s / %s\r\nHost: %s\r\n\r\n", method, SERVNAME_STR, hostname);

  log("REQUEST:\n%s\n",request_str);

  /* 
   * INITIALIZE VARIABLES
   */
  log("intializing variables...\n");
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = ADDR_FAMILY;
  hints.ai_socktype = SOCK_STREAM;
  /*
   * START 
  */
  wrap_log_sys_err();
  /* 
   * GET ADDRESS INFO 
   */
  log("getting addrinfo...\n");
  addr_ret = getaddrinfo(hostname,hostport,&hints, &dres);
  wrap_log_sys_err();
  if(addr_ret!=0){
    log_err("Address info : [%s]\n", gai_strerror(addr_ret));
    exit(1);
  }
  /*
   * PRINT HOSTNAME
   */
  for(temp_dres=dres; temp_dres!=NULL; temp_dres=dres->ai_next){
    gname_ret = getnameinfo(temp_dres->ai_addr, temp_dres->ai_addrlen, dhostname, NI_MAXHOST, NULL, 0, 0);
    if(gname_ret!=0){
      log("Name info : [%s]\n", gai_strerror(gname_ret));
      continue;
    }
    if(*dhostname!='\0')
      log("hostname: %s\n",dhostname);
  }
  /* 
   * MAKE SOCKET 
   */
  log("making socket...\n");
  sockfd = socket(dres->ai_family, dres->ai_socktype, dres->ai_protocol);
  wrap_log_sys_err();
  if(sockfd==-1){
    log_err("unnable to make socket.\n");
    exit(1);
  }
  /* 
   * CONNECT TO SOCKET 
   */
  log("conecting to socket!...\n");
  connect_ret = connect(sockfd, dres->ai_addr, dres->ai_addrlen);
  wrap_log_sys_err();
  if(connect_ret!=0){
    log_err("unnable to connect socket.\n");
    exit(1);
  }
  log("connected!...\n");
  /* 
   * MAKE RESTfull request 
   */
  log("making RESTfull: %s / %s %s\n", method, hostname, SERVNAME_STR);
  send_ret = send(sockfd, request_str, strlen(request_str),0);
  wrap_log_sys_err();
  if(send_ret==-1){
    log_err("unnable to send request.\n");
    exit(1);
  }
  /* 
   * RECEIVE RESTfull response 
   */
  log("receiving response...\n");
  byte_count = recv(sockfd, buff, sizeof(buff)-1, 0);
  wrap_log_sys_err();
  if(byte_count==-1){
    log_err("unnable to recv request response.\n");
    exit(1);
  }
  buff[byte_count] = 0; // string null terminated

  /*
   * LOG RESPONSE 
  */
  log( "RESPONSE, size[%ld]:\n%s\n", byte_count, buff);

  /* 
   * FINALIZE 
   */
  log( "Finalizing...\n");
  freeaddrinfo(dres);
  
  return EXIT_SUCCESS;
}
