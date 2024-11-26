/*
 * modifed in December 2003 by David Scott for less expansion in compression
 * decompression programs same abount of data in bit file byte file could
 * change a max of one byte. Still a complete bijective transform but pseudo
 * random bijective transform. rs() and ws()
 */

/*
 * modifed in Qctober 2002 by David Scott for delayed -2 in write state wz()
 * and wzc() routines
 * made better -2 for wzc() August 2004 David Scott
 */
struct bit_byts {
   public:
   bit_byts() {
      xx();
   }
   void            ir(FILE * fr) {	/* open file for bit read FOF assumed */
      CHK();
      inuse = 0x01;
      f = fr;
      bn = getc(f);
      if (bn == EOF) {
	 fprintf(stderr, " empty file in bit_byts \n");
	 abort();
      }
   }
   void            irc(FILE * fr) {	/* open file for 01 read FOF assumed */
      CHK();
      inuse = 0x01;
      f = fr;
      bn = getc(f);
      if ((bn != (int) '1') && (bn != (int) '0')) {
	 fprintf(stderr, " empty file in bit_byts \n");
	 abort();
      }
   }
   int             irr(FILE * frr) {	/* open and read first bit */
      ir(frr);
      return r();
   }
   int             r();		/* get next bit */
   int             rs() {	/* get next bit psuedo random */
      if ((d1r = r()) < 0)
	 return d1r;
      dr = (ax * dr) % bx;
      return (1 & dr ^ d1r);
   }

   int             rc();	/* get next ASCII 1 or 0 */
   void            iw(FILE * fw) {	/* open file for bit write FOF
					 * assumed */
      CHK();
      inuse = 0x02;
      f = fw;
   }
   int             iww(FILE * fww, int b) {	/* open and write first bit */
      iw(fww);
      return w(b);
   }
   int             w(int);	/* write next bit */
   int             wc(int);	/* write next ASCII 1 or 0 */
   int             status() {
      return inuse;
   }

   /* on read 0 if normal -1 if last bit -2 there after */
   /* on write -1 if current is last -2 if call after last one */
   /****  note a big error to issue -2 of no previous 1 ***/
   int             ws(int c) {	/* write bit pusedo random */
      if (c == 0) {
	 d3w++;
	 return 0;
      }
      if (c == 1) {
	 if (d1w == 0) {
	    d1w = 1;
	    d2w = d3w;
	    d3w = 0;
	    return 0;
	 }
	 for (; d2w > 0; d2w--) {
	    dw = (ax * dw) % bx;
	    w(1 & dw);
	 }
	 d2w = d3w;
	 d3w = 0;
	 dw = (ax * dw) % bx;
	 return w(1 ^ (1 & dw));
      }
      if (c == -2) {
	 if (d1w == 0)
	    return w(-1);
	 for (; d2w > 0; d2w--) {
	    dw = (ax * dw) % bx;
	    w(1 & dw);
	 }
	 d1w = 0;
	 return w(-1);
      }
      if (d1w == 0) {
	 for (; d3w > 0; d3w--) {
	    dw = (ax * dw) % bx;
	    w(1 & dw);
	 }
	 return w(-1);
      }
      for (; d2w > 0; d2w--) {
	 dw = (ax * dw) % bx;
	 w(1 & dw);
      }
      dw = (ax * dw) % bx;
      w(1 ^ (1 & dw));
      for (; d3w > 0; d3w--) {
	 dw = (ax * dw) % bx;
	 w(1 & dw);
      }
      d1w = 0;
      return w(-1);
   }

   /* on read 0 if normal -1 if last bit -2 there after */
   /* on write -1 if current is last -2 if call after last one */
   int             wz(int c) {
      if (c == -2)
	 return w(-2);
      if (c == 0) {
	 bn++;
	 return 0;
      } else {
	 for (; bn > 0; bn--)
	    w(0);
	 return w(c);
      }
   }
   int             wzc(int c) {
      if (c == -2) {
         if ( b1 == 1) wc(-1);
	 return 0;
      }
      if (c == 0) {
	 bn++;
	 return 0;
      } else {
         if ( b1 != 0 ) wc(b1);
	 for (; bn > 0; bn--)
	    wc(0);
         b1 = c;
	 if ( c == -1 )return wc(c);
      }
    return 0;
   }
private:
   void            CHK() {
      if (inuse != 0x69) {
	 fprintf(stderr, " all read in use bit_byts use error %x \n", inuse);
	 abort();
      }
   }
   void            xx() {
      inuse = 0x69;
      M = 0x80;
      l = 0;
      f = NULL;
      bo = 0;
      b1 = 0;
      bn = 0;
      zerf = 0;
      onef = 0;
      zc = 0;

      dw = 1;
      d1w = 0;
      d2w = 0;
      d3w = 0;
      d1r = 0;
      dr = 1;
      bx = 0x7fffffff;
      ax = 16807;
   }
   long long       bx;
   long long       ax;
   int             dw;
   int             dr;
   int             d1r;
   int             d1w;
   int             d2w;
   int             d3w;
   int             inuse;
   int             zerf;
   int             onef;
   int             bn;
   int             b1;
   int             bo;
   int             l;
   int             M;
   long            zc;
   FILE           *f;
};
