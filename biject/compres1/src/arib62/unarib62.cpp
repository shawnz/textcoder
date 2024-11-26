//
//  UNARIB62.CPP
//
//  Mark Nelson
//  March 8, 1996
//  http://web2.airmail.net/markn
//  **** moded by David Scott in may of 2001
//  to make bijective so it will be suited
//
// DESCRIPTION
// -----------
//
//  This program performs an order-0 adaptive arithmetic decoding
//  function on an input file/stream, and sends the result to an
//  output file or stream.
//
//  This program contains the source code from the 1987 CACM
//  article by Witten, Neal, and Cleary.  I have taken the
//  source modules and combined them into this single file for
//  ease of distribution and compilation.  Other than that,
//  the code is essentially unchanged.
//
//  This program takes two arguments: an input file and an output
//  file.  You can leave off one argument and send your output to
//  stdout.  Leave off two arguments and read your input from stdin
//  as well.
//
//  This program accompanies my article "Data Compression with the
//  Burrows-Wheeler Transform."
//
// Build Instructions
// ------------------
//
//  Define the constant unix for UNIX or UNIX-like systems.  The
//  use of this constant turns off the code used to force the MS-DOS
//  file system into binary mode.  g++ does this already, your UNIX
//  C++ compiler might also.
//
//  Borland C++ 4.5 16 bit    : bcc -w unari.cpp
//  Borland C++ 4.5 32 bit    : bcc32 -w unari.cpp
//  Microsoft Visual C++ 1.52 : cl /W4 unari.cpp
//  Microsoft Visual C++ 2.1  : cl /W4 unari.cpp
//  g++                       : g++ -o unari unari.cpp
//
// Typical Use
// -----------
//
//  unari < compressed-file | unrle | unmtf | unbwt | unrle > raw-file
//
//
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#if !defined( unix )
#include <io.h>
#endif

/* THE SET OF SYMBOLS THAT MAY BE ENCODED. */

#define No_of_chars 256                 /* Number of character symbols      */
//#define EOF_symbol (No_of_chars+1)      /* Index of EOF symbol              */
// above line commented out since its not needed

//#define No_of_symbols (No_of_chars +1)  /* Total number of symbols        */
#define No_of_symbols (No_of_chars)   /* Total number of symbols          */
// The number of symbols is 256 not 257 EOF is not needed
/* TRANSLATION TABLES BETWEEN CHARACTERS AND SYMBOL INDEXES. */


int char_to_index[No_of_chars];         /* To index from character          */
unsigned char index_to_char[No_of_symbols+1]; /* To character from index    */

unsigned long long freq[No_of_symbols+1];      /* Symbol frequencies                       */
unsigned long long cum_freq[No_of_symbols+1];  /* Cumulative symbol frequencies            */
// only 1 to 256 used for the symbols

/* CUMULATIVE FREQUENCY TABLE. */


/* DECLARATIONS USED FOR ARITHMETIC ENCODING AND DECODING */


/* SIZE OF ARITHMETIC CODE VALUES. */

#define Code_value_bits 62              /* Number of bits in a code value   */
typedef unsigned long long code_value;    /* Type of an arithmetic code value */

#define Top_value (((code_value)1<<Code_value_bits)-1)  /* Largest code value */


/* HALF AND QUARTER POINTS IN THE CODE VALUE RANGE. */

#define First_qtr (Top_value/4+1)       /* Point after first quarter        */
#define Half      (2*First_qtr)         /* Point after first half           */
#define Third_qtr (3*First_qtr)         /* Point after third quarter        */


/* BIT INPUT ROUTINES. */

/* THE BIT BUFFER. */

int buffer;                     /* Bits waiting to be input                 */
int bufferx;                     /* Bits waiting to be input                 */
int bits_to_go;                 /* Number of bits still in buffer           */
int garbage_bits;               /* Number of bits past end-of-file          */
int ZEND;   /* next 2 flags to tell when last one bit in converted to FOF */
int ZATE;   /* has just been processed. ***NOT always last one bin file** */
int zerf;   /* Next 2 flags for eof of file handling */
int onef;
int zbuffer;  /* nimber of zero blocks */
int lobuffer;  /* specail handle of input flag */
int last_byte; /* one when last byte of input read  */


/* INITIALIZE BIT INPUT. */

void start_inputing_bits( void )
{   bits_to_go = 0;                             /* Buffer starts out with   */
    garbage_bits = 0;                           /* no bits in it.           */
    ZEND = 0;
    ZATE = 1;
    zerf = 0;
    onef = 0;
    zbuffer = 0;
    lobuffer = 0;
    bufferx = getc(stdin);
    if ( bufferx == EOF ) {
      fprintf(stderr, " **zero input file fatal** \n");
      exit(1);
    }
}


/* INPUT A BIT. */

inline int input_bit( void )
{   int t;
    if (bits_to_go==0) {                        /* Read the next byte if no */
        buffer = bufferx;
        if ( buffer != 0x40 ) lobuffer = 0;
        if ( buffer == 0x40 && zbuffer == 1) lobuffer = 1;
        if ( buffer == 0 ) zbuffer = 1;
        else zbuffer = 0;
        bufferx = getc(stdin);                   /* bits are left in buffer. */
        if (bufferx == EOF) {
           if ( zbuffer == 1 || lobuffer == 1) { /* add in last one bit */
             bufferx = 0x40;
             zbuffer = 0;
             lobuffer = 0;
           } else last_byte = 1;
        }
        if (buffer == EOF) {
            buffer = 0;
            garbage_bits += 1;                      /* Return arbitrary bits*/
            if (garbage_bits>Code_value_bits-2) {   /* after eof, but check */
                fprintf(stderr,"**Bad  should not occurr** \n"); 
                exit(-1);
            }
        }
        bits_to_go = 8;
    }
    t = buffer&1;                               /* Return the next bit from */
    buffer >>= 1;                               /* the bottom of the byte.  */
    bits_to_go -= 1;
    if ( buffer == 0 && last_byte == 1 ) ZEND = 1; /* last one sent */
    return t;
}


void start_model( void );
void start_decoding( void );
int decode_symbol( code_value cum_freq[] );
void update_model( int symbol );

int main( int argc, char *argv[] )
{
    int ticker = 0;
    int symbol;

    fprintf( stderr, "Bijective UNArithmetic coding version May 30, 2001 \n " );
    fprintf( stderr, "Arithmetic decoding on " );
    if ( argc > 1 ) {
        freopen( argv[ 1 ], "rb", stdin );
        fprintf( stderr, "%s", argv[ 1 ] );
    } else
        fprintf( stderr, "stdin" );
    fprintf( stderr, " to " );
    if ( argc > 2 ) {
        freopen( argv[ 2 ], "wb", stdout );
        fprintf( stderr, "%s", argv[ 2 ] );
    } else
        fprintf( stderr, "stdout" );
    fprintf( stderr, "\n" );
#if !defined( unix )
    setmode( fileno( stdin ), O_BINARY );
    setmode( fileno( stdout ), O_BINARY );
#endif

    start_model();                              /* Set up other modules.    */
    start_inputing_bits();
    start_decoding();
    for (;;) {                                  /* Loop through characters. */
        int ch;
        symbol = decode_symbol(cum_freq);       /* Decode next symbol.      */
        if ( symbol == 1 ) zerf = 1;
        if ( zerf == 1 && symbol == 2) onef = 1;
        if ( symbol > 1 ) zerf = 0; 
        if ( symbol > 2 ) onef = 0; 
//        if (symbol==EOF_symbol) break;          /* Exit loop if EOF symbol. */
        ch = index_to_char[symbol];             /* Translate to a character.*/
        if ( ZATE == 0 && symbol != 1) break;
        if ( ( ticker++ % 1024 ) == 0 )
            putc( '.', stderr );
        putc((char) ch,stdout);                 /* Write that character.    */
        update_model(symbol);                   /* Update the model.        */
    }
    if ( (zerf + onef) == 0 ) putc((char) index_to_char[symbol],stdout);
    return 1;
}

/* ARITHMETIC DECODING ALGORITHM. */

/* CURRENT STATE OF THE DECODING. */

static code_value value;        /* Currently-seen code value                */
static code_value low, high, swape;    /* Ends of current code region              */

/* START DECODING A STREAM OF SYMBOLS. */

void start_decoding( void )
{   int i;
    value = 0;                                  /* Input bits to fill the   */
    zbuffer = 0;
    lobuffer = 0;
    last_byte = 0;
    for (i = 1; i<=Code_value_bits; i++) {      /* code value.              */
        value = 2*value+input_bit();
        if ( ZEND && ZATE ) {
          if (ZATE++ > Code_value_bits-0 ) ZATE = 0;
        }
    }
    low = 0;                                    /* Full code range.         */
    high = Top_value;
}

code_value dss(code_value r,code_value top,code_value bot){
    code_value t1,t2; /* r could be 64 bits top and bot 32 bits */
                  /* top < bot */
    if ( bot == 0 ) fprintf(stderr," not possible \n");
    t1 = r/bot;
    t2 = r - ( t1*bot);
    t1 = t1*top;
    t2 = t2*top;
    t2 = t2/bot;
    return (t1 + t2);
   /* return r*top/bot; */
}
    
/* DECODE THE NEXT SYMBOL. */

int decode_symbol( code_value cum_freq[] )
{   code_value range;            /* Size of current code region              */
    int symbol;                 /* Symbol decoded                           */
/**/
    low ^= Top_value;
    high ^= Top_value;
    value ^= Top_value;
    swape = low;
    low = high;
    high = swape;
/**/
    range = (code_value)(high-low)+1;
    high = low + range;
    symbol = 0;
    while(value < high)high = low + dss(range,cum_freq[++symbol],cum_freq[0]);
    swape = high;
    high = low + dss(range,cum_freq[symbol-1],cum_freq[0]) -1;
    low = swape;
    

    low ^= Top_value;
    high ^= Top_value;
    swape = low;
    value ^= Top_value;
    low = high;
    high = swape;


    for (;;) {                                  /* Loop to get rid of bits. */
        if (high<Half) {
            /* nothing */                       /* Expand low half.         */
        }
        else if (low>=Half) {                   /* Expand high half.        */
            value -= Half;
            low -= Half;                        /* Subtract offset to top.  */
            high -= Half;
        }
        else if (low>=First_qtr                 /* Expand middle half.      */
              && high<Third_qtr) {
            value -= First_qtr;
            low -= First_qtr;                   /* Subtract offset to middle*/
            high -= First_qtr;
        }
        else break;                             /* Otherwise exit loop.     */
        low = 2*low;
        high = 2*high+1;                        /* Scale up code range.     */
        value = 2*value+input_bit();            /* Move in next input bit.  */
        if ( ZEND && ZATE ) {
          if (ZATE++ > Code_value_bits-0 ) ZATE = 0;
        }
        if ( ZEND && ZATE && value == 0 ) ZATE = 0;
        if ( ZEND && ZATE && value == Half ) ZATE = 0;

    }
    return symbol;
}

/* THE ADAPTIVE SOURCE MODEL */

/* INITIALIZE THE MODEL. */

void start_model( void )
{   int i;
    for (i = 0; i<No_of_chars; i++) {           /* Set up tables that       */
        char_to_index[i] = i+1;                 /* translate between symbol */
        index_to_char[i+1] = (unsigned char) i; /* indexes and characters.  */
    }
    for (i = 0; i<=No_of_symbols; i++) {        /* Set up initial frequency */
        freq[i] = 1;                            /* counts to be one for all */
        cum_freq[i] = No_of_symbols-i;          /* symbols.                 */
    }
    freq[0] = 0;                                /* Freq[0] must not be the  */
}                                               /* same as freq[1].         */


/* UPDATE THE MODEL TO ACCOUNT FOR A NEW SYMBOL. */

void update_model( int symbol )
{   int i;                      /* New index for symbol                     */
    for (i = symbol; freq[i]==freq[i-1]; i--) ; /* Find symbol's new index. */
    if (i<symbol) {
        int ch_i, ch_symbol;
        ch_i = index_to_char[i];                /* Update the translation   */
        ch_symbol = index_to_char[symbol];      /* tables if the symbol has */
        index_to_char[i] = (unsigned char) ch_symbol; /* moved.             */
        index_to_char[symbol] = (unsigned char) ch_i;
        char_to_index[ch_i] = symbol;
        char_to_index[ch_symbol] = i;
    }
    freq[i] += 1;                               /* Increment the frequency  */
    while (i>0) {                               /* count for the symbol and */
        i -= 1;                                 /* update the cumulative    */
        cum_freq[i] += 1;                       /* frequencies.             */
    }
}
