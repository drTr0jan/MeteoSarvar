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

#define ESOCK		1	/* socket general error */
#define EDNSG		2	/* dns general error */
#define	EDNSO		3	/* dns other error */
#define ECONG		4	/* connect general error */
#define ECONO		5	/* connect other error */

typedef struct __attribute__((__packed__)) {
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
} SRVPKT;

int serv_connect(const char *hostname, uint16_t port)
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
  int error, sock;

  if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0)
  {
// !!! Global error
//    printf("socket() failed: %d", errno);
    return -1;
  }

  sprintf(servname,"%u",port);
  if (getaddrinfo(hostname,servname,&hint,&aip) != 0)
  {
    close(sock);
    return -1;
  }
/*  if ((error == EAI_AGAIN) || (error == EAI_NONAME))
  {
    return EDNSG;
  }
  if (error != 0)
    return EDNSO;
  printf("namelen: %d\n",aip->ai_addrlen);*/
  if (connect(sock, aip->ai_addr, aip->ai_addrlen) < 0)
  {
    close(sock);
    return -1;
/*    if ((errno == ETIMEDOUT) ||
        (errno == ECONNREFUSED) ||
        (errno == ECONNRESET) || 
// for freebsd
	(errno == EINVAL) ||
//        
        (errno == ENETUNREACH) || 
        (errno == EHOSTUNREACH))
      return ECONG;
    else
      return ECONO;*/
  }
  return sock;
}

int main(int argc, char *argv[])
{
  iconv_t cd;

  const unsigned short port = 7251;
  const char * hostname = "95.167.117.38";

  srandom(time(NULL));
  cd = iconv_open("CP1251","KOI8-R");

  // Main loop
  while (1)
  {
    int sock, readbytes;
    SRVPKT *spbuf;


    // Connect statement
    if ((sock = serv_connect(hostname,port)) < 0)
    {
      printf("connect() failed: %d\n", errno);
      sleep(5);
      continue;
    }

    // Read section
    spbuf = (SRVPKT *) malloc(sizeof(*spbuf));    
    while ((readbytes = read(sock, spbuf, sizeof(*spbuf))) > 0)
    {

      // If packet is TCP_IP_DATA
      if (spbuf->typy == TCP_IP_DATA)
      {
        int len;
        char filename[13];
        FILE *fp;
        char * msg;

        len = ntohs(spbuf->len);
        printf("Length = %d\n",len);
        
        // Receive the data
        msg = (char *) malloc(len);
        recv(sock, msg, len, MSG_WAITALL);
//printf("Rec size: %d\n",ret);
//      int ret = read(sock,tmpbuf,12);
//      if (ret == -1)
//        printf("An error %d occured.\n",errno);

        // Convert the data
        convert_msg8(cd,&msg,len);
//        printf("Converting success\n");
        
        // Save the data
        get_filename(filename);
        fp = fopen(filename,"w");
        fwrite(msg,len,1,fp);
        fclose(fp);
        free(msg);
        printf("Saving success: %s\n", filename);
        
        // Read end of packet
        read(sock, spbuf, sizeof(*spbuf));
        // If correct
        if (spbuf->typy == TCP_IP_END)
        {
          // Sending an acknowledgement
          spbuf->typy = TCP_IP_ACK;
          if ((write(sock, spbuf, sizeof(*spbuf))) > 0)
            printf("ACK has been sent successful.\n");
          else
            printf("An send error has been occured. Error code is: %d\n",errno);
        }
        else
          printf("A capturing error has been occured. The packet type is %d, but must be TCP_IP_END.\n",spbuf->typy);
      }
      // Print a packet type if not
      else
        printf("The packet type is %d.\n",spbuf->typy);
/*    printf("Length = %d\n",ntohs(spbuf->len));
    printf("Number = %d\n",ntohs(spbuf->num));
    printf("Header = %s\n",spbuf->ahd);
    printf("Priority = %d\n",spbuf->pri);*/
    }
    free(spbuf);

    // If receiving problem
    if (readbytes == -1)
      printf("An receive error has been occured. Error code is: %d\n",errno);
    if (readbytes == 0)
      printf("An EOF has been gotten.\n");
    close(sock);
    sleep(5);
  }    

  iconv_close(cd);
/*  if (argc < 5)
    printf("mts_comm: invalid arguments\n");
  else*/
//    printf("Test\n");
  return 0;
}
