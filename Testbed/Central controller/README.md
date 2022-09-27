The source codes of central controller of ChameleMon.

### Compile

* Set up the environment variable `RTE_SDK`. We use the dpdk version 18.11.11
* Run `make CC=g++`

### Run the code

`./build/fermat_test -w [PCI_device] -l [core_id] [workload_distribution] [output_log]`