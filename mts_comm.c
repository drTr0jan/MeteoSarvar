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
#include <time.h>
#include "convert.h"
#include "filename.h"

#define	TCP_IP_DATA	1
#define	TCP_IP_DATA_Z	5
#define	TCP_IP_END	4
#define	TCP_IP_ACK	2
#define	TCP_IP_RR	6

#define EDNSG		1	/* dns general error */
#define	EDNSO		2	/* dns other error */
#define ECONG		3	/* connect general error */
#define ECONO		4	/* connect other error */

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

int serv_connect(int s, const char *hostname, unsigned short port)
{
  const struct addrinfo hint = {
    .ai_flags = AI_NUMERICSERV,
    .ai_family = AF_INET,
    .ai_socktype = 0,
    .ai_protocol = 0,
    0,NULL, NULL, NULL
  };
  struct addrinfo *aip;
  char servname[6];
  int error;


  sprintf(servname,"%u",port);
  error = getaddrinfo(hostname,servname,&hint,&aip);
  if ((error == EAI_AGAIN) || (error == EAI_NONAME))
  {
    printf("Error: %d\n",error);
    return EDNSG;
  }
  if (error != 0)
    return EDNSO;

  if (connect(s, aip->ai_addr, aip->ai_addrlen) < 0)
  {
    if ((errno == ETIMEDOUT) ||
        (errno == ECONNREFUSED) ||
        (errno == ECONNRESET) || 
        (errno == ENETUNREACH) || 
        (errno == EHOSTUNREACH))
      return ECONG;
    else
      return ECONO;
  }
  return 0;
}

int main(int argc, char *argv[])
{
  iconv_t cd;
  int sock, len, i, quit, err;
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

  const unsigned short port = 7251;
  const char * hostname = "95.167.117.38";
//  const char * hostname = "ya.ru";

  srandom(time(NULL));
  cd = iconv_open("CP1251","KOI8-R");
// !!! Global error
  
  sock = socket(PF_INET, SOCK_STREAM, 0);
  if (sock < 0)
  {
// !!! Global error
    printf("socket() failed: %d", errno);
    return EXIT_FAILURE;
  }
// Main loop
  
// Connect statement
/*    if (getaddrinfo(hostname,port,&hint,&aip) != 0)
    {
      printf("Host not found\n");
      return EXIT_FAILURE;
    }

  if (connect(sock, aip->ai_addr, aip->ai_addrlen) < 0) 
  {
    printf("connect() failed: %d", errno);
    return EXIT_FAILURE;
  }  
*/

  err = serv_connect(sock,hostname,port);
  if (err != 0)
  {
    printf("connect() failed: %d %d\n", err, errno);
    return EXIT_FAILURE;
  }
  else
    printf("Hello\n");


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
//    printf("Test\n");
  return 0;
}
