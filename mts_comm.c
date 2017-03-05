#include <getopt.h>
#include <math.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
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
#include "files.h"

#define WORK_DIR  "/var/db/meteo/"
#define LINK_PATH "LINK/PRM_ASPD_S/"
#define BCKP_PATH "LINK/backup/"

#ifndef LOG_FACILITY
#define LOG_FACILITY LOG_LOCAL0
#endif

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
_Bool fl_daemon = 0;

int convert_msg8(char *, size_t);
void get_filename(char *);
int get_time_msec(char *);
int log_message(int, const char *, ...);
void *rr_loop(void *);
int serv_connect(const char *, uint16_t);
void sigterm(int);
int usage(void);
int write_msg(char *, char *, int);

int convert_msg8(char * msg, size_t len)
{
  size_t srcleft = len;
  size_t dstleft;
  char *buf = (char *)alloca(len);
  char *dst = buf;
  char *src = msg;
  iconv_t cd = iconv_open("CP1251","KOI8-R");

  iconv(cd,&src,&srcleft,&dst,&dstleft);
  strncpy(msg,buf,len);

  iconv_close(cd);
  return 0;
}

void get_filename(char * filename)
{
  const int i = 067777777;
  int r = random() % i + 010000000;
  sprintf(filename,"%o.00p",r);
  return;
}

int get_time_msec(char *buf)
{
  int millisec;
  long usec;
  struct tm* tm_info;
  struct timeval tv;

  gettimeofday(&tv, NULL);

  millisec = lrint(tv.tv_usec/1000.0);
  if (millisec>=1000)
  {
    millisec -=1000;
    tv.tv_sec++;
  }

  tm_info = localtime(&tv.tv_sec);

  sprintf(buf, "%.2d:%.2d:%.2d.%03d", 
	    tm_info->tm_hour, tm_info->tm_min,
	    tm_info->tm_sec, millisec);

  return 0;
}

int log_message(int priority, const char *msg, ...)
{
  char log_message[80];
  va_list ap;

  va_start(ap, msg);
  if (!fl_daemon)
  {
    char curtime[13];

    get_time_msec(curtime);
    vsprintf(log_message, msg, ap);
    printf("%s %s\n", curtime, log_message);
  }
  else
    vsyslog(priority, msg, ap);
  va_end(ap);


  return 0;
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
      log_message(LOG_INFO, "RDY has been sent.");
    else
      log_message(LOG_WARNING, "RDY error has been occured while sent.");
    pthread_mutex_unlock(&rr_mutex);
  }
  return((void *)0);
}

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
void sigterm(int signo)
{
  puts("Exiting...");
  exit(0);
}

int usage()
{
  puts("usage: mts_connect [-bdr] -s host [-a host] -p channel [-w path]");
  return 0;
}

int write_msg(char * filename, char * msg, int len)
{
  FILE *fp;
  int error = 0;

  fp = fopen(filename,"w");
  if(fp == NULL)
    error = -1;
  if(fwrite(msg,len,1,fp) == 0)
    error = -1;
  fclose(fp);
  return error;
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
  struct sigaction sa;

  if (argc < 2)
  {
    usage();
    return EXIT_FAILURE;
  }
  while ((opt = getopt(argc, argv, "hbdrs:a:p:w:")) != -1)
  {
    switch (opt)
    {
      case 'h':
        usage();
        return EXIT_SUCCESS;
      case 'b':
        fl_backup = 1;
        break;
      case 'd':
        fl_daemon = 1;
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

// Init section
  srandom(time(NULL));
  umask(0);
  chdir(work_dir);
  if (fl_daemon)
    openlog("mts_comm",0,LOG_FACILITY);
  if (!fl_backup || (strlen(LINK_PATH) >= strlen(BCKP_PATH)))
    msg_path = (char *) malloc (strlen(LINK_PATH)+13);
  else
    msg_path = (char *) malloc (strlen(BCKP_PATH)+13);
  if (fl_rr)
    pthread_mutex_init(&rr_mutex, NULL);
  sa.sa_handler = sigterm;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  if(sigaction(SIGTERM, &sa, NULL)<0)
  {
    puts("Signal error");
    return EXIT_FAILURE;
  }

  // Main loop
  while (1)
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
      int pnum1, pnum2;
      
      // If packet is TCP_IP_DATA
      if (spbuf->typy == TCP_IP_DATA)
      {
        int len;
        char filename[13];
        char log_text[80];
        char * msg;

        len = ntohs(spbuf->len);
        pnum1 = ntohs(spbuf->num);
//        sprintf(log_text,"Starting capture a DATA packet #%d (%d B)",pnum1,len);
        log_message(LOG_INFO, "Starting capture a DATA packet #%d (%d B)", pnum1, len);

        // Receive the data
        msg = (char *) malloc(len);
        recv(sock, msg, len, MSG_WAITALL);
//printf("Rec size: %d\n",ret);
//      int ret = read(sock,tmpbuf,12);
//      if (ret == -1)
//        printf("An error %d occured.\n",errno);

        // Convert the data
        convert_msg8(msg,len);
//        printf("Converting success\n");



//        printf("Saving success: %s\n", filename);

        // Read the end of packet
        read(sock, spbuf, sizeof(*spbuf));

        // If correct
        pnum2 = ntohs(spbuf->num);
        if ((spbuf->typy == TCP_IP_END) && (pnum1 == pnum2))
        {
          // Save the data
          get_filename(filename);
          sprintf(msg_path,LINK_PATH "%s",filename);
          write_msg(msg_path,msg,len);
//          sprintf(log_text,"The DATA packet (%d: %s) saved to %s.",
//          		pnum1, spbuf->ahd, msg_path);
          log_message(LOG_INFO, "The DATA packet (%d: %s) saved to %s.",
          		pnum1, spbuf->ahd, msg_path);

          if (fl_backup)
          {
            sprintf(msg_path,BCKP_PATH "%s",filename);
            write_msg(msg_path,msg,len);
//            sprintf(log_text,"The DATA packet (%d: %s) backsaved to %s.",
//            		pnum1, spbuf->ahd, msg_path);
            log_message(LOG_DEBUG, "The DATA packet (%d: %s) backsaved to %s.",
            		pnum1, spbuf->ahd, msg_path);
          }

          // Sending an acknowledgement
          spbuf->typy = TCP_IP_ACK;
          if (fl_rr)
            pthread_mutex_lock(&rr_mutex);
          if ((write(sock, spbuf, sizeof(*spbuf))) > 0)
          {
//            sprintf(log_text,"ACK (%d) sended success.",pnum1);
            log_message(LOG_INFO, "ACK (%d) sended success.", pnum1);
          }
          else
            printf("An send error has been occured. Error code is: %d\n",errno);
          if (fl_rr)
            pthread_mutex_unlock(&rr_mutex);
        }
        else
        {
//          sprintf(log_text,"The capture of a DATA packet (%d) #%d/%d failed.",spbuf->typy,pnum1,pnum2);
          log_message(LOG_ERR, "The capture of a DATA packet (%d) #%d/%d failed.", spbuf->typy, pnum1, pnum2);
         // printf("A capturing error has been occured. The packet type is %d, but must be TCP_IP_END.\n",spbuf->typy);
        }
        free(msg);
      }
      // Print a packet type if not
      else if (spbuf->typy == TCP_IP_RR)
      {
        log_message(LOG_INFO, "RDY packet has been received.");
      }
      else
      {
        printf("The packet type is %d.\n",spbuf->typy);
      }
/*    printf("Length = %d\n",ntohs(spbuf->len));
    printf("Number = %d\n",ntohs(spbuf->num));
    printf("Header = %s\n",spbuf->ahd);
    printf("Priority = %d\n",spbuf->pri);*/
    }
    free(spbuf);

    // If receiving problem
    if (readbytes == -1)
      log_message(LOG_ERR,"An receive error has been occured. Error code is: %d\n",errno);
    if (readbytes == 0)
      log_message(LOG_NOTICE, "An EOF has been gotten.\n");

    if (fl_rr)
      pthread_cancel(tid);
    sleep(1);
    close(sock);
    sleep(3);
  }

  return 0;
}

