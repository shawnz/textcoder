This directory includes a MS VC6 project file, biacode.dsp.

If you're using some other compiler under windows, compile as a Win32 console
executable.

test.bat is provided to test the bijective properties of the encoder under Win32.

test <file>

will compress and decompress <file> 4 times in each direction to give files
a1-a9.  a1 is then compressed 8 times to give b1-b9.  b9 is then decompressed
8 times to give c1-c9.  fc /b is then used to ensure that ai==bi==ci for all
i in [0,9]
