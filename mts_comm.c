#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <iconv.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netdb.h> 
#include <netinet/in.h>
#include <unistd.h>
#include <time.h>
#include <limits.h>
#include "convert.h"
#include "files.h"

#define WORK_DIR  "/var/db/meteo/"
#define LINK_PATH "LINK/PRM_ASPD_S/"
#define BCKP_PATH "backup/"

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

int sock;
pthread_mutex_t rr_mutex;


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
    freeaddrinfo(aip);
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
  freeaddrinfo(aip);
  return sock;
}

void *rr_loop(void *arg)
{
  const SRVPKT rrmsg = {
    .typy = TCP_IP_RR,
    0,
    0x03,0x8f,0x6a,
    0,0,
    "            ",
    .pri = 2,
    {0,0,0}
  };
  
  while (1)
  {
    sleep(60);
    pthread_mutex_lock(&rr_mutex);
    if((write(sock, &rrmsg, sizeof(rrmsg))) > 0)
      printf("RR has been sent successful.\n");
    else
      printf("RR error has been occured.\n");
    pthread_mutex_unlock(&rr_mutex);
  }
  return((void *)0);
}

int init()
{
  return 0;
}

int usage()
{
  puts("usage: mts_connect [-br] -s host [-a host] -p channel [-w path]");
  return 0;
}

int main(int argc, char *argv[])
{
  int opt;
  _Bool fl_backup = 0;
  _Bool fl_rr = 0;
  char *hostname = NULL;
  char *hostname_alt = NULL;
  unsigned short port = 0;
  char *work_dir = NULL;
  iconv_t cd;
  char *msg_path, *bak_path;

  if (argc < 2)
  {
    usage();
    return EXIT_FAILURE;
  }
  while ((opt = getopt(argc, argv, "hbrs:a:p:w:")) != -1)
  {
    switch (opt)
    {
      case 'h':
        usage();
        return EXIT_SUCCESS;
      case 'b':
        fl_backup = 1;
        break;
      case 'r':
        fl_rr = 1;
        break;
      case 's':
        hostname = (char *) malloc(strlen(optarg)+1);
        strcpy(hostname,optarg);
        break;
      case 'a':
        hostname_alt = (char *) malloc(strlen(optarg)+1);
        strcpy(hostname_alt,optarg);
        break;
      case 'p':
        port = (unsigned short) atoi(optarg);
        break;
      case 'w':
        work_dir = (char *) malloc(strlen(optarg)+1);
        strcpy(work_dir,optarg);
        break;
      case '?':
        return EXIT_FAILURE;
    }
  }

  if (hostname == NULL || port == 0)
  {
    usage();
    return EXIT_FAILURE;
  }

  if (work_dir == NULL)
  {
    work_dir = (char *) malloc(strlen(WORK_DIR)+1);
    strcpy(work_dir,WORK_DIR);
  }

/*
  printf("Argc: %d\n", argc);
  printf("Optind: %d\n", optind);
  printf("Server: %s\n",hostname);
  if (hostname_alt != NULL)
    printf("Alternate server: %s\n",hostname_alt);
  printf("Port: %d\n", port);
  printf("Workdir: %s\n", work_dir);
  return EXIT_SUCCESS;
*/

//!!!
  fl_rr = 1;
//  const unsigned short port = 7251;
//  const char * hostname = "95.167.117.38";
  
//_POSIX_PATH_MAX
//  char msg_path[] = LINK_PATH "00000000.00p";
//  int msg_fname = strlen(msg_path) - 12;
  
//#ifdef BCKP_PATH
//  char bak_path[] = BCKP_PATH "00000000.00p";
//  int bak_fname = strlen(bak_path) - 12;
//#endif

// Init section
  srandom(time(NULL));
  umask(0);
  chdir(work_dir);
  cd = iconv_open("CP1251","KOI8-R");
  msg_path = (char *) malloc (strlen(LINK_PATH)+13);
  if (fl_backup)
    bak_path = (char *) malloc (strlen(BCKP_PATH)+13);
  if (fl_rr)
    pthread_mutex_init(&rr_mutex, NULL);

  // Main loop
  while (0)
  {
    int readbytes;
    SRVPKT *spbuf;
    pthread_t tid;

    // Connect statement
    if ((sock = serv_connect(hostname,port)) < 0)
    {
      printf("connect() failed: %d\n", errno);
      sleep(5);
      continue;
    }

    // If RR is enabled start the RR loop
    if(fl_rr)
      pthread_create(&tid, NULL, rr_loop, NULL);

    // Read section
    spbuf = (SRVPKT *) malloc(sizeof(*spbuf));
    while ((readbytes = read(sock, spbuf, sizeof(*spbuf))) > 0)
    {

      // If packet is TCP_IP_DATA
      if (spbuf->typy == TCP_IP_DATA)
      {
        int len;
        char filename[13];
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
        sprintf(msg_path,LINK_PATH "%s",filename);
        write_msg(msg_path,msg,len);
        if (fl_backup)
        {
          sprintf(bak_path,BCKP_PATH "%s",filename);
          write_msg(bak_path,msg,len);
        }

        free(msg);
        printf("Saving success: %s\n", filename);

        // Read the end of packet
        read(sock, spbuf, sizeof(*spbuf));
        // If correct
        if (spbuf->typy == TCP_IP_END)
        {
          // Sending an acknowledgement
          spbuf->typy = TCP_IP_ACK;
          if (fl_rr)
            pthread_mutex_lock(&rr_mutex);
          if ((write(sock, spbuf, sizeof(*spbuf))) > 0)
            printf("ACK has been sent successful.\n");
          else
            printf("An send error has been occured. Error code is: %d\n",errno);
          if (fl_rr)
            pthread_mutex_unlock(&rr_mutex);
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
    
    if (fl_rr)
      pthread_cancel(tid);
    sleep(1);
    close(sock);
    sleep(3);
  }    

  iconv_close(cd);
/*  if (argc < 5)
    printf("mts_comm: invalid arguments\n");
  else*/
//    printf("Test\n");
  return 0;
}
