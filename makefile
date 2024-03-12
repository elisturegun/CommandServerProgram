project: comcli.c comserver.c shareddefs.h
	gcc -o comserver comserver.c
	gcc -o comcli comcli.c

clean:
	rm -f comserver comcli cspipe* scpipe* out* in*

testserver:
	./comserver /mqueuename

testclient:
	./comcli /mqueuename

timedtestclient:
	time ./comcli /mqueuename

testbatchclient:
	./comcli /mqueuename -b testinput.txt

testbatchclientQALL:
	./comcli /mqueuename -b testinputquitall.txt

timedtestbatchclient:
	time ./comcli /mqueuename -b testinput.txt

timedtestlongbatchclient:
	time ./comcli /mqueuename -b longtestinput.txt

timedtest1:
	time ./comcli /mqueuename -b testinput.txt -s 64
	
timedtest2:
	time ./comcli /mqueuename -b testinput.txt -s 256
	
timedtest3:
	time ./comcli /mqueuename -b testinput.txt -s 1024
	
timedtest0:
	time ./comcli /mqueuename -b testinput.txt -s 32
	

twoclienttest:
	./comcli /mqueuename -b testinput.txt
	./comcli /mqueuename -b testinput.txt

threeclienttest:
	./comcli /mqueuename -b testinput.txt
	./comcli /mqueuename -b testinput.txt
	./comcli /mqueuename -b testinput.txt

fourclienttest:
	./comcli /mqueuename -b testinput.txt
	./comcli /mqueuename -b testinput.txt
	./comcli /mqueuename -b testinput.txt
	./comcli /mqueuename -b testinput.txt

fiveclienttest:
	./comcli /mqueuename -b testinput.txt
	./comcli /mqueuename -b testinput.txt
	./comcli /mqueuename -b testinput.txt
	./comcli /mqueuename -b testinput.txt
	./comcli /mqueuename -b testinput.txt
