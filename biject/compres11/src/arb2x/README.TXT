** changed 2006 06 02
  changed certain types from LONG to LONG LONG
  and write less to screen

 This code was meant to be a full model of the optimal
2 state bijective compressor decompressor with optimal EOS
handling so one can easily write true bijective file or
string compressors.
 
   There are many ways to end the 2 state arithmetically
so that its bijective. However if one keeps true to the
theory of arithmetic compresstion. The free end method
of Matt Timmermans seems to be the best in that every time
a new symbol is compressed if the file ends it would slowly
grow. In my methods where you add the expanding symbol then
you can effectively limit it to "free ends" of zero one or
two bits plus the length of expanding symbol which could in
certain cases be quite long. I don't think this is exactly
how Matt would have done it so I call it a modifed free end
method.

   The  code is robust enough so the interval can be chopped
up in random ways from call to call. The code is designed
to work on high low registers from "1" bit  to "64" bits.
Note this means at one bit 
First_qtr = 0 Half = 1 Third_qtr = 1 Top_value = 1
for two bits
First_qtr = 0 Half = 2 Third_qtr = 3 Top_value = 3
for three bits
First_qtr = 0 Half = 2 Third_qtr = 3 Top_value = 4
the code does not test for the number of bits.
However certain things like FRX where extended free
ends are used are most likely not needed for the
64 bit case where 2**64 states for free ends are
available.

  This code is again only to show the basic single
2 state coder. It would only cause compresssion
on a file or string of the ratio 1's tp 0's
is extreme. And no compression is the number of ones
equals the number of zeros.  Note my no compression
the length could still change one or two bits due
to the bijective string to bytes transforms.

Here is the how would could compress pure strings.
Where ratio is large.
BYTE2STR filex file1  /* convert ascii one zero to binary */
ARB2X file1 file2    /* compress file1 bijective compression to file2 */
STR2BYTE file2 filey   /* convert file2 back to an ascii one zero bytes */
 see bit_bij.zip for the str2byte and byte2str programs.

if input 64 bytes:
0000000000000000000000000000000000000000000000000000000000000000 
following result if "1" bit width high low used virtually no compression
0110000011010011110011110110011001011111101011010111000000010010 
following with "2" bit high lows some compression
101000001101001111001111011001100 
same with "3" bit widths more compression
100111110010110000 
same with "4" 
100111110000"ZZ" 
same with "62"
100011100 
same with "63"
100011100 
same with "64"
100011100 

Note at higher bit widths this simple example gets the same
strings This is not what happens when the input is longer
and radomly changing. In fact after several bits there is
almost no corrolation between the 64 and 63 bit file output
however there compressed lengths would be about the same. 
start change values.
   See the new ARB255 which is new version using this improved
2 state cell with 255 nodes it compress most files slightly
better and its less apt to fail. I never got the old one to
fail by I am sure that for at least 1 out of 2**42 files it
would fail. I don't think the new one will fail. It more
solid.

