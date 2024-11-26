 Palace this code in a directory by itself. Use at
your own risk.
arib.cpp is a bijective replace meant to Marks ARI
unarib.cpp is a bijective replace meant to Marks UNARI
arib32.cpp is like arib but use 32 bit window instead of 16 
unarib32.cpp is like unarib but use 32 bit window instead of 16 
zeros.bat is a test file.
zeros.dat is a data file.
ari    expands the compression challenge by 3,458 bytes
arib   expands the compression challenge by 3,301 bytes
arib32 expands the compression challenge by 201 bytes
Due to nature of compression the 32 bits gains very little
in terms of space. But it may be useful in compression before
encryption since not only bijective it smears the effects of
data over a larger window in a complex way so a large amount
of work needed to recover what was compressed and then encrypted.
one could even make a poor mans pre whitener before encryption
by compressing with it and then reverse the file and compress
again. This way if any byte in input file changes there is a
good chance the whole file affected and to even test a key
one needs to decrypt all the blocks and do a full pass of the
decompression.
the precomplied exe all done on a DjGPP port of GNU
Good compressing and encrypting
David Scott


