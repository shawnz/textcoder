#include <stdio.h>
#include <stdlib.h>
/*
 this package the numeric faster way io byte file to bit stream
 or bit to byte file. This distorts the file more than bit_byte
 put should be slightly faster and cleaner.
 done by David Scott 2001 August 
*/

#include "bit_bytn.h"

int             bit_bytn::r()
{
   /* return -1 on last bit which is last one in file -2 thereafter */
   /* return 0 on zero return 1 most of the time one */
   /* *** bn has first read in it **** */
   if (f == NULL)
      return -2;
   if ((l >>= 1) == 0) {
      l = 0x80;
      bo = bn;
      bn = fgetc(f);
      bo += onef;
      if (++bo > 0xFF) {
	 bo &= 0xFF;
	 onef = 1;
      } else
	 onef = 0;
      if (bn == EOF && onef == 1) {
	 bn = 0;		/* so one bit for last byte goes at 0x01 */
	 onef = 0;
      }
      
   }
   if ((bo & l) == 0)
      return 0;
   bo ^= l;
   if (bn != EOF || bo != 0)
      return 1;
   xx();
   return -1;
}

int             bit_bytn::w(int x)
{
   /* return 0 if ok */
   /* return -1 if sending last byte */
   /* return -2 if closed longer; */
   /* 0 or 1 writes and -1 wirtes last one -2 means previous 1 was last */

   if (f == NULL)
      return -2;
   if (x == -1) {
      w(1);
      w(-2);
      return -1;
   }
   if (x == -2 && bo == 0x00) {
      xx();
      return -2;
   }
   if ((l >>= 1) == 0) {
      l = 0x80;
   }
   if (x > 0)
      bo ^= l;

   if (l == 1 || x < 0) {
      bo -= onef;
      if (--bo < 0x00) {
	 bo &= 0xFF;
	 onef = 1;
      } else
	 onef = 0;
      for (; zc > 0; zc--) {
	 fputc(0xFF, f);
      }
      if (bo == 0xFF && onef == 1)
	 zc++;
      else
	 fputc(bo, f);
      bo = 0;
      if (x < 0) {
	 xx();
	 return -2;
      }
   }
   return 0;
}
