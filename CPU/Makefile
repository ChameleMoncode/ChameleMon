all: 
	@if [ ! -d ./build ]; then mkdir -p ./build; fi; 
	make countheap cmsketch coco cusketch elasticp4 fcm fermat hashpipe mrac univmon test_bucPF8 test_dropf_mem test_flow_mem test_perc_mem

countheap: ./MeasurementTasks/countheap.cpp
	g++ -O2 -o ./build/countheap.out ./MeasurementTasks/countheap.cpp
cmsketch: ./MeasurementTasks/cmsketch.cpp
	g++ -O2 -o ./build/cmsketch.out ./MeasurementTasks/cmsketch.cpp
coco: ./MeasurementTasks/coco.cpp
	g++ -O2 -o ./build/coco.out ./MeasurementTasks/coco.cpp
cusketch: ./MeasurementTasks/cusketch.cpp
	g++ -O2 -o ./build/cusketch.out ./MeasurementTasks/cusketch.cpp
elasticp4: ./MeasurementTasks/elasticp4.cpp
	g++ -O2 -o ./build/elasticp4.out ./MeasurementTasks/elasticp4.cpp 
fcm: ./MeasurementTasks/fcm.cpp
	g++ -O2 -o ./build/fcm.out ./MeasurementTasks/fcm.cpp -lpthread
fermat: ./MeasurementTasks/fermat.cpp
	g++ -O2 -o ./build/fermat.out ./MeasurementTasks/fermat.cpp
hashpipe: ./MeasurementTasks/hashpipe.cpp
	g++ -O2 -o ./build/hashpipe.out ./MeasurementTasks/hashpipe.cpp
mrac: ./MeasurementTasks/mrac.cpp
	g++ -O2 -o ./build/mrac.out ./MeasurementTasks/mrac.cpp
univmon: ./MeasurementTasks/univmon.cpp
	g++ -O2 -o ./build/univmon.out ./MeasurementTasks/univmon.cpp
test_bucPF8: ./Drop/test_bucPF8.cc
	g++ -O2 -o ./build/test_bucPF8.out ./Drop/test_bucPF8.cc
test_dropf_mem: ./Drop/test_dropf_mem.cc
	g++ -O2 -o ./build/test_dropf_mem.out ./Drop/test_dropf_mem.cc
test_flow_mem: ./Drop/test_flow_mem.cc
	g++ -O2 -o ./build/test_flow_mem.out ./Drop/test_flow_mem.cc
test_perc_mem: ./Drop/test_perc_mem.cc
	g++ -O2 -o ./build/test_perc_mem.out ./Drop/test_perc_mem.cc

clean:
	rm ./build/*.out 