/*
  STR2BYTE
  reads a byte file as a unique string of 1's and
  and 0's amd converts it to is unique "1"
  "0" file. Such that any string of ones and zeroes
  have a unique byte file and vicea versa.
  David Scott 2001 August.
TYPICAL USE "str2byte filein fileout"
COMPLIE first compile all minor routines then
  gcc str2byte.cpp *.o -o str2byte.exe
*/

#include <stdio.h>
#include <stdlib.h>
#if !defined( unix )
#include <io.h>
#endif
#include <fcntl.h>
#include "bit_byte.h"
#include "bit_bytx.h"


struct bij_2c {
   unsigned long   Fone;
   unsigned long   Ftot;
};

bij_2c          ff;



/* THE BIT BUFFER. */


//bit_bytx        in;
//bit_bytn        in;
bit_byte        in;
int             symbol;



int main(int argc, char *argv[])
{
   int             ch;
   int             zc;
   int             ticker = 0;
   if (argc > 1) 
      freopen(argv[1], "rb", stdin);
   if (argc > 2) 
      freopen(argv[2], "wb", stdout);
#if !defined( unix )
   setmode(fileno(stdin), O_BINARY);
   setmode(fileno(stdout), O_BINARY);
#endif
   ch = in.ir(stdin);
   for( zc = -1; (ch = in.r()) != -1;) {
     if ( ch == 0 ) {
       putc('0',stdout);
       fprintf (stderr,"0");
       zc = 0;
     } else if ( ch == 1 ) {
       putc('1',stdout);
       fprintf (stderr,"1");
       if( zc != 0 ) zc = 1;
     } else break;
   }
      if( zc == 1) {
       putc('1',stdout);
       fprintf(stderr,"1");
      }
      fprintf(stderr,"\n ** Conversion Done ** ");
   }
