#ifndef _CMSKETCH_H
#define _CMSKETCH_H

#include <sstream>
#include <cstring>
#include <algorithm>
#include <cstdint>
#include <vector>
#include "../Common/BOBHash32.h"
#include "../common_func.h"

using namespace std;

class CMSketch
{
private:
	int memory_in_bytes = 0;

	int w = 0;
	int* counters[CM_DEPTH] = {NULL};
	BOBHash32* hash[CM_DEPTH] = {NULL};

public:
    std::set<std::vector<uint8_t> > HH_candidates;
	string name;

	CMSketch(){}
	CMSketch(int memory_in_bytes)
	{	initial(memory_in_bytes);	}
	~CMSketch(){clear();}

	void initial(int memory_in_bytes)
	{
		this->memory_in_bytes = memory_in_bytes;
		w = memory_in_bytes / 4 / CM_DEPTH;
        int init = INIT;
		for(int i = 0; i < CM_DEPTH; ++i){
			counters[i] = new int[w];
			memset(counters[i], 0, 4 * w); // 4*w bytes allocate with filling 0 value.

			hash[i] = new BOBHash32(i + init);
		}


		stringstream name_buffer;
        name_buffer << "CM@" << memory_in_bytes << "@" << CM_DEPTH;
        name = name_buffer.str();
	}
	void clear()
	{
		for(int i = 0; i < CM_DEPTH; ++i)
			delete[] counters[i];

		for (int i = 0; i < CM_DEPTH; ++i)
			delete hash[i];
	}

	void print_basic_info()
    {
        printf("CM sketch\n");
        printf("\tCounters: %d\n", w);
        printf("\tMemory: %.6lfMB\n", w * CM_DEPTH * 4.0 / 1024 / 1024);
        int total_pck = 0;
        for (int dep = 0; dep < CM_DEPTH; ++dep)
            for (int i = 0; i < w; ++i)
                total_pck += counters[dep][i];
            printf("\tTotal_packet at depth %d : %10d\n", CM_DEPTH, total_pck);
    }

    void insert(uint8_t * key, int f = 1)
    {
        std::vector<int> meta_indicator(CM_DEPTH, 0);
        for (int i = 0; i < CM_DEPTH; i++) {
            int index = (hash[i]->run((const char *)key, 4)) % w;
            // int index = (hash[i]->run((const char *)key, 13));
            counters[i][index] += f;
            if (counters[i][index] > HH_THRESHOLD)
                meta_indicator[i] = 1;
        }
        if (std::accumulate(meta_indicator.begin(), meta_indicator.end(), 0) == CM_DEPTH)
            heavy_insert(key);
    }

    void heavy_insert(uint8_t * key)
    {
        std::vector<uint8_t> temp_key(4);
        temp_key.assign(key, key + 4);
        HH_candidates.insert(temp_key);
        return;
    }

	int query(uint8_t * key)
    {
        int ret = 1 << 30; // |flow| < 0100,0000,0000,0000 (16 bits = int)
        for (int i = 0; i < CM_DEPTH; i++) {
        	int index = (hash[i]->run((const char *)key, 4)) % w;
            int tmp = counters[i][index];
            ret = min(ret, tmp);
        }
        return ret;
    }
    
    int get_cardinality()
    {
        int empty = 0;
        for (int i = 0; i < w; ++i)
        {
            if (counters[0][i] == 0)
                empty += 1;
        }
        return (int)(w * std::log(double(w) / double(empty)));
    }
};


#endif
