# testdeps.mk 
do_all : test60.stamp
sql : test60.stamp
60: test60.stamp
test60.stamp : test60.exe 
test60.exe : test60.c test60.Sc tests.h 
test60.c test60.Sc : test60.sql  $(SQLCPP)
do_all : testA0.stamp
sql : testA0.stamp
A0: testA0.stamp
testA0.stamp : testA0.exe 
testA0.exe : testA0.c testA0.Sc tests.h 
testA0.c testA0.Sc : testA0.sql  $(SQLCPP)
do_all : testFLdef.stamp
sql : testFLdef.stamp
FLdef: testFLdef.stamp
testFLdef.stamp : testFLdef.exe 
testFLdef.exe : testFLdef.c testFLdef.Sc tests.h 
testFLdef.c testFLdef.Sc : testFLdef.sql  $(SQLCPP)
do_all : testFLins.stamp
sql : testFLins.stamp
FLins: testFLins.stamp
testFLins.stamp : testFLins.exe 
testFLins.exe : testFLins.c testFLins.Sc tests.h 
testFLins.c testFLins.Sc : testFLins.sql  $(SQLCPP)
do_all : testIS0.stamp
sql : testIS0.stamp
IS0: testIS0.stamp
testIS0.stamp : testIS0.exe 
testIS0.exe : testIS0.c testIS0.Sc tests.h 
testIS0.c testIS0.Sc : testIS0.sql  $(SQLCPP)
do_all : testIS1.stamp
sql : testIS1.stamp
IS1: testIS1.stamp
testIS1.stamp : testIS1.exe 
testIS1.exe : testIS1.c testIS1.Sc tests.h 
testIS1.c testIS1.Sc : testIS1.sql  $(SQLCPP)
do_all : testIS2.stamp
sql : testIS2.stamp
IS2: testIS2.stamp
testIS2.stamp : testIS2.exe 
testIS2.exe : testIS2.c testIS2.Sc tests.h 
testIS2.c testIS2.Sc : testIS2.sql  $(SQLCPP)
do_all : testIS3.stamp
sql : testIS3.stamp
IS3: testIS3.stamp
testIS3.stamp : testIS3.exe 
testIS3.exe : testIS3.c testIS3.Sc tests.h 
testIS3.c testIS3.Sc : testIS3.sql  $(SQLCPP)
do_all : testIS4.stamp
sql : testIS4.stamp
IS4: testIS4.stamp
testIS4.stamp : testIS4.exe 
testIS4.exe : testIS4.c testIS4.Sc tests.h 
testIS4.c testIS4.Sc : testIS4.sql  $(SQLCPP)
do_all : testIS5.stamp
sql : testIS5.stamp
IS5: testIS5.stamp
testIS5.stamp : testIS5.exe 
testIS5.exe : testIS5.c testIS5.Sc tests.h 
testIS5.c testIS5.Sc : testIS5.sql  $(SQLCPP)
do_all : testIS6.stamp
sql : testIS6.stamp
IS6: testIS6.stamp
testIS6.stamp : testIS6.exe 
testIS6.exe : testIS6.c testIS6.Sc tests.h 
testIS6.c testIS6.Sc : testIS6.sql  $(SQLCPP)
do_all : testIS7.stamp
sql : testIS7.stamp
IS7: testIS7.stamp
testIS7.stamp : testIS7.exe 
testIS7.exe : testIS7.c testIS7.Sc tests.h 
testIS7.c testIS7.Sc : testIS7.sql  $(SQLCPP)
do_all : testIS8.stamp
sql : testIS8.stamp
IS8: testIS8.stamp
testIS8.stamp : testIS8.exe 
testIS8.exe : testIS8.c testIS8.Sc tests.h 
testIS8.c testIS8.Sc : testIS8.sql  $(SQLCPP)
do_all : testM0.stamp
sql : testM0.stamp
M0: testM0.stamp
testM0.stamp : testM0.exe 
testM0.exe : testM0.c testM0.Sc tests.h 
testM0.c testM0.Sc : testM0.sql  $(SQLCPP)
do_all : testT0.stamp
sql : testT0.stamp
T0: testT0.stamp
testT0.stamp : testT0.exe 
testT0.exe : testT0.c testT0.Sc tests.h 
testT0.c testT0.Sc : testT0.sql  $(SQLCPP)
do_all : testTV.stamp
sql : testTV.stamp
TV: testTV.stamp
testTV.stamp : testTV.exe 
testTV.exe : testTV.c testTV.Sc tests.h 
testTV.c testTV.Sc : testTV.sql  $(SQLCPP)
do_all : testZ.stamp
sql : testZ.stamp
Z: testZ.stamp
testZ.stamp : testZ.exe 
testZ.exe : testZ.c testZ.Sc tests.h 
testZ.c testZ.Sc : testZ.sql  $(SQLCPP)
do_all : test10.stamp
ec : test10.stamp
10: test10.stamp
	echo test10 finished
test10.stamp : test10.exe 
test10.exe : test10.c test10.Sc tests.h 
test10.c test10.Sc : test10.ec  $(SQLCPP)
do_all : test11.stamp
ec : test11.stamp
11: test11.stamp
	echo test11 finished
test11.stamp : test11.exe 
test11.exe : test11.c test11.Sc tests.h 
test11.c test11.Sc : test11.ec  $(SQLCPP)
do_all : test12.stamp
ec : test12.stamp
12: test12.stamp
	echo test12 finished
test12.stamp : test12.exe 
test12.exe : test12.c test12.Sc tests.h 
test12.c test12.Sc : test12.ec  $(SQLCPP)
do_all : test13.stamp
ec : test13.stamp
13: test13.stamp
	echo test13 finished
test13.stamp : test13.exe 
test13.exe : test13.c test13.Sc tests.h 
test13.c test13.Sc : test13.ec  $(SQLCPP)
do_all : test14.stamp
ec : test14.stamp
14: test14.stamp
	echo test14 finished
test14.stamp : test14.exe 
test14.exe : test14.c test14.Sc tests.h 
test14.c test14.Sc : test14.ec  $(SQLCPP)
do_all : test15.stamp
ec : test15.stamp
15: test15.stamp
	echo test15 finished
test15.stamp : test15.exe 
test15.exe : test15.c test15.Sc tests.h 
test15.c test15.Sc : test15.ec  $(SQLCPP)
do_all : test16.stamp
ec : test16.stamp
16: test16.stamp
	echo test16 finished
test16.stamp : test16.exe 
test16.exe : test16.c test16.Sc tests.h 
test16.c test16.Sc : test16.ec  $(SQLCPP)
do_all : test20.stamp
ec : test20.stamp
20: test20.stamp
	echo test20 finished
test20.stamp : test20.exe 
test20.exe : test20.c test20.Sc tests.h 
test20.c test20.Sc : test20.ec  $(SQLCPP)
do_all : test21.stamp
ec : test21.stamp
21: test21.stamp
	echo test21 finished
test21.stamp : test21.exe 
test21.exe : test21.c test21.Sc tests.h 
test21.c test21.Sc : test21.ec  $(SQLCPP)
do_all : test22.stamp
ec : test22.stamp
22: test22.stamp
	echo test22 finished
test22.stamp : test22.exe 
test22.exe : test22.c test22.Sc tests.h 
test22.c test22.Sc : test22.ec  $(SQLCPP)
do_all : test23.stamp
ec : test23.stamp
23: test23.stamp
	echo test23 finished
test23.stamp : test23.exe 
test23.exe : test23.c test23.Sc tests.h 
test23.c test23.Sc : test23.ec  $(SQLCPP)
do_all : test24.stamp
ec : test24.stamp
24: test24.stamp
	echo test24 finished
test24.stamp : test24.exe 
test24.exe : test24.c test24.Sc tests.h 
test24.c test24.Sc : test24.ec  $(SQLCPP)
do_all : test25.stamp
ec : test25.stamp
25: test25.stamp
	echo test25 finished
test25.stamp : test25.exe 
test25.exe : test25.c test25.Sc tests.h 
test25.c test25.Sc : test25.ec  $(SQLCPP)
do_all : test26.stamp
ec : test26.stamp
26: test26.stamp
	echo test26 finished
test26.stamp : test26.exe 
test26.exe : test26.c test26.Sc tests.h 
test26.c test26.Sc : test26.ec  $(SQLCPP)
do_all : test30.stamp
ec : test30.stamp
30: test30.stamp
	echo test30 finished
test30.stamp : test30.exe 
test30.exe : test30.c test30.Sc tests.h 
test30.c test30.Sc : test30.ec  $(SQLCPP)
do_all : test31.stamp
ec : test31.stamp
31: test31.stamp
	echo test31 finished
test31.stamp : test31.exe 
test31.exe : test31.c test31.Sc tests.h 
test31.c test31.Sc : test31.ec  $(SQLCPP)
do_all : test33.stamp
ec : test33.stamp
33: test33.stamp
	echo test33 finished
test33.stamp : test33.exe 
test33.exe : test33.c test33.Sc tests.h 
test33.c test33.Sc : test33.ec  $(SQLCPP)
do_all : test3a.stamp
ec : test3a.stamp
3a: test3a.stamp
	echo test3a finished
test3a.stamp : test3a.exe 
test3a.exe : test3a.c test3a.Sc tests.h 
test3a.c test3a.Sc : test3a.ec  $(SQLCPP)
do_all : test3b.stamp
ec : test3b.stamp
3b: test3b.stamp
	echo test3b finished
test3b.stamp : test3b.exe 
test3b.exe : test3b.c test3b.Sc tests.h 
test3b.c test3b.Sc : test3b.ec  $(SQLCPP)
do_all : test3c.stamp
ec : test3c.stamp
3c: test3c.stamp
	echo test3c finished
test3c.stamp : test3c.exe 
test3c.exe : test3c.c test3c.Sc tests.h 
test3c.c test3c.Sc : test3c.ec  $(SQLCPP)
do_all : test40.stamp
ec : test40.stamp
40: test40.stamp
	echo test40 finished
test40.stamp : test40.exe 
test40.exe : test40.c test40.Sc tests.h 
test40.c test40.Sc : test40.ec  $(SQLCPP)
do_all : test41.stamp
ec : test41.stamp
41: test41.stamp
	echo test41 finished
test41.stamp : test41.exe 
test41.exe : test41.c test41.Sc tests.h 
test41.c test41.Sc : test41.ec  $(SQLCPP)
do_all : test42.stamp
ec : test42.stamp
42: test42.stamp
	echo test42 finished
test42.stamp : test42.exe 
test42.exe : test42.c test42.Sc tests.h 
test42.c test42.Sc : test42.ec  $(SQLCPP)
do_all : test50.stamp
ec : test50.stamp
50: test50.stamp
	echo test50 finished
test50.stamp : test50.exe 
test50.exe : test50.c test50.Sc tests.h 
test50.c test50.Sc : test50.ec  $(SQLCPP)
do_all : test51.stamp
ec : test51.stamp
51: test51.stamp
	echo test51 finished
test51.stamp : test51.exe 
test51.exe : test51.c test51.Sc tests.h 
test51.c test51.Sc : test51.ec  $(SQLCPP)
do_all : test70.stamp
ec : test70.stamp
70: test70.stamp
	echo test70 finished
test70.stamp : test70.exe 
test70.exe : test70.c test70.Sc tests.h 
test70.c test70.Sc : test70.ec  $(SQLCPP)
do_all : test80.stamp
ec : test80.stamp
80: test80.stamp
	echo test80 finished
test80.stamp : test80.exe 
test80.exe : test80.c test80.Sc tests.h 
test80.c test80.Sc : test80.ec  $(SQLCPP)
do_all : testA1.stamp
ec : testA1.stamp
A1: testA1.stamp
	echo testA1 finished
testA1.stamp : testA1.exe 
testA1.exe : testA1.c testA1.Sc tests.h 
testA1.c testA1.Sc : testA1.ec  $(SQLCPP)
do_all : testd1.stamp
ec : testd1.stamp
d1: testd1.stamp
	echo testd1 finished
testd1.stamp : testd1.exe 
testd1.exe : testd1.c testd1.Sc tests.h 
testd1.c testd1.Sc : testd1.ec  $(SQLCPP)
do_all : testd2.stamp
ec : testd2.stamp
d2: testd2.stamp
	echo testd2 finished
testd2.stamp : testd2.exe 
testd2.exe : testd2.c testd2.Sc tests.h 
testd2.c testd2.Sc : testd2.ec  $(SQLCPP)
do_all : testd4.stamp
ec : testd4.stamp
d4: testd4.stamp
	echo testd4 finished
testd4.stamp : testd4.exe 
testd4.exe : testd4.c testd4.Sc tests.h 
testd4.c testd4.Sc : testd4.ec  $(SQLCPP)
do_all : testM1.stamp
ec : testM1.stamp
M1: testM1.stamp
	echo testM1 finished
testM1.stamp : testM1.exe 
testM1.exe : testM1.c testM1.Sc tests.h 
testM1.c testM1.Sc : testM1.ec  $(SQLCPP)
do_all : testM1a.stamp
ec : testM1a.stamp
M1a: testM1a.stamp
	echo testM1a finished
testM1a.stamp : testM1a.exe 
testM1a.exe : testM1a.c testM1a.Sc tests.h 
testM1a.c testM1a.Sc : testM1a.ec  $(SQLCPP)
do_all : testM2.stamp
ec : testM2.stamp
M2: testM2.stamp
	echo testM2 finished
testM2.stamp : testM2.exe 
testM2.exe : testM2.c testM2.Sc tests.h 
testM2.c testM2.Sc : testM2.ec  $(SQLCPP)
do_all : testM3.stamp
ec : testM3.stamp
M3: testM3.stamp
	echo testM3 finished
testM3.stamp : testM3.exe 
testM3.exe : testM3.c testM3.Sc tests.h 
testM3.c testM3.Sc : testM3.ec  $(SQLCPP)
do_all : testM4.stamp
ec : testM4.stamp
M4: testM4.stamp
	echo testM4 finished
testM4.stamp : testM4.exe 
testM4.exe : testM4.c testM4.Sc tests.h 
testM4.c testM4.Sc : testM4.ec  $(SQLCPP)
do_all : testT10.stamp
ec : testT10.stamp
T10: testT10.stamp
	echo testT10 finished
testT10.stamp : testT10.exe 
testT10.exe : testT10.c testT10.Sc tests.h 
testT10.c testT10.Sc : testT10.ec  $(SQLCPP)
do_all : testT11.stamp
ec : testT11.stamp
T11: testT11.stamp
	echo testT11 finished
testT11.stamp : testT11.exe 
testT11.exe : testT11.c testT11.Sc tests.h 
testT11.c testT11.Sc : testT11.ec  $(SQLCPP)
do_all : testT20.stamp
ec : testT20.stamp
T20: testT20.stamp
	echo testT20 finished
testT20.stamp : testT20.exe 
testT20.exe : testT20.c testT20.Sc tests.h 
testT20.c testT20.Sc : testT20.ec  $(SQLCPP)
do_all : testT21.stamp
ec : testT21.stamp
T21: testT21.stamp
	echo testT21 finished
testT21.stamp : testT21.exe 
testT21.exe : testT21.c testT21.Sc tests.h 
testT21.c testT21.Sc : testT21.ec  $(SQLCPP)
do_all : testT30.stamp
ec : testT30.stamp
T30: testT30.stamp
	echo testT30 finished
testT30.stamp : testT30.exe 
testT30.exe : testT30.c testT30.Sc tests.h 
testT30.c testT30.Sc : testT30.ec  $(SQLCPP)
do_all : testT40.stamp
ec : testT40.stamp
T40: testT40.stamp
	echo testT40 finished
testT40.stamp : testT40.exe 
testT40.exe : testT40.c testT40.Sc tests.h 
testT40.c testT40.Sc : testT40.ec  $(SQLCPP)
do_all : testTV20.stamp
ec : testTV20.stamp
TV20: testTV20.stamp
	echo testTV20 finished
testTV20.stamp : testTV20.exe 
testTV20.exe : testTV20.c testTV20.Sc tests.h 
testTV20.c testTV20.Sc : testTV20.ec  $(SQLCPP)
do_all : testZ20.stamp
ec : testZ20.stamp
Z20: testZ20.stamp
	echo testZ20 finished
testZ20.stamp : testZ20.exe 
testZ20.exe : testZ20.c testZ20.Sc tests.h 
testZ20.c testZ20.Sc : testZ20.ec  $(SQLCPP)
do_all : testZ30.stamp
ec : testZ30.stamp
Z30: testZ30.stamp
	echo testZ30 finished
testZ30.stamp : testZ30.exe 
testZ30.exe : testZ30.c testZ30.Sc tests.h 
testZ30.c testZ30.Sc : testZ30.ec  $(SQLCPP)
do_all : testZ31.stamp
ec : testZ31.stamp
Z31: testZ31.stamp
	echo testZ31 finished
testZ31.stamp : testZ31.exe 
testZ31.exe : testZ31.c testZ31.Sc tests.h 
testZ31.c testZ31.Sc : testZ31.ec  $(SQLCPP)
do_all : testZ40.stamp
ec : testZ40.stamp
Z40: testZ40.stamp
	echo testZ40 finished
testZ40.stamp : testZ40.exe 
testZ40.exe : testZ40.c testZ40.Sc tests.h 
testZ40.c testZ40.Sc : testZ40.ec  $(SQLCPP)
