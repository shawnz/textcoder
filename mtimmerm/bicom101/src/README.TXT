===========================================================================
BICOM 1.01 bijective compressor
===========================================================================
Copyright (C) 2000 Matt Timmermans

Free for non-commercial purposes as long as this notice remains intact.
To discuss commercial use, mail me at matt@timmermans.org
===========================================================================

EXECUTION

Run bicom without arguments to get help on the command line.

===========================================================================

BUILDING

The two directories include MS VC6 project files.  bicom\bicom.dsw is the 
main workspace.

If you're using some other compiler, just compile all the .cpp files in
bialib as a static library, and compile bicom.cpp as a normal console app
that links with it.

This code should work on most platforms, but has only been tested in Win32.

===========================================================================

TESTING

bitest.bat is provided to test the bijective properties of the encoder
under Win32.

bitest <file>

will compress and decompress <file> 4 times in each direction to give files
a1-a9.  a1 is then compressed 8 times to give b1-b9.  b9 is then decompressed
8 times to give c1-c9.  fc /b is then used to ensure that ai==bi==ci for all
i in [0,9]

bitest <file> -p<password>

will do the same thing, but with encryption turned on.

=========
!WARNING! Arbitrary files can get quite big when you repeatedly
========= decompress them!


If you run bicom -T, it will test bijectivity on all small files.  A
passphrase can be supplied.  It prints out periods as it goes, with one
period == 256 test files. (each test file is processed 4 times, in different
ways)  This is very slow, because each run needs a fair bit of setup work.

