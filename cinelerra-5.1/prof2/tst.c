#include <stdio.h>

void tst0()
{
   printf(".");
}

void tst1()
{
   printf("\r");
}

void tst2(int n)
{
   volatile int i = n;
   while( --i >= 0 );
   if( (n&1) != 0 ) tst1(); else tst0();
}

int main(int ac, char **av,char **ev)
{
   int i;
   //profileStart();
   setbuf(stdout,NULL);
   printf("ok. %s\n",ev[0]);
   for( i=30000; --i>=0; ) tst2(i);
   printf("\n");
   printf("done\n");
   return 0;
}

