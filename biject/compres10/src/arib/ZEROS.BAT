arib zeros.dat zb1.cmp
unarib zb1.cmp xx.tmp
fc /b xx.tmp zeros.dat > yy.tmp
del xx.tmp
unarib zeros.dat zb1.unc
arib zb1.unc xx.tmp
fc /b xx.tmp zeros.dat >> yy.tmp
del xx.tmp
arib32 zeros.dat zb2.cmp
unarib32 zb2.cmp xx.tmp
fc /b xx.tmp zeros.dat >> yy.tmp
del xx.tmp
unarib32 zeros.dat zb2.unc
arib32 zb2.unc xx.tmp
fc /b xx.tmp zeros.dat >> yy.tmp
del xx.tmp
arib arib.cpp zb3.cmp
unarib zb3.cmp xx.tmp
fc /b xx.tmp arib.cpp >> yy.tmp
del xx.tmp
unarib arib.cpp zb3.unc
arib zb3.unc xx.tmp
fc /b xx.tmp arib.cpp >> yy.tmp
del xx.tmp
arib32 arib.cpp zb4.cmp
unarib32 zb4.cmp xx.tmp
fc /b xx.tmp arib.cpp >> yy.tmp
del xx.tmp
unarib32 arib.cpp zb4.unc
arib32 zb4.unc xx.tmp
fc /b xx.tmp arib.cpp >> yy.tmp
del xx.tmp
