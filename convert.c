#include <stdio.h>
#include <stdlib.h>
#include <iconv.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

int convert_msg8(iconv_t cd, char ** msg, size_t len){
  size_t plen = len;
  size_t alen = len;
  char *buf = malloc(len);
  char *result = buf;
//  printf("(1)\n");
  iconv(cd,msg,&plen,&buf,&alen);
//  printf("(2)\n");
  strncpy(*msg,result,len);
//  printf("(3)\n");  
  free(result);
  return 0;
}

/*int main(int argc, char *argv[])
{
  iconv_t cd;
  int len, i;
  size_t plen, alen;
  char *msg, *cmsg, *lmsg;
  FILE *fp;

  cd = iconv_open("CP1251","KOI8-R");
  
  len = 1734;
  msg = (char *)malloc(len);
  
  fp = fopen("tmp.bin","r");
  
  fread(msg,len,1,fp);
  fclose(fp);
  
  convert_msg8(cd,&msg,len);

  fp = fopen("new.bin","w");
  fwrite(msg,len,1,fp);
  fclose(fp);
  
  free(msg);

  iconv_close(cd);
  printf("Test\n");
  return 0;
}
*/