/*
  UNARB255.CPP
  This is loosly based on Mark Nelson arithmetic coder.
  This is a "two state bijective code". That uses a tree
  arrangement of two stat bijective coders for the
  compression. See UNARB2X.CPP for details.
   Use it your own risk I make no claims as to its safety
  on you machine.
   David A. Scott July 2005
  demo code for 2 state 64 bit high low bijective adaptive
  code the mother of all bijective acompressors.
  used to compress bytes using a tree.
TYPICAL USE "unarb255 filein fileout"
COMPLIE first compile all minor routines then
  gcc unarb255.cpp *.o -o unarb255.exe
*/

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#if !defined( unix )
#include <io.h>
#endif
#include "bit_byts.h"


/* DECLARATIONS USED FOR ARITHMETIC ENCODING AND DECODING */

struct bij_2c {
   unsigned long long   Fone;
   unsigned long long   Ftot;
};

bij_2c          ff[255]; /* 256 terminal nodes */

int cc; /* curent cell */

/* SIZE OF ARITHMETIC CODE VALUES. */


typedef unsigned long long code_value;	/* Type of an arithmetic code value */

#define Top_value code_value(0XFFFFFFFFFFFFFFFFull)	/* Largest code value */
// #define Top_value code_value(0X1ull)	/* smallest Largest code value */


/* HALF AND QUARTER POINTS IN THE CODE VALUE RANGE. */

#define Half	  code_value((Top_value >> 1) +1)	/* Point after first
							 * half  */
#define First_qtr code_value(Half >> 1)	/* Point after first quarter    */
#define Third_qtr code_value(Half + First_qtr)	/* Point after third quarter */

/* The free end thingy */
/*
 * This is like Matt Timmermans free ends. There are many ways to make the
 * endings bijective however most I have used in the past are not as clean in
 * the endings as what a modified method of Matt does
 * 
 * There are bacially 2 sets of free ends. One occurs when a free zero is
 * available or if still in interval the First_qtr point is its alternate
 * free end. The other is around the Half point. I start there and
 * sysmatically go higher till past the current high. If still in interval
 * with Half point I then go down in value till less than low at with point I
 * shift to next higher point and repeat. This can never fail since interval
 * had to expand eventually when it becomes on the order of half the points
 * in the space and eventually you would use all the free points up but by
 * then the interval would become haf size at witch time the number of points
 * in the interval at least doubles.
 */

code_value freeend;		/* current free end */
code_value      fcount;
int             CMOD = 0;
int             FRX = 0;
int             FRXX = 0;


/* BIT INPUT ROUTINES. */

/* THE BIT BUFFER. */

bit_byts        in;
bit_byts        out;

/****** . ********/
/* in.r or in.rs */
#define dasr in.rs()
// #define dasr in.r()
#define dasw out.wz


int             ZEND;		/* flag to tell when last one bit in in the
				 * IFO file sense */
code_value      VALUE;
static code_value low, high;	/* Ends of current code region              */



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

/* INPUT A BIT. */

inline int
input_bit(void)
{
   int             t;
   t = dasr;
   if (t < 0) {
      if (t == -1)
	 t = 1;
      else
	 t = 0;
      ZEND = 1;
   }
   return t;
}

/* CURRENT STATE OF THE DECODING. */


void            start_decoding(void);
int             decode_symbol(bij_2c);



int 
main(int argc, char *argv[])
{
   int             ticker = 0;
   int             ch;
   fprintf(stderr, "Bijective Arithmetic 2 state uncoding version 20040723\n");
   fprintf(stderr, "Arithmetic of 256 Symbols decoding on ");
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
   low = 0;
   high = Top_value;
   start_decoding();
   for (;;) {			/* Loop through characters. */
      if ((ticker++ % 1024) == 0)
	 putc('.', stderr);
      ch = decode_symbol(ff[cc]);
      dasw(ch);
      if (ch == -1)
	 break;
      if (ch == 1)
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
      if (ch == 1) {
	 fprintf(stderr, "1");
	 freeend -= fcount;
      } else
	 fprintf(stderr, "0");
   }
   fprintf(stderr, " SUCCESSFUL \n");
   if (FRXX == 1)
      fprintf(stderr, "BUT USED HIGH FREEENDS");
   return 0;
}

/* ARITHMETIC DECODING ALGORITHM. */
/* START DECODING A STREAM OF SYMBOLS. */



void
start_decoding(void)
{
   VALUE = 1;
   freeend = Half;
   fcount = 1;
   ZEND = 0;
   for (; VALUE < Half;) {
      VALUE = 2 * VALUE + input_bit();	/* Move in next input bit.  */
   }
   VALUE -= Half;
   VALUE = 2 * VALUE + input_bit();	/* Move in next input bit.  */
}

/* DECODE THE NEXT SYMBOL. */
code_value      BBB = 100;
int
decode_symbol(bij_2c ff)
{
   code_value      c, a, b;	/* Size of the current code region          */
   code_value      Fzero;
   code_value      oldlow, oldhigh;
   int             LPS;
   static int      EXX = 0;
   int             symbol = 0;	/* Symbol decoded                      */

   oldlow = low;
   oldhigh = high;
   if (high < low || freeend > high || freeend < low) {
      fprintf(stderr, " STOP 1 impossible exit ");
      exit(0);
   }
   if (ZEND == 1 && VALUE == freeend && FRX == 0)
      return (-1);		/* EXIT DONE */

   if (ZEND == 1 && FRX == 0 && ((VALUE == 0 && CMOD == 0) ||
		     (VALUE == Half && CMOD == 1))) {
      fprintf(stderr, " STOP past end ");
      EXX++;
      if (EXX > 5) {
	 exit(0);
      }
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
      if (VALUE >= (high - a)) {/* LPS top of interval */
	 symbol = LPS;
	 low = high - a;
      } else {
	 symbol = 1 - LPS;
	 high = (high - a) - 1;
      }
   } else
    /* LPS bottom of interval */ if (VALUE <= (low + a)) {
      symbol = LPS;
      high = low + a;
   } else {
      symbol = 1 - LPS;
      low = low + a + 1;
   }


   /* is current free end in new space */
   if (FRX != 0) {
fprintf(stderr, "\n HERE AT LAST ");
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



   if (high < low || low < oldlow || high > oldhigh) {
      fprintf(stderr, " STOP 2 impossible exit ");
      exit(0);
   }
   if (VALUE > high || VALUE < low) {
      fprintf(stderr, " not possible high = %16.16llx VALUE = %16.16llx low = %16.16llx ", high, VALUE, low);
      exit(0);
   }
   for (;;) {			/* Loop to get rid of bits. */
      if (high < Half) {
	 CMOD = 0;
	 /* nothing *//* Expand low half.         */
      } else if (low >= Half) {	/* Expand high half.        */
	 CMOD = 0;
	 VALUE -= Half;
	 freeend -= Half;
	 low -= Half;		/* Subtract offset to top.  */
	 high -= Half;
      } else if (low >= First_qtr	/* Expand middle half.      */
		 && high < Third_qtr) {
	 CMOD = 1;
	 VALUE -= First_qtr;
	 freeend -= First_qtr;
	 low -= First_qtr;	/* Subtract offset to middle */
	 high -= First_qtr;
      } else
	 break;			/* Otherwise exit loop.     */
      low = 2 * low;
      high = 2 * high + 1;	/* Scale up code range.     */
      VALUE = 2 * VALUE + input_bit();	/* Move in next input bit.  */
      freeend = 2 * freeend + FRX;
      FRX = 0;
   }
   return (symbol);
}
