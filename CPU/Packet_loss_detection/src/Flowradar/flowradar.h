#ifndef _FLOWRADAR_H_
#define _FLOWRADAR_H_

#include <iostream>
#include <cstdint>
#include <queue>
#include <unordered_map>
#include <cstring>
#include "util/BOBHash32.h"

using namespace std;

#define DEBUG_FR 0

class BitMap {
    int len;
    uint8_t *bitmap;
public:
    BitMap(int bit_num) {
        len = bit_num / 8 + 1;
        bitmap = new uint8_t[len];
    }
    ~BitMap() {
        delete [] bitmap;
    }
    void set(int pos) {
        int x = pos / 8, y = pos % 8;
        bitmap[x] |= (0x1 << y);
    }
    bool test(int pos) {
        int x = pos / 8, y = pos % 8;
        return (bitmap[x] & (0x1 << y)) != 0;
    }
};

class FlowFilter {
    int bit_num;
    int F;
    BitMap bloomfilter;
    BOBHash32 *hashes;
public:
    FlowFilter(int _num, int f, int _init) : bit_num(_num), F(f), bloomfilter(_num) {
        hashes = new BOBHash32[F];
        for (int i = 0; i < F; ++i)
            hashes[i].initialize(_init + i);
    }
    ~FlowFilter() {
        delete [] hashes;
    }
    void add(uint32_t flow_id) {
        for (int i = 0; i < F; ++i) {
            uint32_t pos = hashes[i].run((char*)&flow_id, sizeof(uint32_t)) % bit_num;
            bloomfilter.set(pos);
        }
    }
    bool query(uint32_t flow_id) {
        for (int i = 0; i < F; ++i) {
            uint32_t pos = hashes[i].run((char*)&flow_id, sizeof(uint32_t)) % bit_num;
            if (!bloomfilter.test(pos))
                return false;
        }
        return true;
    }
};

class FlowRadar {
    FlowFilter *flowfilter;
    
    int array_len;
    uint32_t *flow_xor;
    uint32_t *flow_count;
    uint32_t *packet_count;

    int C; // hash number in FlowRadar
    int hash_len;
    BOBHash32 *hashes;
public:
    int pure_cnt;
    FlowRadar(int _len, int c, int bit_num, int f, int _init) : array_len(_len), C(c) {
        pure_cnt = 0;
        // flowfilter
        flowfilter = new FlowFilter(bit_num, f, _init + C);
        
        hash_len = array_len / C;
        flow_xor = new uint32_t[array_len];
        memset(flow_xor, 0, sizeof(uint32_t) * array_len);
        flow_count = new uint32_t[array_len];
        memset(flow_count, 0, sizeof(uint32_t) * array_len);
        packet_count = new uint32_t[array_len];
        memset(packet_count, 0, sizeof(uint32_t) * array_len);
        hashes = new BOBHash32[C];
        for (int i = 0; i < C; ++i) 
            hashes[i].initialize(_init + i);
    }

    FlowRadar(int _mem, int c, int _init) : C(c) {
        pure_cnt = 0;
        // bitmap
        int bit_num = _mem * 0.1 * 8;
        flowfilter = new FlowFilter(bit_num, 10, _init + C);

        array_len = _mem * 0.9 / 12;
        // cout << "construct flowradar with " << bit_num << " bloomfilter and " << array_len << " bucket" << endl;
        hash_len = array_len / C;
        flow_xor = new uint32_t[array_len];
        memset(flow_xor, 0, sizeof(uint32_t) * array_len);
        flow_count = new uint32_t[array_len];
        memset(flow_count, 0, sizeof(uint32_t) * array_len);
        packet_count = new uint32_t[array_len];
        memset(packet_count, 0, sizeof(uint32_t) * array_len);
        hashes = new BOBHash32[C];
        for (int i = 0; i < C; ++i) 
            hashes[i].initialize(_init + i);
    }
    ~FlowRadar() {
        delete [] flow_xor;
        delete [] flow_count;
        delete [] packet_count;
        delete [] hashes;
        delete flowfilter;
    }

    void Insert(uint32_t flow_id) {
        if (!flowfilter->query(flow_id)) {
            flowfilter->add(flow_id);
            for (int i = 0; i < C; ++i) {
                uint32_t pos = hashes[i].run((char*)&flow_id, sizeof(uint32_t));
                pos = pos % hash_len + i * hash_len;
                flow_xor[pos] = flow_xor[pos] ^ flow_id;
                ++flow_count[pos];
                ++packet_count[pos];
            }
        }
        else {
            for (int i = 0; i < C; ++i) {
                uint32_t pos = hashes[i].run((char*)&flow_id, sizeof(uint32_t));
                pos = pos % hash_len + i * hash_len;
                ++packet_count[pos];
            }
        }
    }

    bool SingleDecode(unordered_map<uint32_t, int> &result) {
        queue<int> candidate;
        // first round
        for (int i = 0; i < array_len; ++i) {
            if (flow_count[i] == 1) {
                #if DEBUG_FR
                ++pure_cnt;
                #endif
                // find pure bucket
                uint32_t flow_id = flow_xor[i];
                uint32_t count = packet_count[i];
                result[flow_id] = count;
                // delete flow from other bucket
                for (int j = 0; j < C; ++j) {
                    uint32_t pos = hashes[j].run((char*)&flow_id, sizeof(uint32_t));
                    pos = pos % hash_len + j * hash_len;
                    flow_xor[pos] ^= flow_id;
                    --flow_count[pos];
                    packet_count[pos] -= count;
                    if (pos < i && flow_count[pos] == 1)
                        candidate.push(pos);
                }
            }
        }

        while (!candidate.empty()) {
            int i = candidate.front();
            candidate.pop();
            if (flow_count[i] == 1) {
                #if DEBUG_FR
                ++pure_cnt;
                #endif
                // find pure bucket
                uint32_t flow_id = flow_xor[i];
                uint32_t count = packet_count[i];
                result[flow_id] = count;
                // delete flow from other bucket
                for (int j = 0; j < C; ++j) {
                    uint32_t pos = hashes[j].run((char*)&flow_id, sizeof(uint32_t));
                    pos = pos % hash_len + j * hash_len;
                    flow_xor[pos] ^= flow_id;
                    --flow_count[pos];
                    packet_count[pos] -= count;
                    if (flow_count[pos] == 1)
                        candidate.push(pos);
                }
            }
        }

        bool success = true;
        for (int i = 0; i < array_len; ++i)
            if (flow_count[i]) {
                success = false;
                break;
            }
            
        return success;
    }
};

#endif