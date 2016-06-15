#include <stdio.h>
#include <stdlib.h>
#include <iconv.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h> 
#include <netinet/in.h>
#include <unistd.h>
#include "convert.h"
#include "filename.h"

#define	TCP_IP_DATA	1
#define	TCP_IP_DATA_Z	5
#define	TCP_IP_END	4
#define	TCP_IP_ACK	2
#define	TCP_IP_RR	6

struct __attribute__((__packed__)) srvpkt {
  uint8_t typy;
  uint32_t ijp;
  uint8_t format;
  uint8_t jinfo_ijp;
  uint8_t text_offs;
  uint16_t len;
  uint16_t num;
  char ahd[12];
  uint8_t pri;
  uint8_t pad[3];
};

int main(int argc, char *argv[])
{
  iconv_t cd;
  int sock, len, i;
  struct hostent *server;
  struct sockaddr_in serv_addr;
  const struct addrinfo hint = {
    .ai_flags = AI_NUMERICHOST,
    .ai_family = AF_INET,
    .ai_socktype = 0,
    .ai_protocol = 0,
    0,NULL, NULL, NULL
  };
  struct addrinfo *aip;
  struct srvpkt *spbuf;
  char * msg;
  FILE *fp;

  const char * port = "7251";
  const char * hostname = "95.167.117.38";

  srandom(time(NULL));
  cd = iconv_open("CP1251","KOI8-R");
  
  sock = socket(PF_INET, SOCK_STREAM, 0);
  if (sock < 0)
  {
    printf("socket() failed: %d", errno);
    return EXIT_FAILURE;
  }
  
/*  server = gethostbyname(hostname);
  if (server == NULL) 
  {
    printf("Host not found\n");
    return EXIT_FAILURE;
  }
*/
  if (getaddrinfo(hostname,port,&hint,&aip) != 0)
  {
    printf("Host not found\n");
    return EXIT_FAILURE;
  }

  if (connect(sock, aip->ai_addr, aip->ai_addrlen) < 0) 
  {
    printf("connect() failed: %d", errno);
    return EXIT_FAILURE;
  }  

  if (read(sock, spbuf, sizeof(*spbuf))>0)
  {
    if (spbuf->typy == TCP_IP_DATA)
    {
      len = ntohs(spbuf->len);
      printf("Length = %d\n",len);      
      msg = (char *) malloc(len);
      recv(sock, msg, len, MSG_WAITALL);
//printf("Rec size: %d\n",ret);
//      int ret = read(sock,tmpbuf,12);
//      if (ret == -1)
//        printf("An error %d occured.\n",errno);
//      
      convert_msg8(cd,&msg,len);
      fp = fopen(get_filename(),"w");
      fwrite(msg,len,1,fp);
      fclose(fp);
      free(msg);
      read(sock, spbuf, sizeof(*spbuf));
      if (spbuf->typy == TCP_IP_END)
        printf("A packet has been captured successful.\n");
      else
        printf("A capturing error has been occured.\n");
    }
    else
      printf("The packet type is %d.\n",spbuf->typy);
/*    printf("Length = %d\n",ntohs(spbuf->len));
    printf("Number = %d\n",ntohs(spbuf->num));
    printf("Header = %s\n",spbuf->ahd);
    printf("Priority = %d\n",spbuf->pri);*/
  }
  else
    printf("An receive error has been occured.\n");
//  close(sock);
    iconv_close(cd);
/*  if (argc < 5)
    printf("mts_comm: invalid arguments\n");
  else*/
    printf("Test\n");
  return 0;
}
