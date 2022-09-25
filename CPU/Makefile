all: countheap cmsketch coco cusketch elasticp4 fcm fermat hashpipe mrac univmon

countheap: ./test/countheap.cpp
	g++ -O2 -o ./out/countheap.out ./test/countheap.cpp
cmsketch: ./test/cmsketch.cpp
	g++ -O2 -o ./out/cmsketch.out ./test/cmsketch.cpp
coco: ./test/coco.cpp
	g++ -O2 -o ./out/coco.out ./test/coco.cpp
cusketch: ./test/cusketch.cpp
	g++ -O2 -o ./out/cusketch.out ./test/cusketch.cpp
elasticp4: ./test/elasticp4.cpp
	g++ -O2 -o ./out/elasticp4.out ./test/elasticp4.cpp 
fcm: ./test/fcm.cpp
	g++ -O2 -o ./out/fcm.out ./test/fcm.cpp -lpthread
fermat: ./test/fermat.cpp
	g++ -O2 -o ./out/fermat.out ./test/fermat.cpp
hashpipe: ./test/hashpipe.cpp
	g++ -O2 -o ./out/hashpipe.out ./test/hashpipe.cpp
mrac: ./test/mrac.cpp
	g++ -O2 -o ./out/mrac.out ./test/mrac.cpp
univmon: ./test/univmon.cpp
	g++ -O2 -o ./out/univmon.out ./test/univmon.cpp

clean:
	rm ./out/*.out 