copy %1 a5
biacode a5 a6
biacode a6 a7
biacode a7 a8
biacode a8 a9
biacode -d a5 a4
biacode -d a4 a3
biacode -d a3 a2
biacode -d a2 a1
copy a1 b1
biacode b1 b2
biacode b2 b3
biacode b3 b4
biacode b4 b5
biacode b5 b6
biacode b6 b7
biacode b7 b8
biacode b8 b9
copy b9 c9
biacode -d c9 c8
biacode -d c8 c7
biacode -d c7 c6
biacode -d c6 c5
biacode -d c5 c4
biacode -d c4 c3
biacode -d c3 c2
biacode -d c2 c1
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

