/*
  ARB255.CPP
  This is loosly based on Mark Nelson arithmetic coder.
  This is a "two state bijective code". That uses a tree
  arrangement of two state bijective coders for the
  compression. See ARB2x.CPP for details.
   Use it your own risk I make no claims as to its safety
  on you machine.
   David A. Scott July 2004
  demo code for 2 state 64 bit high low bijective adaptive
  code the mother of all bijective acompressors.
  used to compress bytes using a tree.
TYPICAL USE "arb255 filein fileout"
COMPLIE first compile all minor routines then
  gcc arb255.cpp *.o -o arb255.exe
*/

#include <stdio.h>
#include <stdlib.h>
#if !defined( unix )
#include <io.h>
#endif
#include <fcntl.h>
#include "bit_byts.h"


struct bij_2c {
   unsigned long long   Fone;
   unsigned long long   Ftot;
};

bij_2c          ff[255]; /* 256 terminal nodes */

int cc; /* curent cell */


/* ADAPTIVE SOURCE MODEL */


/* DECLARATIONS USED FOR ARITHMETIC ENCODING AND DECODING */

/* SIZE OF ARITHMETIC CODE VALUES. */

#define Code_value_bits 64	/* Number of bits in a code value   */
typedef unsigned long long code_value;	/* Type of an arithmetic code value */

#define Top_value code_value(0XFFFFFFFFFFFFFFFFull)	/* Largest code value */


/* HALF AND QUARTER POINTS IN THE CODE VALUE RANGE. */


#define Half	  code_value((Top_value >> 1) +1)	/* Point after first
							 * half  */
#define First_qtr code_value(Half >> 1)	/* Point after first quarter    */
#define Third_qtr code_value(Half + First_qtr)	/* Point after third quarter */


void encode_symbol(int, bij_2c);

code_value freeend;		/* current free end */
code_value      fcount;
int             CMOD = 0;
int             FRX = 0;
int             FRXX = 0;



/* THE BIT BUFFER. */


bit_byts        out;
bit_byts        in;


/****** . ********/
/* out.wz or out.ws */
#define dasr in.r()
#define dasw out.ws
// #define dasw out.wz

int             symbol;
int             ZATE;
code_value      Nz, No;		/* number future zeros or ones */

/* CURRENT STATE OF THE ENCODING. */

code_value      low, high;	/* Ends of the current code region          */
code_value      bits_to_follow;	/* Number of opposite bits to output after  */
/* the next bit.                            */


/* increment free point */


void 
fre_2_cnt(void)
{
   code_value      f1, f2, f3;
   f3 = freeend;
   for (f1 = Half, f2 = 1, fcount = 1; f3 != 0; f1 >>= 1) {
      if (f3 == f1)
	 break;
      fcount <<= 1;
      if (f1 & f3) {
	 fcount++;
	 f3 -= f1;
      }
   }
   if (f3 == 0)
      fcount = 0;
}

code_value
cnt_2_fre(void)
{
   code_value      f1, f2, f3;
   if (fcount == 0 || fcount > Top_value) {
      freeend = 0;
      return 0;
   }
   f3 = fcount;
   for (f1 = Half, f2 = 1, freeend = Half; f3 > 1; f3 >>= 1) {
      f1 >>= 1;
      freeend >>= 1;
      if (f2 & f3) {
	 freeend += Half;
      }
   }
   return f1;
}

 
void
inc_fre(void)
{
   code_value      freeetemp;
   code_value      f1;
   fre_2_cnt();
   fcount++;
   freeetemp = cnt_2_fre();
   if (freeend == 0) {
      FRX = 1;
      FRXX = 1;
      freeend = low;
      return;
   }
   if (low <= freeend && freeend <= high){
      return;
   }
   if (fcount > (Top_value - 1)) {
     FRX = 1;
     FRXX = 1;
     freeend = low;
     return;
    }
   if (freeend > high) {
      freeetemp >>= 1;
      for(;freeetemp > high;) {
      freeetemp >>= 1;
      }
      if (freeetemp == 0) {
	 FRX = 1;
	 FRXX = 1;
         freeend = low;
	 return;
      } else if (low <= freeetemp && freeetemp <= high) {
	 freeend = freeetemp;
	 return;
      }
   }
   f1 = Top_value >> 1;
   f1 = f1 + freeetemp;
   f1 -= Half;
   freeend = 0;
   for (;; f1 >>= 1, freeetemp >>= 1) {
      freeend = ((low + f1) & ~f1) | freeetemp;
      if ( freeetemp == 0 ) {
         FRX = 1;
         return;
      }
      if (low <= freeend && freeend <= high)
	 break;
   }
}


/* OUTPUT BITS PLUS FOLLOWING OPPOSITE BITS. */

void
bit_plus_follow(int bit)
{
   for (dasw(bit); bits_to_follow > 0; bits_to_follow--)
      dasw(1 ^ bit);
}



int main(int argc, char *argv[])
{
   int             ch;
   int             ticker = 0;
   fprintf(stderr, "Bijective Arithmetic 2 state coding version 20040723 \n ");
   fprintf(stderr, "Arithmetic of 256 symbols coding on ");
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
   for ( cc = 255; cc-->0; ) {
      ff[cc].Fone = 1;
      ff[cc].Ftot = 2;
   }
   out.iw(stdout);
   in.ir(stdin);
   cc = 0;
   high = Top_value;
   low = 0;
   freeend = Half;
   fcount = 1;
   bits_to_follow = 0;		/* No bits to follow next.  */
   for (;;) {			/* Loop through characters. */
      ch = in.r();
      if ((ticker++ % 1024) == 0)
	 putc('.', stderr);
      if (ch < 0)
	 break;
      encode_symbol(ch, ff[cc]);
      if ( (ch) == 1)
	 ff[cc].Fone++;
      ff[cc].Ftot++;
      if (ch == 0 ) {
        cc = 2 * cc + 1;
      } else {
        cc = 2 * cc + 2;
      }
      if (cc >= 255 ) cc = 0;
      /* cc = 0; */ 
   }
   fprintf(stderr, "\n EOS = ");
   fcount = Half;
   if (freeend == 0)
      fprintf(stderr, " { NULL } ");
   for (; freeend != 0; fcount >>= 1) {
      ch = (fcount & freeend) != 0 ? 1 : 0;
      bit_plus_follow(ch);
      if (ch == 1) {
	 fprintf(stderr, "1");
	 freeend -= fcount;
      } else
	 fprintf(stderr, "0");
   }
   bit_plus_follow(0);
   dasw(-2);
   fprintf(stderr, " SUCCESSFUL \n");
   if (FRXX == 1)
      fprintf(stderr, "BUT USED HIGH FREEENDS");
   return 0;

}

/* ENCODE A SYMBOL. */


void
encode_symbol(int symbol, bij_2c ff)
{
   code_value      c, a, b;	/* Size of the current code region          */
   code_value      Fzero;
   int             LPS;




   if (high < low || freeend > high || freeend < low) {
      fprintf(stderr, " STOP 1 impossible exit ");
      exit(0);
   }
   c = high - low;		/* so first bit unchanged */
   a = c / ff.Ftot;
   b = c - a * ff.Ftot;
   Fzero = ff.Ftot - ff.Fone;
   if (Fzero > ff.Fone) {
      LPS = 1;
      a = a * ff.Fone + (b * ff.Fone) / ff.Ftot;
   } else {
      LPS = 0;
      a = a * Fzero + (b * Fzero) / ff.Ftot;
   }
   if ((low + a) > (high - a))
      a--;
   if (low >= First_qtr && (high - a) <= Third_qtr &&
       (high - a) >= Half) {
      /* LPS top of interval */
      if (symbol == LPS)
	 low = high - a;
      else
	 high = (high - a) - 1;
   } else if (symbol == LPS)	/* LPS bottom of interval */
      high = low + a;
   else
      low = low + a + 1;
   /*
    * Notice this is not like most 2 state arithmetic compressors where
    * either the symbol 1 or 0 is set to the top part when split or the more
    * recent method of where the LSB part is set to the top of interval. This
    * is set such that LSB is usually at bottom of interval to keep freeend
    * values low. Except that when the LSB low would result only one of the
    * two split intervals where a LSB high portion allows expansion to occur
    * in both that path is taken since it reduces freeend value and gives an
    * expanded interval for dividing that the other split would not
    */


   /* is current free end in new space */
   if (FRX != 0) {
      if (low > freeend) freeend = low;
      else if (freeend < high) freeend += 1;
      else {
        fprintf(stderr, "\n NO FREE END SO FATAL ERROR ");
        fprintf(stderr, "\n THIS SHOULD NOT HAPPEN ");
        exit(0);
      }
   } else if (freeend == Top_value) {
      freeend = low;
      FRX = 1;
   } else if (CMOD == 0 || (freeend | Half) != Half) {
      inc_fre();
   } else if (freeend == 0 || low != 0) {
      freeend = Half;
      inc_fre();
   } else
      freeend = 0;
   if ((freeend > high || freeend < low)) {
      fprintf(stderr, "\n NOWAY ");
      exit(0);
   }

   for (;;) {			/* Loop to output bits.     */
      if (high < Half) {
	 CMOD = 0;
	 bit_plus_follow(0);	/* Output 0 if in low half. */
      } else if (low >= Half) {	/* Output 1 if in high half. */
	 CMOD = 0;
	 bit_plus_follow(1);
	 low -= Half;
	 high -= Half;		/* Subtract offset to top.  */
	 freeend -= Half;
      } else if (low >= First_qtr	/* Output an opposite bit   */
		 && high < Third_qtr) {	/* later if in middle half. */
	 CMOD = 1;
	 bits_to_follow += 1;
	 freeend -= First_qtr;
	 low -= First_qtr;	/* Subtract offset to middle */
	 high -= First_qtr;
      } else
	 break;			/* Otherwise exit loop.     */
      low = 2 * low;
      high = 2 * high + 1;	/* Scale up code range.     */
      freeend = 2 * freeend + FRX;
      FRX = 0;
   }
   return;
}
