all: 
	@if [ ! -d ./build ]; then mkdir -p ./build; fi; 
	make test_bucPF8 test_dropf_mem test_flow_mem test_perc_mem

test_bucPF8: ./test_bucPF8.cc
	g++ -O2 -o ./build/test_bucPF8.out ./test_bucPF8.cc
test_dropf_mem: ./test_dropf_mem.cc
	g++ -O2 -o ./build/test_dropf_mem.out ./test_dropf_mem.cc
test_flow_mem: ./test_flow_mem.cc
	g++ -O2 -o ./build/test_flow_mem.out ./test_flow_mem.cc
test_perc_mem: ./test_perc_mem.cc
	g++ -O2 -o ./build/test_perc_mem.out ./test_perc_mem.cc

clean:
	rm ./build/*.out 