/*
  UNARB256.CPP
  This is loosly based on Mark Nelson arithmetic coder.
  This is a "two state bijective code". That uses a tree
  arrangement of two stat bijective coders for the
  compression. See UNARB2.CPP for details.
   Use it your own risk I make no claims as to its safety
  on you machine.
   David A. Scott August 2001
  demo code for 2 state 62 bit high low bijective adaptive
  code the mother of all bijective acompressors.
  used to compress bytes using a tree.
TYPICAL USE "unarb256 filein fileout"
COMPLIE first compile all minor routines then
  gcc unarb256.cpp *.o -o unarb256.exe
*/

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#if !defined( unix )
#include <io.h>
#endif
#include "bit_byte.h"
#include "bit_bytx.h"


/* DECLARATIONS USED FOR ARITHMETIC ENCODING AND DECODING */

struct bij_2c {
   unsigned long   Fone;
   unsigned long   Ftot;
};

bij_2c          ff[257]; /* 256 terminal nodes */

int cc; /* curent cell */

/* SIZE OF ARITHMETIC CODE VALUES. */

#define Code_value_bits 62	/* Number of bits in a code value   */
typedef unsigned long long code_value;	/* Type of an arithmetic code value */

#define Top_value code_value(0X3FFFFFFFFFFFFFFFull)	/* Largest code value */


/* HALF AND QUARTER POINTS IN THE CODE VALUE RANGE. */

#define First_qtr code_value(Top_value/4+1)	/* Point after first quarter    */
#define Half	  code_value(2*First_qtr)	/* Point after first half  */
#define Third_qtr code_value(3*First_qtr)	/* Point after third quarter  */


/* BIT INPUT ROUTINES. */

/* THE BIT BUFFER. */

bit_bytx        in;
bit_byte        out;

int             ZEND;		/* next 2 flags to tell when last one bit in
				 * converted to FOF */
int             ZATE;		/* has just been processed. ***NOT always
				 * last one bin file** */
int             Zero_av;
code_value      VALUE;
int             sw;

/* INPUT A BIT. */

inline int input_bit(void)
{
   int             t, garbage_bits = 0;
   t = in.r();
   if (t == -2) {		/* Read the next byte if no */
      garbage_bits += 1;	/* Return arbitrary bits */
      if (garbage_bits > (2 * Code_value_bits)) { /* after eof, but check */
	 fprintf(stderr, "**Bad  should not occurr** \n");
	 exit(-1);
      }
   }
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

static code_value low, high;	/* Ends of current code region              */

void            start_decoding(void);
int             decode_symbol(bij_2c);

int 
main(int argc, char *argv[])
{
   int             ticker = 0;
   int             symbol = 1;
   int             ch;
   int             f_p;
   fprintf(stderr, "Bijective Arithmetic 2 state uncoding version 20010822\n");
   fprintf(stderr, "Arithmetic of 256 symbols decoding on ");
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
   for ( cc = 257; cc-->0; ) {
      ff[cc].Fone = 1;
      ff[cc].Ftot = 2;
   }
   out.iw(stdout);
   in.ir(stdin);
   cc = 0;
   start_decoding();
   if (ZATE == 1 && ZEND == 1 && VALUE == Half) {
      out.w(-1);		/* only a eof marker in file */
      return 0;
   }
   if (VALUE < Half) {
      VALUE += Half;
      ch = 1;
      sw = 1;
   } else {
      sw = 0;
      ch = 1;
   }
   high = Top_value;
   low = Half - 1;
   ff[cc].Fone = 2;
   ff[cc].Ftot = 3;
   Zero_av = 1;
   cc = 2;
   for (;;) {			/* Loop through characters. */
      if ((ticker++ % 1024) == 0)
	 putc('.', stderr);
      if (ZEND == 1 && ZATE == 1) {
	 if ((VALUE == 0 && Zero_av == 1) || VALUE == Half)
	    break;
      }
      out.w(sw^ch);
      ch = decode_symbol(ff[cc]);
      if (ch == 1)
	 ff[cc].Fone++;
      ff[cc].Ftot++;
      if ((sw^ch) == 0 ) {
        cc = 2 * cc + 1;
      } else {
        cc = 2 * cc + 2;
      }
      if (cc > 256 ) cc = 0;
      /* cc = 0; */
   }
   out.w(-1);
   return 1;
}

/* ARITHMETIC DECODING ALGORITHM. */
/* START DECODING A STREAM OF SYMBOLS. */

void start_decoding(void)
{
   int             i;
   VALUE = 0;
   ZEND = 0;
   ZATE = 1;
   for (i = 1; i <= Code_value_bits; i++) {	/* code value.              */
      VALUE = 2 * VALUE + input_bit();	/* Move in next input bit.  */
   }
}

/* DECODE THE NEXT SYMBOL. */
int decode_symbol(bij_2c ff)
{
   code_value      range;	/* Size of current code region              */
   int             symbol;	/* Symbol decoded                           */
   int             s1;
   range = (code_value) (high - low) + 1;	/* YEAH NO +1 */
   range = range / ff.Ftot;
   range = range * ff.Fone;
   range = high - range;
   if (VALUE >= range) {
      symbol = 1;
      low = range;
   } else {
      symbol = 0;
      high = range - 1;
   }
   ZATE = ((high < Half) || (low >= Half)) ? 1 : 0;
   s1 = 0;
   for (;;) {			/* Loop to get rid of bits. */
      if (high < Half) {
	 /* nothing *//* Expand low half.         */
      } else if (low >= Half) {	/* Expand high half.        */
	 Zero_av = 0;
	 VALUE -= Half;
	 low -= Half;		/* Subtract offset to top.  */
	 high -= Half;
      } else if (low >= First_qtr	/* Expand middle half.      */
		 && high < Third_qtr) {
	 Zero_av = 0;
	 VALUE -= First_qtr;
	 low -= First_qtr;	/* Subtract offset to middle */
	 high -= First_qtr;
      } else
	 break;			/* Otherwise exit loop.     */
      low = 2 * low;
      high = 2 * high + 1;	/* Scale up code range.     */
      s1 = 1;
      VALUE = 2 * VALUE + input_bit();	/* Move in next input bit.  */
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
   return symbol;
}
