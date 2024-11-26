/*
  ARB3W.CPP  Heres one for the Gipper
  This is loosly based on Mark Nelson arithmetic coder.
  And I feel like a midget standing on the heads of giants
  see ARB3H.CPP for details.

   This is really to show how one could use 2 state bijective
  arithmetic comperession to do string arithmetiv compression
  Not practical just an example. Use at your own risk.


 I demo the 3 state reverse bijective arithmetic 
A = 1
B = 01
C = 11
this reqires to internal nodes A versus BC and B versus C

   David A. Scott  2004 August
*/

#include <stdio.h>
#include <stdlib.h>
#include<math.h>
#if !defined( unix )
#include <io.h>
#endif
#include <fcntl.h>
#include "bit_byts.h"


struct bij_2c {
   unsigned long   Fone;
   unsigned long   Ftot;
};

bij_2c          ffavbc,ffbvc;

/* ADAPTIVE SOURCE MODEL */


/* DECLARATIONS USED FOR ARITHMETIC ENCODING AND DECODING */

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
 */
code_value freeend;		/* current free end */
code_value      fcount;
int             CMOD = 0;
int             FRX = 0;
int             FRXX = 0;

void            encode_symbol(int, bij_2c);


/* THE BIT BUFFER. */


bit_byts        out;
bit_byts        in;


/****** . ********/
/* out.wz or out.ws */
#define dasr in.r()
// #define dasw out.ws
#define dasw out.wzc
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



int
get_value(void) {
/*
routine uses two nodes which is n - 1 number of symbols
since n is 3 number of nodes is 2.
let for the long run
A = 1
B = 01
C = 00  note  a huffman like tree
next tree when symbol follow A B or C is EOS
A = -1    CA  001 -1
B = 0 -1
C = 0 0 -1  
which for general case is last 1 replaced with -1
if symbol is the least likely symbol add -1 at end
*/
static int value = 0;
static int flag = 0;
static int ch = 0;
static int chold = 0;
if ( value < 0 ) {
   ch = value;
   value = -2;
   return ch;
}
if ( value == 0 ) {
   chold = getchar();
   if ( chold != 'A' && chold  != 'B' && chold != 'C') {
       value = -2;
       return -2;
   }
   ch = getchar();
   if ( ch != 'A' && ch  != 'B' && ch != 'C') ch = -1; 
   if (chold != 'A') {
     value = 2;
     return 0;
   }
   if ( ch != -1 ) {
     value = 1;
     return 1;
   }
   value = -2; 
   return -1;  
}
if ( value == 2 ) {
  if (ch != -1) {  /*** continue here ***/
     value = 1;
     flag = 0;
     if ( chold == 'B' ) return 1;
     flag = 1;
     return 0;
  } else {
     if ( chold == 'B') return -1;
     value = -1; 
     return 0;
  }
}
if ( value == 1 ) {
   chold = ch;
   ch = getchar();
   if ( ch != 'A' && ch  != 'B' && ch != 'C') ch = -1; 
   if ( chold == 'A' ) {
      if ( ch != -1 )  return 1; 
      else {
         if ( flag == 0 ) return -1;
      } 
      value = -1;
      return 1;
   }
   value = 2;
}
return 0;
}

unsigned long long b2,b3;
double p1,p2,p3;
double l1,l2,l3;
double lave;
unsigned long w1,w2,w3;

void firstp(){
double t;
t = (double) w1 + w2 + w3;
p1 = w1 / t;
p2 = w2 / t;
p3 = w3 / t;
}
void nextp(){
double t;
t = p1 + p2 + p3;
p1 = p1 / t;
p2 = p2 / t;
p3 = p3 / t;
}


int
main(int argc, char *argv[])
{
   int             ch;
   int             ticker = 0;
   FILE *fc;
   fprintf(stderr, "Bijective Arithmetic 2 state coding version 20040723 \n ");
   fprintf(stderr, "Arithmetic Compressor on ");
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
   fc = NULL;
   if (argc > 3) {
      fc = fopen(argv[3], "rb");
      fprintf(stderr, " wieghts from ");
      fprintf(stderr, "%s", argv[3]);
   } else
   fprintf(stderr, "\n");
#if !defined( unix )
   setmode(fileno(stdin), O_BINARY);
   setmode(fileno(stdout), O_BINARY);
#endif
if ( fc == NULL ) {
fprintf(stderr,"\n No weight file assuming all one ");
w1 = 1;
w2 = 1;
w3 = 1;
} else {
fscanf( fc,"%ld",&w1);
fscanf( fc,"%ld",&w2);
fscanf( fc,"%ld",&w3);
}
if ( w1 > w2 || w2 > w3 ) {
  fprintf(stderr,"\n bad weigthts try again later ERRORING OFF ");
  exit(0);
}
fprintf(stderr,"\n here is w1 = %ld",w1);
fprintf(stderr," here is w2 = %ld",w2);
fprintf(stderr," here is w3 = %ld",w3);
firstp();
for( int i = 100; i-- >0; ){
p1 = pow( p2,(double)w1/w2);
p3 = pow( p2,(double)w3/w2);
nextp();
}
fprintf(stderr,"\n probabilites p1 = %f p2 = %f p3 = %f",p1,p2,p3);
l1 = log(1.0/p1)/log(2);
l2 = log(1.0/p2)/log(2);
l3 = log(1.0/p3)/log(2);
fprintf(stderr,"\n lengths l1 = %f l2 = %f l3 = %f",l1,l2,l3);
lave = (l1 + l2 + l3 )/3;
fprintf(stderr,"\n  average of three = %f",lave);
fprintf(stderr,"\n lengths/weight l1 = %f l2 = %f l3 = %f",l1/w1,l2/w2,l3/w3);
fprintf(stderr,"\n if 3 values above not close to same a bad fit ");
b2 = (unsigned long long) (p1 * 2000000llu);
b3 = (unsigned long long) (p2/(p2 + p3) * 2000000llu);
b2 += 1;
b3 += 1;
b2 >>= 1;
b3 >>= 1;
//printf("\n cell 1 tot = 1000000 Fone = %llu",b2);
//printf("\n cell 2 tot = 1000000 Fone = %llu",b3);

   out.iw(stdout);
   low = 0;
//   ffavbc.Ftot = 2000;	/* both set up for 50/50 split */
//   ffavbc.Fone = 1000;
//   ffbvc.Ftot = 2000;
//   ffbvc.Fone = 1000;
   ffavbc.Ftot = 1000000;  /* here A B C all have same wieght */
   ffavbc.Fone =  b2;  /* the only difference is weight otherwise */
   ffbvc.Ftot = 1000000;   /* code exactly same as huffman */
   ffbvc.Fone =  b3;
   freeend = Half;
   fcount = 1;
   high = Top_value;
   bits_to_follow = 0;		/* No bits to follow next.  */
   for (;;) {			/* Loop through characters. */
      ch = get_value();
      if ((ticker++ % 1024) == 0)
	 putc('.', stderr);
      if (ch < 0)
	 break; /*  */
      encode_symbol(ch, ffavbc);
      if ( ch == 1 ) continue;
      ch = get_value();
      if (ch < 0)
         break;
      encode_symbol(ch, ffbvc);
     }
   if ( ch == -2 ) {
     fprintf(stderr,"\n *** FATAL ERROR *** ");
     fprintf(stderr,"\n file most start with A B or C ");
     return 3;
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
