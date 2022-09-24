#ifndef _ELASTIC_HEAVYPART_H_
#define _ELASTIC_HEAVYPART_H_

#include "../common_func.h"
#include "../Common/BOBHash32.h"

class HeavyPart
{
public:
	Bucket buckets[ELASTIC_BUCKET];

	HeavyPart(){ clear(); }
	~HeavyPart(){}

	void clear(){
		memset(buckets, 0, 12 * ELASTIC_BUCKET);
	}

/* insertion */
	int insert(uint8_t *key, uint8_t *swap_key, uint32_t &swap_val, int pos, int f = 1)
	{
		// return 0 if do nothing afterall
		// return 1 if update next using key, f
		// return 2 if update next using swap_key, swap_val

		// We emulate the implementation of software version ElasticSketch.
		// For details, refer ElasticSketch's Github
		// https://github.com/BlockLiu/ElasticSketchCode/blob/master/src/CPU/elastic/HeavyPart-noSIMD.h

		buckets[pos].guard_val += f;
		
		/* uint32_t to uint8_t array */
		uint8_t swap_key_arr[4];
		*(uint32_t*)swap_key_arr = buckets[pos].key;


		if (buckets[pos].key == *((uint32_t*)key)){ // if matched
			buckets[pos].val += f;
			return 0;
		}
		// If non-matched item and empty bucket, then insert
		if (buckets[pos].key == 0){
			buckets[pos].key = *((uint32_t*)key);
			buckets[pos].val += f;
			return 0;
		}
		// If non-matched item and non-empty : check swap or not
		if (!JUDGE_IF_SWAP_ELASTIC_P4(buckets[pos].val, buckets[pos].guard_val)){
			return 1; // if not swap
		}

		// swap
		std::copy(swap_key_arr, swap_key_arr + 4, swap_key); // swap key
		swap_val = buckets[pos].val; // swap val
		buckets[pos].key = *((uint32_t*)key);
		buckets[pos].val += f;
		return 2;
	}

/* query */
	uint32_t query(uint8_t *key, int pos)
	{
		if (buckets[pos].key == *((uint32_t*)key))
			return buckets[pos].val;
		return 0;
	}
};

#endif

