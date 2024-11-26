#include <stdio.h>
#include <stdlib.h>
/*
 this package the fast io byte file to bit stream
 or bit to byte file use "0 0x80 and other" method
 done by David Scott 2001 August
*/

#include "bit_byts.h"

int             bit_byts::
rc()
{				/* FILE of ASCII ones and zeroes only */
   if (f == NULL)
      return -2;
   if (bn == 2) {
      bo = fgetc(f);
      if (bo == (int) '1')
	 return 1;
      if (bo == (int) '0')
	 return 0;
      xx();
      return -1;
   }
   if (bn == (int) '1') {
      bo = fgetc(f);
      if (bo == (int) '1')
	 return 1;		/* one in a string of all ones */
      if (bo == (int) '0') {
	 bn = 1;
	 return 1;
      }
      xx();
      return -1;
   }
   if (bn == (int) '0') {
      bn = 2;
      return 0;
   }
   if (bn == 1) {
      bn = 2;
      return 0;
   }
   return 7;
}

int             bit_byts::
r()
{
   /* return -1 on last bit which is last one in file -2 thereafter */
   /* return 0 on zero return 1 most of the time one */
   if (f == NULL)
      return -2;
   if ((l >>= 1) == 0) {
      l = 0x80;
      bo = bn;
      bn = fgetc(f);
      if (bo == 0)
	 zerf = 1;
      else if (bo != M) {
	 zerf = 0;
	 onef = 0;
      } else if (zerf == 1)
	 onef = 1;
      if (bn == EOF && (onef + zerf) > 0) {
	 onef = 0;
	 zerf = 0;
	 bn = M;
      }
   }
   if ((bo & l) == 0)
      return 0;;
   bo ^= l;
   if (bn != EOF || bo != 0)
      return 1;
   xx();
   return -1;
}

int             bit_byts::
wc(int x)
{
   /* like w() but writes ascii one or zero  returns 0 if ok */
   /* return -1 if sending last byte */
   /* return -2 if closed longer; */
   /* 0 or 1 writes and -1 writes last one -2 means previous 1 was last */

   if (f == NULL)
      return -2;
   if (x == 1) {
      fputc((int) '1', f);
      return 0;
   }
   if (x == 0) {
      fputc((int) '0', f);
      bo = 1;
   }
   if (x == -2)
      bo = 1;
   if (x == -1) {
      if (bo == 0)
	 fputc((int) '1', f);
      xx();
      return x;
   }
   return 0;
}

int             bit_byts::
w(int x)
{
   /* return 0 if ok */
   /* return -1 if sending last byte */
   /* return -2 if closed longer; */
   /* 0 or 1 writes and -1 writes last one -2 means previous 1 was last */

   if (f == NULL)
      return -2;
   if (x == -1) {
      w(1);
      w(-2);
      return -1;
   }
   if (x == -2) {
      if (bo == M && (zerf + onef) == 0)
	 fputc(bo, f);
      if (bo == M || bo == 0) {
	 xx();
	 return -2;
      }
   }
   if ((l >>= 1) == 0) {
      l = 0x80;
   }
   if (x > 0)
      bo ^= l;

   if (l == 1 || x < 0) {
      if (bo == 0)
	 zerf = 1;
      else if (bo != M) {
	 zerf = 0;
	 onef = 0;
      } else if (zerf == 1)
	 onef = 1;
      if (x < 0) {
	 if ((onef + zerf) == 0 || bo != M)
	    fputc(bo, f);
	 xx();
	 return -2;
      } else {
	 fputc(bo, f);
	 bo = 0;
      }
   }
   return 0;
}
