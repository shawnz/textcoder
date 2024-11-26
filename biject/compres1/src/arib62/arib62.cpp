//
//  ARIB62.CPP
//
//  Mark Nelson
//  March 8, 1996
//  http://web2.airmail.net/markn
//  **** moded by David Scott in may of 2001
//  to make bijective so it will be suited
//  
//
// DESCRIPTION
// -----------
//
//  This program performs an order-0 adaptive arithmetic encoding
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
//  file system into binary mode.  g++ already does this, and your
//  UNIX C++ compiler might also.
//
//  Borland C++ 4.5 16 bit    : bcc -w ari.cpp
//  Borland C++ 4.5 32 bit    : bcc32 -w ari.cpp
//  Microsoft Visual C++ 1.52 : cl /W4 ari.cpp
//  Microsoft Visual C++ 2.1  : cl /W4 ari.cpp
//  g++                       : g++ -o ari ari.cpp
//
// Typical Use
// -----------
//
//  rle < raw-file | bwt | mtf | rle | ari > compressed-file
//
//

#include <stdio.h>
#include <stdlib.h>
#if !defined( unix )
#include <io.h>
#endif
#include <fcntl.h>

#define No_of_chars 256                 /* Number of character symbols      */
//#define EOF_symbol (No_of_chars+1)      /* Index of EOF symbol            */
// above line commented out since its not needed

//#define No_of_symbols (No_of_chars +1 ) /* Total number of symbols       */
#define No_of_symbols (No_of_chars )   /* Total number of symbols          */
// The number of symbols is 256 not 257 EOF is not needed
/* TRANSLATION TABLES BETWEEN CHARACTERS AND SYMBOL INDEXES. */

int char_to_index[No_of_chars];         /* To index from character          */
unsigned char index_to_char[No_of_symbols+1]; /* To character from index    */

/* ADAPTIVE SOURCE MODEL */

unsigned long long freq[No_of_symbols+1];  /* Symbol frequencies                    */
unsigned long long cum_freq[No_of_symbols+1]; /*Cumulative symbol frequencies*/
// only 1 to 256 used for the symbols

/* DECLARATIONS USED FOR ARITHMETIC ENCODING AND DECODING */

/* SIZE OF ARITHMETIC CODE VALUES. */

#define Code_value_bits 62              /* Number of bits in a code value   */
typedef unsigned long long code_value;  /* Type of an arithmetic code value */

#define Top_value (((long long)1<<Code_value_bits)-1) /*Largest code value*/


/* HALF AND QUARTER POINTS IN THE CODE VALUE RANGE. */

#define First_qtr (Top_value/4+1)       /* Point after first quarter        */
#define Half	  (2*First_qtr)            /* Point after first half           */
#define Third_qtr (3*First_qtr)         /* Point after third quarter        */


/* CUMULATIVE FREQUENCY TABLE. */




void start_model( void );
void start_encoding( void );
void encode_symbol(int symbol,code_value cum_freq[] );
void update_model(int symbol);
void done_encoding( void );
void done_outputing_bits( void );

/* BIT OUTPUT ROUTINES. */

/* THE BIT BUFFER. */

int buffer;                     /* Bits buffered for output                 */
int bits_to_go;                 /* Number of bits free in buffer            */
int zbuffer;                    /* Number of zero bytes buffered up         */
int lobuffer;                   /* one if z lo tail in effect */


/* INITIALIZE FOR BIT OUTPUT. */

void start_outputing_bits( void )
{   buffer = 0;                                 /* Buffer is empty to start */
    bits_to_go= 8;                              /* with.                    */
    zbuffer = 0;            /* number of zero bytes buffered up */
    lobuffer = 0;           /* one if possible cuttoff */
}

/* OUTPUT A BIT. */

inline void output_bit( int bit )
{   buffer >>= 1; if (bit) buffer |= 0x80;      /* Put bit in top of buffer.*/
    bits_to_go -= 1;
    if (bits_to_go==0) {                        /* Output buffer if it is   */
        if ( buffer != 0 ) {
          if ( lobuffer != 0 ) putc( 0x40, stdout);
          if ( zbuffer > 0 && buffer == 0x40 ) lobuffer = 1; 
          if ( buffer != 0x40) lobuffer = 0; /* 0x40 just because its cool */
          for ( ;zbuffer > 0; zbuffer--) putc( 0, stdout);
          if ( lobuffer != 1)putc( (char) buffer, stdout );  
        }
        else zbuffer++;
        bits_to_go = 8;
        buffer = 0;
    }
}

/* FLUSH OUT THE LAST BITS. */

void done_outputing_bits( void )
{  /* write out remaining bits most of the time */
   if ( buffer != 0 )  {
       if ( bits_to_go != 0 ) buffer >>= bits_to_go;
       if ( lobuffer != 0 ) putc( 0x40, stdout);
       if ( zbuffer > 0 && buffer == 0x40 ) lobuffer = 1; 
       if ( buffer != 0x40) lobuffer = 0;
       for ( ;zbuffer > 0; zbuffer--) putc( 0, stdout);
       if ( lobuffer != 1)putc( (char) buffer, stdout );  
   }
}

/* CURRENT STATE OF THE ENCODING. */

code_value low, high, swape;     /* Ends of the current code region          */
code_value bits_to_follow;       /* Number of opposite bits to output after  */
                                /* the next bit.                            */
/* OUTPUT BITS PLUS FOLLOWING OPPOSITE BITS. */

void bit_plus_follow( int bit )
{   output_bit(bit);                            /* Output the bit.          */
    while (bits_to_follow>0) {
        output_bit(!bit);                       /* Output bits_to_follow    */
        bits_to_follow -= 1;                    /* opposite bits. Set       */
    }                                           /* bits_to_follow to zero.  */
}

int main( int argc, char *argv[] )
{
    int ticker = 0;
    int zerf = 0;
    int onef = 0;

    fprintf( stderr, "Bijective Arithmetic coding version May 30, 2001 \n " );
    fprintf( stderr, "Arithmetic coding on " );
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
    start_outputing_bits();
    start_encoding();
    for (;;) {                                  /* Loop through characters. */
        int ch; int symbol;
        if ( ( ticker++ % 1024 ) == 0 )
            putc( '.', stderr );
        ch = getc(stdin);                       /* Read the next character. */
        if (ch==EOF) break;                     /* Exit loop on end-of-file.*/
        symbol = char_to_index[ch];             /* Translate to an index.   */
        if ( symbol == 1 ) zerf = 1;
        if ( zerf == 1 && symbol == 2) onef = 1;
        if ( symbol > 1 ) zerf = 0; 
        if ( symbol > 2 ) onef = 0; 
        encode_symbol(symbol,cum_freq);         /* Encode that symbol.      */
        update_model(symbol);                   /* Update the model.        */
    }
    if ( ( zerf + onef) > 0 ) {
      encode_symbol ( 2, cum_freq);
      update_model( 2);
    }


//    encode_symbol(EOF_symbol,cum_freq);         /* Encode the EOF symbol.   */
    done_encoding();                            /* Send the last few bits.  */
    done_outputing_bits();
    putc( '\n', stderr );
    return 0;
}

/* ARITHMETIC ENCODING ALGORITHM. */


/* START ENCODING A STREAM OF SYMBOLS. */

void start_encoding( void )
{   low = 0;                                    /* Full code range.         */
    high = Top_value;
    bits_to_follow = 0;                         /* No bits to follow next.  */
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
    
/* ENCODE A SYMBOL. */

void encode_symbol(int symbol,code_value cum_freq[] )
{   code_value range;           /* Size of the current code region          */
    range = (code_value)(high-low)+1;
    high = low   +  dss(range,cum_freq[symbol-1],cum_freq[0]) -1;
//      (range*cum_freq[symbol-1])/cum_freq[0]-1; 
    low = low +   dss( range, cum_freq[symbol],cum_freq[0]);
//      (range*cum_freq[symbol])/cum_freq[0];

     low ^= Top_value; /* mapping so largest segment at bottom */
     high ^= Top_value; /* key is so symbol 2 will have at least */
     swape = low;  /* one "one bit" so bijective at end */
     low = high;
     high = swape;

    for (;;) {                                  /* Loop to output bits.     */
        if (high<Half) {
            bit_plus_follow(0);                 /* Output 0 if in low half. */
        }
        else if (low>=Half) {                   /* Output 1 if in high half.*/
            bit_plus_follow(1);
            low -= Half;
            high -= Half;                       /* Subtract offset to top.  */
        }
        else if (low>=First_qtr                 /* Output an opposite bit   */
              && high<Third_qtr) {              /* later if in middle half. */
            bits_to_follow += 1;
            low -= First_qtr;                   /* Subtract offset to middle*/
            high -= First_qtr;
        }
        else break;                             /* Otherwise exit loop.     */
        low = 2*low;
        high = 2*high+1;                        /* Scale up code range.     */
    }
    low ^= Top_value;
    high ^= Top_value;
    swape = low;
    low = high;
    high = swape;
}


/* FINISH ENCODING THE STREAM. */

void done_encoding( void )
{   if (bits_to_follow > 0 ) { /* saving at least one bit here */
       bit_plus_follow(1);
     } else if ( high != Top_value ) bit_plus_follow(1);
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
        index_to_char[i] = (unsigned char) ch_symbol;           /* moved.                   */
        index_to_char[symbol] = (unsigned char) ch_i;
        char_to_index[ch_i] = symbol;
        char_to_index[ch_symbol] = i;
    }
    freq[i] ++;                               /* Increment the frequency  */
    while (i>0) {                               /* count for the symbol and */
        i -= 1;                                 /* update the cumulative    */
        cum_freq[i] ++;                       /* frequencies.             */
    }
}
