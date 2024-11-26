/*
  BYTE2STR
  reads a byte file and converts it to is unique "1"
  "0" file. Such that any string of ones and zeroes
  have a unique byte file and vicea versa.
  David Scott 2001 August.
TYPICAL USE "byte2str filein fileout"
COMPLIE first compile all minor routines then
  gcc byte2str.cpp *.o -o byte2str.exe
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


//bit_bytx        out;
//bit_bytn        out;
bit_byte        out;
int             symbol;

/* CURRENT STATE OF THE ENCODING. */


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
   out.iw(stdout);
   for( zc = -1; (ch = getc(stdin)) != EOF;) {
     if ( ch == '0' ) {
       out.w(0);
       fprintf (stderr,"0");
       zc = 0;
     } else if ( ch == '1' ) {
       out.w(1);
       fprintf (stderr,"1");
       if( zc != 0 ) zc = 1;
     } else break;
   }
      if(zc == 0) out.w(-1);
      else if(zc == 1)out.w(-2);
      else {
        fprintf(stderr," ** ERROR NOT A FILE OF ONES AND ZEROS ** ");
        return 0;
      }
      fprintf(stderr,"\n ** Conversion Done ** ");
   }
