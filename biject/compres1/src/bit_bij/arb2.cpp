/*
  ARB2.CPP
  This is loosly based on Mark Nelson arithmetic coder.
  This is a "two state bijective code". The input file is
  a bit stream that is compressed in a special bijective
  manner to the output file. Any file can be treated as
  a compresed file or an uncompressed file.
   Use it your own risk I make no claims as to its safety
  on you machine.
   David A. Scott August 2001
  demo code for 2 state 62 bit high low bijective adaptive
  code the mother of all bijective acompressors.
TYPICAL USE "arb2 filein fileout"
COMPLIE first compile all minor routines then
  gcc arb2.cpp *.o -o arb2.exe
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

/* ADAPTIVE SOURCE MODEL */


/* DECLARATIONS USED FOR ARITHMETIC ENCODING AND DECODING */

/* SIZE OF ARITHMETIC CODE VALUES. */

#define Code_value_bits 62	/* Number of bits in a code value   */
typedef unsigned long long code_value;	/* Type of an arithmetic code value */

#define Top_value code_value(0X3FFFFFFFFFFFFFFFull)	/* Largest code value */


/* HALF AND QUARTER POINTS IN THE CODE VALUE RANGE. */

#define First_qtr code_value(Top_value/4+1)	/* Point after first quarter    */
#define Half	  code_value(2*First_qtr)	/* Point after first half  */
#define Third_qtr code_value(3*First_qtr)	/* Point after third quarter  */

void encode_symbol(int, bij_2c);


/* THE BIT BUFFER. */


bit_bytx        out;
bit_byte        in;
int             symbol;
int             Zero_av;
int             ZATE;
int             sw;
int             sww;

/* CURRENT STATE OF THE ENCODING. */

code_value      low, high;	/* Ends of the current code region          */
code_value      bits_to_follow;	/* Number of opposite bits to output after  */
                                /* the next bit.                            */


/* OUTPUT BITS PLUS FOLLOWING OPPOSITE BITS. */

void bit_plus_follow(int bit)
{
   static int      zc = 0;
   if (bit == 1) {
      for (; zc > 0; zc--)
	 out.w(0);
      for (out.w(1^sww); bits_to_follow > 0; bits_to_follow--)
	 zc++;
      sww = 0;
   } else if (bit == 0) {
      if (bits_to_follow > 0) {
	 for (; zc > 0; zc--)
	    out.w(0);
	 for (out.w(0^sww); bits_to_follow > 0; bits_to_follow--)
	    out.w(1);
         sww = 0;
      } else
	 zc++;
   } else
      abort();
}

int main(int argc, char *argv[])
{
   int             ch;
   int             ticker = 0;
   fprintf(stderr, "Bijective Arithmetic 2 state coding version 20010822 \n ");
   fprintf(stderr, "Arithmetic coding on ");
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
   in.ir(stdin);
   ch = in.r();
   if (ch < 0) {
      out.w(-1);
      return 0;
   }
   if (ch == 0) {
      sw = 1;
      sww = 1;
   } else {
      sw = 0;
      sww = 0;
   }
   high = Top_value;
   low = Half - 1;
   ch = in.r();
   Zero_av = 1;
   ff.Ftot = 3;
   ff.Fone = 2;
   bits_to_follow = 0;		/* No bits to follow next.  */
   for (;;) {			/* Loop through characters. */
      if ((ticker++ % 1024) == 0)
	 putc('.', stderr);
      if (ch < 0)
	 break;
      encode_symbol(ch^sw, ff);
      if ((ch^sw) == 1)
	 ff.Fone++;
      ff.Ftot++;
      ch = in.r();
   }
   encode_symbol(-1, ff);
   if (low == 0 && Zero_av == 1) {
      bit_plus_follow(0);
   } else
      bit_plus_follow(1);
   out.w(-2);
   return 0;
}

/* ENCODE A SYMBOL. */

void encode_symbol(int symbol, bij_2c ff)
{
   code_value      range, a, b;	/* Size of the current code region          */
   int             s1;
   range = (code_value) (high - low) + 1;	/* so first bit unchanged */
   range = range / ff.Ftot;
   range = range * ff.Fone;
   range = high - range;

   if (symbol == 0)
      high = range - 1;
   else if (symbol == 1)
      low = range;
   else {
      /* do what ever symbol has output test each for 1 then 0 then C */
      if (range >= Half) {
	 symbol = 1;
	 low = range;
      } else {
	 symbol = 0;
	 high = range - 1;
      }
   }
   ZATE = ((high < Half) || (low >= Half)) ? 1 : 0;
   s1 = 0;
   for (;;) {			/* Loop to output bits.     */
      if (high < Half) {
	 bit_plus_follow(0);	/* Output 0 if in low half. */
      } else if (low >= Half) {	/* Output 1 if in high half. */
	 bit_plus_follow(1);
	 Zero_av = 0;
	 low -= Half;
	 high -= Half;		/* Subtract offset to top.  */
      } else if (low >= First_qtr	/* Output an opposite bit   */
		 && high < Third_qtr) {	/* later if in middle half. */
	 bits_to_follow += 1;
	 Zero_av = 0;
	 low -= First_qtr;	/* Subtract offset to middle */
	 high -= First_qtr;
      } else {
	 break;			/* Otherwise exit loop.     */
      }
      low = 2 * low;
      high = 2 * high + 1;	/* Scale up code range.     */
      s1 = 1;
   }
   if (ZATE == 1) {
      if (low == 0 && Zero_av == 0)
	 Zero_av = 1;		/* only first time */
      else if (low > 0)
	 Zero_av = 0;		/* available in future */
      else
	 Zero_av = 2;		/* not available */
   } else if (s1 == 1)
      Zero_av = 0;
}
