 This is hopefully like David Carrys reverse huffman idea
except using a bijective arithmetic.
 The code is a slightly modifed ARB255 with the 3 symbol
case. Two symbols is trival and more than 3 can be done
with slight changes to this file. It just seems that for
three symbols is the first interesting case.
 The reverse problem is like this. Suppose you have the
reduced set of binary messages but you have to replace
them with an equivalent set of messages in another 
symbols set where there is a cost wieght assocaiated
with each symbol. The problem is to find the lowest cost
for the equivalent message.
 See http://bijective.dogma.net/compres12.htm for more
details.
*** USE at you own risk ***

unarb3w infile outfile weightfile

infile is file on only ascii "1"s and "0"s
outfile will be the file of only "A"s "B"s and "C"s
as determined by 3 line ascii realative weight file.

unarb2w file.in file.out weight122.txt
should create a output file that mathes file
file.ABC
