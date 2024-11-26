/*
  A012B01.CPP 
   in a bijective way.
    Converts any ascii string of "1" and "0" to a normal
  binary file in a bijective way.

   David A. Scott December 2007
*** use at your own risk ***
TYPICAL USE "a012b01 filein fileout"
COMPLIE first compile all minor routines then
  gcc arb2x.cpp *.o -o arb2x.exe
*/

#include <stdio.h>
#include <stdlib.h>
#if !defined( unix )
#include <io.h>
#endif
#include <fcntl.h>
#include "bit_byts.h"


struct bij_2c {
   unsigned long Fone;
   unsigned long Ftot;
};

bij_2c ff;


bit_byts out;
bit_byts in;

int main(int argc, char *argv[])
{
   int ch;
   int ticker = 0;
   unsigned int numb0 = 0;
   unsigned int numb1 = 0;
   unsigned int ntot = 0;
   fprintf(stderr, "Bijective convert ascii 0 and 1 to binary");
   fprintf(stderr, " file version 20071227 \n ");
   fprintf(stderr, " on ");
   if (argc > 1) {
      freopen(argv[1], "rb", stdin);
      fprintf(stderr, "%s", argv[1]);
   } else
      fprintf(stderr, "stdin");
   fprintf(stderr, " to ");
   if (argc > 2) {
      freopen(argv[2], "wb", stdout);
      fprintf(stderr, "%s", argv[2]);
   } else
      fprintf(stderr, "stdout");
   fprintf(stderr, "\n");
#if !defined( unix )
   setmode(fileno(stdin), O_BINARY);
   setmode(fileno(stdout), O_BINARY);
#endif
   out.iw(stdout);
   in.irc(stdin);
   fprintf(stderr, "\n");
   for (;;) {
      ch = in.rc();
      if ((ticker++ % 1024) == 0)
	 putc('.', stderr);
      if (ch == -1)
	 break;
      if (ch == 1) {
	 out.w(1);
	 numb1++;
      } else if (ch == 0) {
	 out.w(0);
	 numb0++;
      } else {
	 fprintf(stderr, " ** warning either unvalid input file ** \n");
	 fprintf(stderr, " ** or empty file on input ** \n");
	 exit(1);
      }
   }
   out.w(-1);
   numb1 += numb0 == 0 ? 1: 0; 
   fprintf(stderr, " \n number of ones %u number of zeros %u \n", numb1,
	   numb0);
   ntot = numb0 + numb1;
   fprintf(stderr, " string has %u bits and %u.%u  bytes \n",
	   ntot, (ntot >> 3), ((ntot % 8) * 125));
   fprintf(stderr, " *** DONE *** ");
}
