#include <stdio.h>
#include <stdlib.h>

char * get_filename(void){
  char * filename;
  const int i = 067777777;
  int r = random() % i + 010000000;
  sprintf(filename,"%o.00p",r);
  return filename;
}

/*int main(int argc, char *argv[])
{
  char * str;

  srandom(time(NULL));
 
  str = get_filename();
//  int result = r % i;
  printf("Random number is: %s\n",str);
  return 0;
}
*/