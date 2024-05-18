copy %1 a5
bicom %2 a5 a6
bicom %2 a6 a7
bicom %2 a7 a8
bicom %2 a8 a9
bicom %2 -d a5 a4
bicom %2 -d a4 a3
bicom %2 -d a3 a2
bicom %2 -d a2 a1
copy a1 b1
bicom %2 b1 b2
bicom %2 b2 b3
bicom %2 b3 b4
bicom %2 b4 b5
bicom %2 b5 b6
bicom %2 b6 b7
bicom %2 b7 b8
bicom %2 b8 b9
copy b9 c9
bicom %2 -d c9 c8
bicom %2 -d c8 c7
bicom %2 -d c7 c6
bicom %2 -d c6 c5
bicom %2 -d c5 c4
bicom %2 -d c4 c3
bicom %2 -d c3 c2
bicom %2 -d c2 c1
@echo off
fc /b a1 b1
fc /b a2 b2
fc /b a3 b3
fc /b a4 b4
fc /b a5 b5
fc /b a6 b6
fc /b a7 b7
fc /b a8 b8
fc /b a9 b9
fc /b b1 c1
fc /b b2 c2
fc /b b3 c3
fc /b b4 c4
fc /b b5 c5
fc /b b6 c6
fc /b b7 c7
fc /b b8 c8
fc /b b9 c9

