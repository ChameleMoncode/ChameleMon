#ifndef _ELASTIC_P4_H_
#define _ELASTIC_P4_H_

#include "./our_heavypart.h"
#include "./LightPart.h"

class ElasticSketch
{
    public:
    HeavyPart * heavy_part[ELASTIC_HEAVY_STAGE] = {NULL};
    LightPart * light_part = NULL;
    BOBHash32* hash_heavy[ELASTIC_HEAVY_STAGE] = {NULL};

public:
    ElasticSketch(){
        int init = INIT;
        for (int i = 0; i < ELASTIC_HEAVY_STAGE; ++i){
            heavy_part[i] = new HeavyPart();
            hash_heavy[i] = new BOBHash32(init+i);
        }
        light_part = new LightPart();
        clear();
        // printf("[ElasticP4_Simulator] Memory Usage : %2.2f MB\n", (ELASTIC_WL + ELASTIC_HEAVY_STAGE*ELASTIC_BUCKET * 12.0) / 1024 / 1024);
        // if (ELASTIC_TOFINO)
        //     printf("[ElasticP4_Tofino] Memory Usage : %2.2f MB\n", (ELASTIC_WL + ELASTIC_HEAVY_STAGE*ELASTIC_BUCKET * 28.0) / 1024 / 1024);
        // else
        //     printf("[ElasticP4_Simulator] Memory Usage : %2.2f MB\n", (ELASTIC_WL + ELASTIC_HEAVY_STAGE*ELASTIC_BUCKET * 12.0) / 1024 / 1024);
    }
    ~ElasticSketch(){
        delete light_part;
        for (int i = 0; i < ELASTIC_HEAVY_STAGE; ++i){
            delete heavy_part[i];
            delete hash_heavy[i];
        }
    }
    void clear()
    {
        for (int i = 0; i < ELASTIC_HEAVY_STAGE; ++i)
            heavy_part[i]->clear();
        light_part->clear();
    }

    void insert(uint8_t *key, int f=1)
    {
        uint8_t swap_key[4]; // metafield for key
        uint32_t swap_val = 0; // metafield for value
        int pos;
        int result;
        int val = f;
        for (int i = 0; i < ELASTIC_HEAVY_STAGE; ++i){
            pos = hash_heavy[i]->run((const char *)key, 4) % ELASTIC_BUCKET;
            result = heavy_part[i]->insert(key, swap_key, swap_val, pos, val);

            switch(result) {
                case 0: return;
                case 1:{ // key, f
                    swap_val = 1;
                    continue;
                }
                case 2:{ // swap_key, swap_val
                    std::copy(swap_key, swap_key + 4, key);
                    continue;
                }
            }
        }
        // if (swap_val)
        //     cout << "[Elastic] swap_val = " << swap_val << endl; 
        light_part->insert(key, swap_val);
        return;
    }

    uint32_t query(uint8_t *key) // 4 bytes prefix
    {
        uint32_t ret_val = 0;
        int pos;
        for (int i = 0; i < ELASTIC_HEAVY_STAGE; ++i){
            pos = hash_heavy[i]->run((const char*)key, 4) % ELASTIC_BUCKET;
            ret_val += heavy_part[i]->query(key, pos);
        }
        ret_val += light_part->query(key);
        return ret_val;
    }

    void get_heavy_hitters(set<uint32_t> & results)
    {
        for (int i = 0; i < ELASTIC_HEAVY_STAGE; ++i){
            for (int j = 0; j < ELASTIC_BUCKET; ++j) {
                uint32_t key = heavy_part[i]->buckets[j].key;
                uint32_t val = this->query((uint8_t *)&key);
                if (val > HH_THRESHOLD)
                    results.insert(key);
            }
        }
    }
    void get_hc_candidates(unordered_map<uint32_t, int>& hc_candidates)
    {
        for (int i = 0; i < ELASTIC_HEAVY_STAGE; ++i){
            for (int j = 0; j < ELASTIC_BUCKET; ++j) {
                uint32_t key = heavy_part[i]->buckets[j].key;
                uint32_t val = this->query((uint8_t *)&key);
                if (val > HC_THRESHOLD)
                    hc_candidates[key] = val;
            }
        }
    }

/* interface */
    int get_cardinality()
    {

        int num_light_nonzero = 0;

        int card = light_part->get_cardinality();
        // step 1. collecting all items in Heavy Part
        set<uint32_t> checked_items;
        for (int i = 0; i < ELASTIC_HEAVY_STAGE; ++i){
            for (int j = 0; j < ELASTIC_BUCKET; ++j){
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
            for (int i = 0; i < ELASTIC_HEAVY_STAGE; ++i){
                *(uint32_t*)key = *it;
                int pos = hash_heavy[i]->run((const char*)key, 4) % ELASTIC_BUCKET;
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

    double get_entropy()
    {
        int tot = 0;
        double entr = 0;

        light_part->get_entropy(tot, entr);

        // step 1. collecting all items in Heavy Part
        set<uint32_t> checked_items;
        for (int i = 0; i < ELASTIC_HEAVY_STAGE; ++i){
            for (int j = 0; j < ELASTIC_BUCKET; ++j){
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
            for (int i = 0; i < ELASTIC_HEAVY_STAGE; ++i){
                *(uint32_t*)key = *it;
                int pos = hash_heavy[i]->run((const char*)key, 4) % ELASTIC_BUCKET;
                heavy_val += heavy_part[i]->query(key, pos);
            }
            sum_val += heavy_val;

            // light part query
            uint32_t light_val = light_part->query(key);

            if (light_val != 0){
                sum_val += light_val;
                tot -= light_val;
                entr -= light_val * log2(light_val);
            }
            if (sum_val){
                tot += sum_val;
                entr += sum_val * log2(sum_val);
            }
        }
        return -entr / tot + log2(tot);
    }

    void get_distribution(vector<double> &dist)
    {
        // printf("*** Start getting distribution in light part\n");
        light_part->get_distribution(dist);
        
        // printf("*** Finish getting distribution in light part\n");

        // step 1. collecting all items in Heavy Part
        set<uint32_t> checked_items;
        for (int i = 0; i < ELASTIC_HEAVY_STAGE; ++i){
            for (int j = 0; j < ELASTIC_BUCKET; ++j){
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
            for (int i = 0; i < ELASTIC_HEAVY_STAGE; ++i){
                *(uint32_t*)key = *it;
                int pos = hash_heavy[i]->run((const char*)key, 4) % ELASTIC_BUCKET;
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
