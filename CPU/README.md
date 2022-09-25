The source code of algorithm is under the folder ./src, while the test code is under the folder ./test. Here we test the accuracy of flow measurement and drop packets. 
You can simply use command "make" to compile all the files and the target file are in folder ./out.
# Flow measurement
We use different algorithm to complete six different tasks of flow measurement. By executing SKETCHNAME.out you can get the accuracy of measurement.
# Drop packets
The test_dropf_mem.out, test_flow_mem.out, test_perc_mem.out are the output files.

## test_dropf_mem
- x axix is dropped flow number, y axix is memory
- param: ./test_dropf_mem $cnt $mod $accuracy
- $cnt: the number of dropped flow in experiment
- $mod: 0 to drop 1% of packets and 1 to drop 10% of packets
- $accuracy: accuracy to stop the experiment. 999 means when 999 of the 1000 experiments report true, it will stop the running. e.g. 

## test_flow_mem
- x axix is total flow number, y axix is memory
- param: ./test_flow_mem $cnt $accuracy
- $cnt: the number of total flow in experiment
- $accuracy: accuracy to stop the experiment. 999 means when 999 of the 1000 experiments report true, it will stop the running. e.g. 

## test_perc_mem
- x axix is flow drop percent, y axix is memory
- param: ./test_perc_mem $perc $accuracy
- $perc: the percent of flow drop in experiment
- $accuracy: accuracy to stop the experiment. 999 means when 999 of the 1000 experiments report true, it will stop the running. e.g. 