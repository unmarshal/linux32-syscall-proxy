#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

int
main(int argc, char **argv)
{
   int sock_fd, new_fd, size;
   struct sockaddr_in sin;

   size = sizeof(sin);

   if((sock_fd = bind_socket(atoi(argv[1]))) < 0)
      exit(-1);

   new_fd = accept(sock_fd, (struct sockaddr *)&sin, &size);
   
   if(new_fd < 0) {
      printf("accept error\n");
      exit(-1);
   }

   do_proxy(new_fd);
}

int
do_proxy(int fd)
{
   int ret;

   asm("accept_request:"
       "movl %1, %%ebx;"
       "push %%ebx;"
       "push %%esp;"
       
       "send_esp:"
       "mov $4, %%eax;"
       "movl %%esp, %%ecx;"
       "mov $4, %%edx;"
       "int $0x80;"
       
       "read_request:"
       "movl %%ebp, %%esp;"
       "xorl %%eax, %%eax;"
       "movl $4, %%edx;"
       
       "read_request2:"
       "mov  $3, %%al;"
       "mov  %%esp, %%ecx;"
       "int  $0x80;"
       
       "add  %%eax, %%ecx;"
       "sub  %%eax, %%edx;"
       "jnz  read_request2;"
       "pop  %%edx;"
       "sub  %%edx, %%esp;"

       "read_request3:"
       "movl $3, %%eax;"
       "mov  %%esp, %%ecx;"
       "int  $0x80;"
       "add  %%eax, %%ecx;"
       "sub  %%eax, %%edx;"
//       "jnz  read_request3;"

       "do_request:"
       "pop  %%eax;"
       "pop  %%ebx;"
       "pop  %%ecx;"
       "pop  %%edx;"
       "pop  %%esi;"
       "pop  %%edi;"
       "int  $0x80;"
       "push %%edi;"
       "push %%esi;"
       "push %%edx;"
       "push %%ecx;"
       "push %%ebx;"
       "push %%eax;"

       "do_send_answer:"
       "mov  $4, %%eax;"
       "mov  0x8(%%ebp), %%ebx;"
       "mov  %%esp, %%ecx;"
       "mov  %%ebp, %%edx;"
       "sub  %%esp, %%edx;"
       "int  $0x80;"

       "jmp  read_request;"

       "end:"
       "pop  %%eax;"
       "pop  %%ebp;"
       
     :"=r"(ret):"r"(fd));
}

int bind_socket(int port)
{
  int i = 1;
  int sockfd;        /* bound socket descriptor   */
  struct sockaddr_in sin;  /* socket information struct */

  /*
   * initialize socket and bind to specified port
   */

  if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket()");
    return(-1);
  }

  sin.sin_port = htons(port);
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = INADDR_ANY;

  if(setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,(char *)&i,sizeof(i)) < 0) {
     perror("setsockopt()");
     return(-1);
  }

  if(bind(sockfd, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
    perror("bind()");
    return(-1);
  }

  if(listen(sockfd, 100) < 0) {
    perror("listen()");
    return(-1);
  }
  return(sockfd);
}
