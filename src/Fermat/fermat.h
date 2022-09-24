#ifndef _FERMAT_H_
#define _FERMAT_H_

#include <iostream>
#include <cstdint>
#include <unordered_map>
#include <queue>
#include <cstring>
#include "../Common/BOBHash32.h"
#include "util/mod.h"
#include "util/prime.h"
#include "murmur3.h"

using namespace std;

#define DEBUG_F 0

// fingprint no used

// use a 16-bit prime, so 2 * a mod PRIME will not overflow
static const uint32_t PRIME_ID = MAXPRIME[32];
static const uint32_t PRIME_FING = MAXPRIME[32];

inline uint64_t checkTable(uint64_t pos)
{
    return powMod32(pos, PRIME_ID - 2, PRIME_ID);
}

class Fermat
{
    // arrays
    int array_num;
    int entry_num;
    uint32_t **id;
    uint32_t **fingerprint;
    uint32_t **counter;
    uint32_t **idcpy, **fingcpy, **countercpy;
    int decodeflag = 0;
    // hash
    BOBHash32 *hash;
    BOBHash32 *hash_fp;

    uint32_t *table;

    bool use_fing;

public:
    int pure_cnt;

    void clear_look_up_table()
    {
        delete[] table;
    }

    void create_array()
    {
        pure_cnt = 0;
        // id
        id = new uint32_t *[array_num];
        for (int i = 0; i < array_num; ++i)
        {
            id[i] = new uint32_t[entry_num];
            memset(id[i], 0, entry_num * sizeof(uint32_t));
        }
        // fingerprint
        if (use_fing)
        {
            fingerprint = new uint32_t *[array_num];
            for (int i = 0; i < array_num; ++i)
            {
                fingerprint[i] = new uint32_t[entry_num];
                memset(fingerprint[i], 0, entry_num * sizeof(uint32_t));
            }
        }

        // counter
        counter = new uint32_t *[array_num];
        for (int i = 0; i < array_num; ++i)
        {
            counter[i] = new uint32_t[entry_num];
            memset(counter[i], 0, entry_num * sizeof(uint32_t));
        }
    }

    void clear_array()
    {
        for (int i = 0; i < array_num; ++i)
            delete[] id[i];
        delete[] id;

        if (fingerprint)
        {
            for (int i = 0; i < array_num; ++i)
                delete[] fingerprint[i];
            delete[] fingerprint;
        }

        for (int i = 0; i < array_num; ++i)
            delete[] counter[i];
        delete[] counter;
    }

    Fermat(int _a, int _e, bool _fing, uint32_t _init) : array_num(_a), entry_num(_e), use_fing(_fing), fingerprint(nullptr), hash_fp(nullptr)
    {
        create_array();
        // hash
        if (use_fing)
            hash_fp = new BOBHash32(_init);
        hash = new BOBHash32[array_num];
        for (int i = 0; i < array_num; ++i)
        {
            hash[i].initialize(_init + (10 * i) + 1);
        }
    }

    Fermat(int _memory, bool _fing, uint32_t _init) : use_fing(_fing), fingerprint(nullptr), hash_fp(nullptr)
    {
        array_num = 3;
        if (use_fing)
            entry_num = _memory / (array_num * 12);
        else
            entry_num = _memory / (array_num * 8);

        // cout << "construct fermat with " << entry_num << " entry" << endl;
        create_array();

        // hash
        if (use_fing)
            hash_fp = new BOBHash32(_init);
        hash = new BOBHash32[array_num];
        for (int i = 0; i < array_num; ++i)
            hash[i].initialize(_init + i + 1);
    }

    ~Fermat()
    {
        clear_array();
        clear_look_up_table();
        if (hash_fp)
            delete hash_fp;
        delete[] hash;
    }

    void Insert(uint32_t flow_id, uint32_t cnt)
    {
        if (use_fing)
        {
            uint32_t fing = hash_fp->run((char *)&flow_id, sizeof(uint32_t));
            for (int i = 0; i < array_num; ++i)
            {
                uint32_t pos = hash[i].run((char *)&flow_id, sizeof(uint32_t)) % entry_num;
                id[i][pos] = (mulMod32(flow_id, cnt, PRIME_ID) + (uint64_t)id[i][pos]) % PRIME_ID;
                fingerprint[i][pos] = ((uint64_t)fingerprint[i][pos] + mulMod32(fing, cnt, PRIME_FING)) % PRIME_FING;
                counter[i][pos] += cnt;
            }
        }
        else
        {
            for (int i = 0; i < array_num; ++i)
            {
                uint32_t pos = hash[i].run((char *)&flow_id, sizeof(uint32_t)) % entry_num;
                id[i][pos] = (mulMod32(flow_id, cnt, PRIME_ID) + (uint64_t)id[i][pos]) % PRIME_ID;
                counter[i][pos] += cnt;
            }
        }
    }

    void Insert_one(uint32_t flow_id)
    {
        // flow_id should < PRIME_ID
        if (use_fing)
        {
            uint32_t fing = hash_fp->run((char *)&flow_id, sizeof(uint32_t)) % PRIME_FING;
            for (int i = 0; i < array_num; ++i)
            {
                uint32_t pos = hash[i].run((char *)&flow_id, sizeof(uint32_t)) % entry_num;
                id[i][pos] = ((uint64_t)id[i][pos] + (uint64_t)(flow_id % PRIME_ID)) % PRIME_ID;
                fingerprint[i][pos] = ((uint64_t)fingerprint[i][pos] + (uint64_t)fing) % PRIME_FING;
                counter[i][pos]++;
            }
        }
        else
        {
            for (int i = 0; i < array_num; ++i)
            {
                uint32_t pos = hash[i].run((char *)&flow_id, sizeof(uint32_t)) % entry_num;
                id[i][pos] = ((uint64_t)id[i][pos] + (uint64_t)(flow_id % PRIME_ID)) % PRIME_ID;
                counter[i][pos]++;
            }
        }
    }

    void Delete_in_one_bucket(int row, int col, int pure_row, int pure_col)
    {
        // delete (flow_id, fing, cnt) in bucket (row, col)
        id[row][col] = ((uint64_t)PRIME_ID + (uint64_t)id[row][col] - (uint64_t)id[pure_row][pure_col]) % PRIME_ID;
        if (use_fing)
            fingerprint[row][col] = ((uint64_t)PRIME_FING + (uint64_t)fingerprint[row][col] - (uint64_t)fingerprint[pure_row][pure_col]) % PRIME_FING;
        counter[row][col] -= counter[pure_row][pure_col];
    }

    bool verify(int row, int col, uint32_t &flow_id, uint32_t &fing)
    {
#if DEBUG_F
        ++pure_cnt;
#endif
        if (counter[row][col] & 0x80000000)
        {
            uint64_t temp = checkTable(~counter[row][col] + 1);
            flow_id = mulMod32(PRIME_ID - id[row][col], temp, PRIME_ID);
        }
        else
        {
            uint64_t temp = checkTable(counter[row][col]);
            flow_id = mulMod32(id[row][col], temp, PRIME_ID);
        }
        // flow_id = (id[row][col] * table[counter[row][col] % PRIME_ID]) % PRIME_ID;
        if (use_fing)
        {
            fing = powMod32(counter[row][col], PRIME_FING - 2, PRIME_FING);
            fing = mulMod32(fingerprint[row][col], fing, PRIME_FING);
        }
        if (!(hash[row].run((char *)&flow_id, sizeof(uint32_t)) % entry_num == col))
            return false;
        if (use_fing && !(hash_fp->run((char *)&flow_id, sizeof(uint32_t)) % PRIME_FING == fing))
            return false;
        return true;
    }

    void display()
    {
        cout << " --- display --- " << endl;
        for (int i = 0; i < array_num; ++i)
        {
            for (int j = 0; j < entry_num; ++j)
            {
                if (counter[i][j])
                {
                    cout << i << "," << j << ":" << counter[i][j] << endl;
                }
            }
        }
    }
    int query(const char *key)
    {
        uint32_t flow_id = *(uint32_t *)key;
        uint32_t ret = 1 << 30;
        if (decodeflag)
        {
            for (int i = 0; i < array_num; ++i)
            {
                uint32_t pos = hash[i].run((char *)&flow_id, sizeof(uint32_t)) % entry_num;
                ret = min(counter[i][pos], ret);
            }
        }
        return (int)ret;
    }
    bool Decode(unordered_map<uint32_t, int> &result)
    {
        idcpy = new uint32_t *[array_num];
        for (int i = 0; i < array_num; i++)
        {
            idcpy[i] = new uint32_t[entry_num];
            for (int j = 0; j < entry_num; j++)
                idcpy[i][j] = id[i][j];
        }
        if (use_fing)
        {
            fingcpy = new uint32_t *[array_num];
            for (int i = 0; i < array_num; i++)
            {
                fingcpy[i] = new uint32_t[entry_num];
                for (int j = 0; j < entry_num; j++)
                    fingcpy[i][j] = fingerprint[i][j];
            }
        }
        countercpy = new uint32_t *[array_num];
        for (int i = 0; i < array_num; i++)
        {
            countercpy[i] = new uint32_t[entry_num];
            for (int j = 0; j < entry_num; j++)
                countercpy[i][j] = counter[i][j];
        }
        decodeflag = 1;
        queue<int> *candidate = new queue<int>[array_num];
        uint32_t flow_id = 0;
        uint32_t fing = 0;

        // first round
        for (int i = 0; i < array_num; ++i)
            for (int j = 0; j < entry_num; ++j)
            {
                if (counter[i][j] == 0)
                {
                    continue;
                }
                else if (verify(i, j, flow_id, fing))
                {
                    // find pure bucket
                    if (result.count(flow_id) != 0)
                    {
                        result[flow_id] += counter[i][j];
                    }
                    else
                    {
                        result[flow_id] = counter[i][j];
                    }
                    // delete flow from other rows
                    for (int t = 0; t < array_num; ++t)
                    {
                        if (t == i)
                            continue;
                        uint32_t pos = hash[t].run((char *)&flow_id, sizeof(uint32_t)) % entry_num;
                        Delete_in_one_bucket(t, pos, i, j);
                        candidate[t].push(pos);
                    }
                    Delete_in_one_bucket(i, j, i, j);
                }
            }

        bool pause;
        int acc = 0;
        while (true)
        {
            acc++;
            pause = true;
            for (int i = 0; i < array_num; ++i)
            {
                if (!candidate[i].empty())
                    pause = false;
                while (!candidate[i].empty())
                {
                    int check = candidate[i].front();
                    candidate[i].pop();
                    // cout << i << " " << check << endl;
                    if (counter[i][check] == 0)
                    {
                        continue;
                    }
                    else if (verify(i, check, flow_id, fing))
                    {
                        // find pure bucket
                        if (result.count(flow_id) != 0)
                        {
                            result[flow_id] += counter[i][check];
                        }
                        else
                        {
                            result[flow_id] = counter[i][check];
                        }
                        // delete flow from other rows
                        for (int t = 0; t < array_num; ++t)
                        {
                            if (t == i)
                                continue;
                            uint32_t pos = hash[t].run((char *)&flow_id, sizeof(uint32_t)) % entry_num;
                            Delete_in_one_bucket(t, pos, i, check);
                            candidate[t].push(pos);
                        }
                        Delete_in_one_bucket(i, check, i, check);
                    }
                }
            }
            if (pause)
                break;
            if (acc > 10000)
                break;
        }

        delete[] candidate;
        bool flag = true;
        for (int i = 0; i < array_num; ++i)
            for (int j = 0; j < entry_num; ++j)
                if (counter[i][j] != 0)
                {
                    // cout << "undecoded  i " << i << " j " << j << endl;
                    // cout << counter[i][j] << endl;
                    flag = false;
                }
        for (auto p : result)
        {
            if (p.second == 0)
            {
                result.erase(p.first);
            }
        }
        return flag;
    }
};

#endif