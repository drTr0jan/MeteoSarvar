#include <stdio.h>
#include <stdlib.h>

void get_filename(char * filename)
{
  const int i = 067777777;
  int r = random() % i + 010000000;
  sprintf(filename,"%o.00p",r);
  return;
}

int write_msg(char * filename, char * msg, int len)
{
  FILE *fp;
  int error = 0;
  fp = fopen(filename,"w");
  if(fwrite(msg,len,1,fp) == 0)
    error = -1;
  fclose(fp);
  return error;
}
