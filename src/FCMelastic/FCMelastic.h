#ifndef _FCMPLUS_H_
#define _FCMPLUS_H_

#include "./FCMPlus_sketch.h"
#include "./our_heavypart.h"
class FCMPlus
{
private:
	ourHeavyPart * heavy_part[FCMPLUS_HEAVY_STAGE] = {NULL};
	FCMPlus_sketch * light_part = NULL; // sketch part
	BOBHash32* hash_heavy[FCMPLUS_HEAVY_STAGE] = {NULL}; // BobHash[Top-K level]
    uint32_t hash_num[FCMPLUS_HEAVY_STAGE] = {0};
public:
	FCMPlus(){
		// initialize FCM-Sketch
		light_part = new FCMPlus_sketch;
        int init = INIT;
		// set hash functions
		for (int i = 0; i < FCMPLUS_HEAVY_STAGE; ++i){
            heavy_part[i] = new ourHeavyPart();
            hash_heavy[i] = new BOBHash32(init+i);
            hash_num[i] = init + i;
        }

		// calculate total memory
		uint32_t total_memory = FCMPLUS_BUCKET * FCMPLUS_HEAVY_STAGE * 12; // key, val, val_all => 20 bytes in Tofino (with 2 times duplicated storage of key-value)
        total_memory += light_part->total_memory;
		printf("[INFO-FCMPLUS] Total Memory Usage in Actual Tofino hardware : %10dbit (%2.2fMB)\n", total_memory, total_memory / 1024.0 / 1024.0);
	}

	~FCMPlus(){
		delete light_part;
		for (int i = 0; i < FCMPLUS_HEAVY_STAGE; ++i){
            delete heavy_part[i];
            delete hash_heavy[i];
        }
	}

	void insert(uint8_t * key, uint32_t f=1) {
		uint8_t swap_key[4]; // metafield for key
        uint32_t swap_val = 0; // metafield for value
        int pos;
        int result;
        int val = f;
        for (int i = 0; i < FCMPLUS_HEAVY_STAGE; ++i){
            pos = hash_heavy[i]->run((const char *)key, 4) % FCMPLUS_BUCKET;

            result = heavy_part[i]->insert(key, swap_key, swap_val, pos, val);
            switch(result) {
                case 0: 
                return;
                case 1:{ // key, f
                    swap_val = 1;
                    continue;
                }
                case 2:{ // swap_key, swap_val
                    memcpy(key, swap_key, 4);
                    continue;
                }
            }
        }
        light_part->insert(key, val);
        return;
	}

	uint32_t query(uint8_t * key){
		uint32_t ret_val = 0;
        int pos;
        for (int i = 0; i < FCMPLUS_HEAVY_STAGE; ++i){
            pos = hash_heavy[i]->run((const char*)key, 4) % FCMPLUS_BUCKET;
            ret_val += heavy_part[i]->query(key, pos);
        }
        ret_val += light_part->query(key);
        return ret_val;
	}
	void get_heavy_hitters(set<uint32_t> & results){
		for (int i = 0; i <FCMPLUS_HEAVY_STAGE; ++i){
            for (int j = 0; j < FCMPLUS_BUCKET; ++j) {
                uint32_t key = heavy_part[i]->buckets[j].key;
                uint32_t val = this->query((uint8_t *)&key);
                if (val > HH_THRESHOLD)
                    results.insert(key);
            }
        }
	}
	void get_hc_candidates(unordered_map<uint32_t, int>& hc_candidates)
    {
        for (int i = 0; i < FCMPLUS_HEAVY_STAGE; ++i){
            for (int j = 0; j < FCMPLUS_BUCKET; ++j) {
                uint32_t key = heavy_part[i]->buckets[j].key;
                uint32_t val = this->query((uint8_t *)&key);
                if (val > HC_THRESHOLD)
                    hc_candidates[key] = val;
            }
        }
    }

	int get_cardinality(){
		int num_light_nonzero = 0;
		int card = light_part->get_cardinality();
		// step 1. collecting all items in Heavy Part
        set<uint32_t> checked_items;
        for (int i = 0; i < FCMPLUS_HEAVY_STAGE; ++i){
            for (int j = 0; j < FCMPLUS_BUCKET; ++j){
                uint32_t key = heavy_part[i]->buckets[j].key;
                if (key != 0)
                    checked_items.insert(key);
            }
        }

        // step 2. Merge heavy & light cardinality
        for (auto it = checked_items.begin(); it != checked_items.end(); ++it){
            uint32_t sum_val = 0;
            // heavy part query
            uint32_t heavy_val = 0;
            uint8_t key[4]; // temp key
            for (int i = 0; i < FCMPLUS_HEAVY_STAGE; ++i){
                *(uint32_t*)key = *it;
                int pos = hash_heavy[i]->run((const char*)key, 4) % FCMPLUS_BUCKET;
                heavy_val += heavy_part[i]->query(key, pos);
            }
            sum_val += heavy_val;

            // light part query
            uint32_t light_val = light_part->query(key);
            if (light_val != 0){
                sum_val += light_val;
                card--;
                num_light_nonzero++; 
            }
            if (sum_val){
                card++;
            }
        }
        return card;
	}


    void get_distribution(vector<double> &dist){
    	// printf("*** Start getting distribution in light part\n");
        light_part->get_distribution(dist);
        // printf("*** Finish getting distribution in light part\n");

        // step 1. collecting all items in Heavy Part
        set<uint32_t> checked_items;
        for (int i = 0; i < FCMPLUS_HEAVY_STAGE; ++i){
            for (int j = 0; j < FCMPLUS_BUCKET; ++j){
                uint32_t key = heavy_part[i]->buckets[j].key;
                if (key != 0)
                    checked_items.insert(key);
            }
        }
 
        // step 2. Merge heavy & light entropy
        for (auto it = checked_items.begin(); it != checked_items.end(); ++it){
            uint32_t sum_val = 0;
            // heavy part query
            uint32_t heavy_val = 0;
            uint8_t key[4]; // temp key
            for (int i = 0; i < FCMPLUS_HEAVY_STAGE; ++i){
                *(uint32_t*)key = *it;
                int pos = hash_heavy[i]->run((const char*)key, 4) % FCMPLUS_BUCKET;
                heavy_val += heavy_part[i]->query(key, pos);
            }
            sum_val += heavy_val;

            // light part query
            uint32_t light_val = light_part->query(key);

            if (light_val != 0){
                sum_val += light_val;
                dist[light_val]--;
            }
            if (sum_val){
                if (sum_val + 1 > dist.size())
                    dist.resize(sum_val + 1);
                dist[sum_val]++;
            }
        }
    }
	
};

#endif
