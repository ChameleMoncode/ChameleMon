#ifndef _LOSSRADAR_H_
#define _LOSSRADAR_H_

#include <iostream>
#include <cstdint>
#include <unordered_map>
#include <queue>
#include <cstring>
#include "util/BOBHash32.h"

using namespace std;

#define DEBUG_LR 0

class LossRadar {
    // array
    uint32_t *xorSum;
    uint16_t *xorNo;
    uint32_t *countSum;
    int array_len;
    int hash_len;

    // hash
    BOBHash32 *hashes;
    int hashNum;
public:
    int pure_cnt;
    // LossRadar(int _hashNum, int _len, int _init) {
    //     array_len = _len;
    //     hashNum = _hashNum;
    //     hash_len = array_len / hashNum;

    //     xorSum = new uint64_t[array_len];
    //     memset(xorSum, 0, array_len * sizeof(uint64_t));
    //     countSum = new uint32_t[array_len];
    //     memset(countSum, 0, array_len * sizeof(uint64_t));
    //     hashes = new BOBHash32[hashNum];
    //     for (int i = 0; i < hashNum; ++i)
    //         hashes[i].initialize(_init + i);
    // }

    LossRadar(int _hashNum, int _mem, int _init) {
        pure_cnt = 0;
        array_len = _mem / 10;
        // cout << "construct lossradar with " << array_len << " bucket" << endl;
        hashNum = _hashNum;
        hash_len = array_len / hashNum;
        
        xorSum = new uint32_t[array_len];
        memset(xorSum, 0, array_len * sizeof(uint32_t));
        xorNo = new uint16_t[array_len];
        memset(xorNo, 0, sizeof(uint16_t) * array_len);
        countSum = new uint32_t[array_len];
        memset(countSum, 0, array_len * sizeof(uint32_t));

        hashes = new BOBHash32[hashNum];

        for (int i = 0; i < hashNum; ++i)
            hashes[i].initialize(_init + i);

    }

    ~LossRadar() {
        delete [] xorSum;
        delete [] xorNo;
        delete [] countSum;
        delete [] hashes; 
    }

    void Insert_id_seq(uint32_t id, uint16_t seq) {
        char p[6] = { 0 };
        memcpy(p, &id, sizeof(uint32_t));
        memcpy(p + sizeof(uint32_t), &seq, sizeof(uint16_t));
        for (int i = 0; i < hashNum; ++i) {
            uint32_t pos = hashes[i].run(p, 6);
            pos = pos % hash_len + i * hash_len;
            xorSum[pos] ^= id;
            xorNo[pos] ^= seq;
            countSum[pos]++;
        }
    }

    void Insert_range_data(uint32_t id, uint16_t count) {
        for (uint16_t i = 0; i < count; ++i)
            Insert_id_seq(id, i);
    }

    bool Decode(unordered_map<uint32_t, int> &result) {
        // decode
        queue<int> candidate;
        char p[6];
        
        // first round
        for (int i = 0; i < array_len; ++i) {
            if (countSum[i] == 1) {
                #if DEBUG_LR
                pure_cnt++;
                #endif
                uint32_t id = xorSum[i];
                uint16_t seq = xorNo[i];
                // delete from lossradar
                for (uint32_t j = 0; j < hashNum; ++j) {
                    memcpy(p, &id, sizeof(uint32_t));
                    memcpy(p + sizeof(uint32_t), &seq, sizeof(uint16_t));
                    uint32_t pos = hashes[j].run(p, 6);
                    pos = pos % hash_len + j * hash_len;
                    xorSum[pos] ^= id;
                    xorNo[pos] ^= seq;
                    --countSum[pos];
                    if (pos < i && countSum[pos] == 1)
                        candidate.push(pos);
                }
                // insert in result

                if (result.find(id) == result.end())
                    result[id] = 1;
                else
                    ++result[id];
            }
        }

        while (!candidate.empty()) {
            int i = candidate.front();
            candidate.pop();
            if (countSum[i] == 1) {
                #if DEBUG_LR
                pure_cnt++;
                #endif
                uint32_t id = xorSum[i];
                uint16_t seq = xorNo[i];
                for (int j = 0; j < hashNum; ++j) {
                    memcpy(p, &id, sizeof(uint32_t));
                    memcpy(p + sizeof(uint32_t), &seq, sizeof(uint16_t));
                    uint32_t pos = hashes[j].run(p, 6);
                    pos = pos % hash_len + j * hash_len;
                    xorSum[pos] ^= id;
                    xorNo[pos] ^= seq;
                    --countSum[pos];
                    if (countSum[pos] == 1) 
                        candidate.push(pos);
                }

                if (result.find(id) == result.end())
                    result[id] = 1;
                else 
                    ++result[id];
            }
        }

        bool success = true;
        for (int i = 0; i < array_len; ++i)
            if (countSum[i]) {
                success = false;
                break;
            }

        return success;
    }

};

#endif