#ifndef _ELASTIC_LIGHT_PART_H_
#define _ELASTIC_LIGHT_PART_H_

#include "../Common/BOBHash32.h"
#include "../Common/EMFSD.h"
#include "../common_func.h"

class LightPart
{
	static constexpr int counter_num = ELASTIC_WL;
	BOBHash32 *bobhash = NULL;

public:
	uint8_t * counters;
	int mice_dist[256];
	EMFSD *em_fsd_algo = NULL;

	LightPart()
	{
        counters = new uint8_t[counter_num];
       	bobhash = new BOBHash32(INIT);
        //printf("[ElasticP4_Simulator] Light counter number : %d\n", counter_num);
        clear();
	}

	~LightPart()
	{
		delete bobhash;
        delete counters;
	}

	void clear()
	{
		memset(counters, 0, counter_num);
		memset(mice_dist, 0, sizeof(int) * 256);
	}


/* insertion */
	void insert(uint8_t *key, int f = 1)
	{
		uint32_t hash_val = (uint32_t)bobhash->run((const char*)key, 4);
		uint32_t pos = hash_val % (uint32_t)counter_num;

		int old_val = (int)counters[pos];
        int new_val = (int)counters[pos] + f;

        new_val = new_val < 255 ? new_val : 255;
        counters[pos] = (uint8_t)new_val;

        mice_dist[old_val]--;
        mice_dist[new_val]++;
	}

/* query */
	int query(uint8_t *key)
	{
        uint32_t hash_val = (uint32_t)bobhash->run((const char*)key, 4);
        uint32_t pos = hash_val % (uint32_t)counter_num;

        return (int)counters[pos];
    }

/* other measurement task */
   	int get_cardinality()
   	{
		int mice_card = 0;
        for (int i = 1; i < 256; i++)
			mice_card += mice_dist[i];

        double rate = (counter_num - mice_card) / (double)counter_num;
        return (int)(counter_num * log(1 / rate));
    }

    void get_entropy(int &tot, double &entr)
    {
        for (int i = 1; i < 256; i++)
        {
            tot += mice_dist[i] * i;
			entr += mice_dist[i] * i * log2(i);
		}
    }

    void get_distribution(vector<double> &dist)
    {
        uint32_t * tmp_counters = new uint32_t[counter_num];
		for (int i = 0; i < counter_num; i++)
			tmp_counters[i] = counters[i];

        em_fsd_algo = new EMFSD();
        em_fsd_algo->set_counters(counter_num, tmp_counters);

        for (int i = 0; i < ELASTIC_EM_ITER; ++i){
            em_fsd_algo->next_epoch();
            printf("[EMFSD] Iteration %d is finished...\n", i);
        }

        dist = em_fsd_algo->ns;

        delete em_fsd_algo;
        delete tmp_counters;
    }
};





#endif
