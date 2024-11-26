struct bit_bytn {
   public:
   bit_bytn() {
      xx();
   }
   int             ir(FILE * fr) {	/* open file for bit read FOF assumed */
      CHK();
      inuse = 0x01;
      f = fr;
      bn = getc(f);
      if (bn == EOF) {
	 fprintf(stderr, " empty file in bit_bytn \n");
	 abort();
      }
   }
   int             irr(FILE * frr) {	/* open and read first bit */
      ir(frr);
      return r();
   }
   int             r();		/* get next bit */
   int             iw(FILE * fw) {	/* open file for bit write FOF
					 * assumed */
      CHK();
      inuse = 0x02;
      f = fw;
   }
   int             iww(FILE * fww, int b) {	/* open and read first bit */
      iw(fww);
      return w(b);
   }
   int             w(int);	/* get next bit */
   int             status() {
      return inuse;
   }
   /* on read 0 if normal -1 if last bit -2 there after */
   /* on write -1 if current is last -2 if call after last */
                   private:
   int             CHK() {
      if (inuse != 0x69) {
	 fprintf(stderr, " all read in use bit_bytn use error %x \n", inuse);
	 abort();
      }
   }
   int             xx() {
      inuse = 0x69;
      M = 0x80;
      l = 0;
      f = NULL;
      bn = 0;
      bo = 0;
      zerf = 0;
      onef = 0;
      zc = 0;
   }
   int             inuse;
   int             zerf;
   int             onef;
   int             bn;
   int             bo;
   int             l;
   int             M;
   long            zc;
   FILE           *f;
};
