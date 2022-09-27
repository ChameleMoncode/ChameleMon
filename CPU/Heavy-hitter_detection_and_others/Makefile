all: 
	@if [ ! -d ./build ]; then mkdir -p ./build; fi; 
	make countheap cmsketch coco cusketch elasticp4 fcm fermat hashpipe mrac univmon

countheap: ./countheap.cpp
	g++ -O2 -o ./build/countheap.out ./countheap.cpp
cmsketch: ./cmsketch.cpp
	g++ -O2 -o ./build/cmsketch.out ./cmsketch.cpp
coco: ./coco.cpp
	g++ -O2 -o ./build/coco.out ./coco.cpp
cusketch: ./cusketch.cpp
	g++ -O2 -o ./build/cusketch.out ./cusketch.cpp
elasticp4: ./elasticp4.cpp
	g++ -O2 -o ./build/elasticp4.out ./elasticp4.cpp 
fcm: ./fcm.cpp
	g++ -O2 -o ./build/fcm.out ./fcm.cpp -lpthread
fermat: ./fermat.cpp
	g++ -O2 -o ./build/fermat.out ./fermat.cpp
hashpipe: ./hashpipe.cpp
	g++ -O2 -o ./build/hashpipe.out ./hashpipe.cpp
mrac: ./mrac.cpp
	g++ -O2 -o ./build/mrac.out ./mrac.cpp
univmon: ./univmon.cpp
	g++ -O2 -o ./build/univmon.out ./univmon.cpp


clean:
	rm ./build/*.out 