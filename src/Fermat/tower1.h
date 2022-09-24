#ifndef _TOWER_H
#define _TOWER_H

#include <cstring>
#include "murmur3.h"
#include "../Common/EMFSD.h"
#include <random>
#include "../common_func.h"
const uint32_t d = 3;
const uint32_t cs[] = {1, 2, 3, 4, 5};
const uint32_t cpw[] = {4, 3, 2, 1, 0};
const uint32_t lo[] = {0xf, 0x7, 0x3, 0x1, 0x0};
const uint32_t mask[] = {0x3, 0xf, 0xff, 0xffff, 0xffffffff};

class TowerSketch
{
public:
	uint32_t w[d];
	uint32_t *A[d];
	uint32_t hashseed[d];
	int idx[d];
	int wd;
	int threshold = 256;

public:
	TowerSketch() {}
	TowerSketch(uint32_t w_d) { init(w_d); }
	~TowerSketch() { clear(); }

	void init(uint32_t w_d)
	{
		for (int i = 0; i < d; ++i)
		{
			w[i] = w_d << d - i + 1;
			A[i] = new uint32_t[w_d];
			memset(A[i], 0, w_d * sizeof(uint32_t));
			hashseed[i] = rand() % MAX_PRIME32;
		}
		wd = w_d;
	}

	void clear()
	{
		for (int i = 0; i < d; ++i)
			delete[] A[i];
	}

	uint32_t insert(const char *key)
	{
		uint32_t ret = UINT32_MAX;
		for (int i = 0; i < d; ++i)
			idx[i] = MurmurHash3_x86_32(key, 4, hashseed[i]) % w[i];
		for (int i = 0; i < d; ++i)
		{
			uint32_t &a = A[i][idx[i] >> cpw[i]];
			uint32_t shift = (idx[i] & lo[i]) << cs[i];
			uint32_t val = (a >> shift) & mask[i];
			if(val < ret)
				ret = val;
			a += (val < mask[i]) ? (1 << shift) : 0;
		}
		return ret;
	}
	int get_cardinality()
	{
		int empty = 0;
		for (int i = 0; i < w[0]; i++)
		{
			uint32_t &a = A[0][i >> cpw[0]];
			uint32_t shift = (i & lo[0]) << cs[0];
			uint32_t val = (a >> shift) & mask[0];
			if (!val)
				empty++;
		}
		return (int)((double)w[0] * log((double)w[0] / (double)empty));
	}
	void get_distribution(vector<double> &dist)
	{
		uint32_t *countercpy = new uint32_t[w[2]];
		for (int i = 0; i < w[2]; i++)
		{
			uint32_t &a = A[2][i >> cpw[2]];
			uint32_t shift = (i & lo[2]) << cs[2];
			uint32_t val = (a >> shift) & mask[2];
			countercpy[i] = val;
		}
		EMFSD *em = new EMFSD();
		em->set_counters(w[2], countercpy);
		for (int i = 0; i < FERMAT_EM_ITER; i++){
			em->next_epoch();
			printf("%d epoch\n",i);
			fflush(stdout);
		}
		dist.resize(em->ns.size());
		for (int i = 1; i < em->ns.size(); i++)
			dist[i] = em->ns[i];
		return;
	}
	uint32_t query(const char *key)
	{
		uint32_t ret = 255;
		for (int i = 0; i < d; ++i)
		{
			uint32_t idx = MurmurHash3_x86_32(key, 4, hashseed[i]) % w[i];
			uint32_t a = A[i][idx >> cpw[i]];
			uint32_t shift = (idx & lo[i]) << cs[i];
			uint32_t val = (a >> shift) & mask[i];
			ret = (val < mask[i] && val < ret) ? val : ret;
		}
		return ret;
	}
};

class TowerSketchCU : public TowerSketch
{
public:
	TowerSketchCU() {}
	TowerSketchCU(uint32_t w_d) { init(w_d); }
	~TowerSketchCU() {}

	void insert(const char *key, uint16_t key_len)
	{
		uint32_t min_val = UINT32_MAX;
		for (int i = 0; i < d; ++i)
			idx[i] = MurmurHash3_x86_32(key, key_len, hashseed[i]) % w[i];
		for (int i = 0; i < d; ++i)
		{
			uint32_t a = A[i][idx[i] >> cpw[i]];
			uint32_t shift = (idx[i] & lo[i]) << cs[i];
			uint32_t val = (a >> shift) & mask[i];
			min_val = (val < mask[i] && val < min_val) ? val : min_val;
		}
		for (int i = 0; i < d; ++i)
		{
			uint32_t &a = A[i][idx[i] >> cpw[i]];
			uint32_t shift = (idx[i] & lo[i]) << cs[i];
			uint32_t val = (a >> shift) & mask[i];
			a += (val < mask[i] && val == min_val) ? (1 << shift) : 0;
		}
	}
};

#endif