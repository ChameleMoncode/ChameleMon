#ifndef _FCMPLUS_H_
#define _FCMPLUS_H_

#include "./FCMPlus_sketch.h"

class FCMPlus
{
	vector<pair<uint32_t, uint32_t> > KV {vector<pair<uint32_t, uint32_t> >(FCMPLUS_BUCKET, std::make_pair(0,0))}; // <key, value> pairs
	vector<uint32_t> VALALL = vector<uint32_t>(FCMPLUS_BUCKET, 0); // val_all in ElasticSketch Top-K
	FCMPlus_sketch * fcmsketch; // sketch part
	BOBHash32* hash_topk; // BobHash[Top-K level]

public:
	set<uint32_t> hh_candidates;

	FCMPlus(){
		// initialize FCM-Sketch
		fcmsketch = new FCMPlus_sketch;

		// set hash functions
		hash_topk = new BOBHash32(730);

		// calculate total memory
		uint32_t total_memory = FCMPLUS_BUCKET * (8 * 3 + 4); // key, val, val_all => 28 bytes in Tofino (with 3 times duplicated storage of key-value)
		total_memory += fcmsketch->total_memory;
		printf("[INFO-FCMPLUS] Total Memory Usage in Actual Tofino hardware : %10dbit (%2.2fMB)\n", total_memory, total_memory / 1024.0 / 1024.0);
	}

	~FCMPlus(){
		delete fcmsketch;
		delete hash_topk;
	}

	void insert(uint8_t * key, uint32_t f=1) {
		/* we emulate the pipeline process on PISA */

		/******** Insert packet into Top-K Algorithm ********/
		int pos_topk = init_hashing_topk(key);
		// compare with existing <key,val> and check match/noswap/swap
		uint32_t swap_key;
		uint32_t swap_val;
		bool condition_swap = 0;
		bool condition_match = 0;

		VALALL[pos_topk] += f; // update val_all
		// check conditions (including "shift")
		if (JUDGE_IF_SWAP_FCMPLUS_P4(KV[pos_topk].second, VALALL[pos_topk]))
			condition_swap = 1; // swap condition
		if (KV[pos_topk].first == *((uint32_t*)key))
			condition_match = 1; // matching key

		// do stateful actions on registers
		if (condition_swap == 1 and condition_match == 0){
			// do swap
			swap_key = KV[pos_topk].first;
			swap_val = KV[pos_topk].second; 
		}

		if (condition_swap == 1 or condition_match == 1){
			KV[pos_topk].second += f; // update counters
			KV[pos_topk].first = *((uint32_t*)key); // if swap
		}

		// take action on next (sketch) for different cases
		if (condition_swap == 1 and condition_match == 0){
			// swap happens. Use swap_key, swap_val to update sketch
			uint8_t swap_key_arr[4] = {0};
	        for (int i=0; i<4 ;++i){
	        	uint32_t temp_key = htonl(swap_key);
            	swap_key_arr[i] = ((uint8_t*)&temp_key)[3-i];
        	}
			fcmsketch->insert(swap_key_arr, swap_val);
		}
		else if (condition_swap == 0 and condition_match == 0){
			// No swap. Update the sketch part using origianl key-value
			fcmsketch->insert(key, f);
		}
		// Otherwise, do nothing in sketch part
		return; 
	}

	uint32_t query(uint8_t * key){
		uint32_t count_query = 0;
		// Collect counts in Top-K Algorithm
		int pos_topk = init_hashing_topk(key);
		if (KV[pos_topk].first == *((uint32_t*)key)){
			count_query += KV[pos_topk].second;
			// topk_val += KV[pos_topk].second; // temp
		}
		// Collect counts in FCM-Sketch
		count_query += fcmsketch->query(key);
		return count_query;
	}

	int init_hashing_topk(uint8_t * key){
		return (hash_topk->run((const char *)key, 4)) % FCMPLUS_BUCKET;
	}

	int get_cardinality(){
		int card = fcmsketch->get_cardinality();
		for (auto it = KV.begin(); it != KV.end(); ++it){
			uint32_t sum_val = it->second; // counts in Top-K
			uint8_t key_arr[4] = {0};
			for (int i = 0; i < 4; ++i){
				uint32_t temp_key = htonl(it->first);
				key_arr[i] = ((uint8_t*)&temp_key)[3-i];
			}
			uint32_t sketch_val = fcmsketch->query(key_arr); // counts in sketch

			if (sketch_val != 0){
				sum_val += sketch_val;
				card--;
			}
			if (sum_val){
				card++;
			}
		}
		return card;
	}


    void get_distribution(vector<double> &dist){
    	fcmsketch->get_distribution(dist);
    	for (auto it = KV.begin(); it != KV.end(); ++it){
    		uint32_t sum_val = it->second; // counts in Top-K
			uint8_t key_arr[4] = {0};
			for (int i = 0; i < 4; ++i){
				uint32_t temp_key = htonl(it->first);
				key_arr[i] = ((uint8_t*)&temp_key)[3-i];
			}
			uint32_t sketch_val = fcmsketch->query(key_arr); // counts in sketch

			if (sketch_val != 0){
				sum_val += sketch_val;
				dist[sketch_val]--;
			}
			if (sum_val){
				if (sum_val + 1 > dist.size())
					dist.resize(sum_val + 1);
				dist[sum_val]++;
			}
		}
    }

    void push_hh(){
    	this->hh_candidates = fcmsketch->hh_candidates;
    	for (auto it = KV.begin(); it != KV.end(); ++it){
    		uint8_t key[4] = {0};
    		for (int i = 0; i < 4; ++i){
    			uint32_t temp_key = htonl(it->first);
    			key[i] = ((uint8_t*)&temp_key)[3-i];
    		}
    		if (this->query(key) > HC_THRESHOLD)
    			this->hh_candidates.insert(it->first);
    	}
    }
};

#endif
