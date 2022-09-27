#ifndef _FERMAT_8_H_
#define _FERMAT_8_H_

#include <iostream>
#include <cstdint>
#include <unordered_map>
#include <queue>
#include <cstring>
#include "util/BOBHash32.h"
#include "util/mod.h"
#include "util/prime.h"
#include "murmur3.h"

using namespace std;

#define DEBUG_F 0

// fingprint no used

// use a 16-bit prime, so 2 * a mod PRIME will not overflow
static const uint64_t PRIME_ID_8BIT = MAXPRIME[48];
static uint64_t tot_cnt_tot = 0;

uint64_t checkTable8Bit(uint64_t pos) {
    return powMod64(pos, PRIME_ID_8BIT - 2, PRIME_ID_8BIT);
}


class Fermat_8bit {  
    // arrays
    int array_num;
    int entry_num;
    uint64_t **id;
    uint32_t **fingerprint;
    uint32_t **counter;
    // hash
    uint32_t *d;
    // int d1;
    // int d2;
    // int d;
    uint32_t *hash;
    uint32_t hash_fp;
    // BOBHash32 *hash_4_div;

    uint32_t *table;

    bool use_fing;
    unordered_map<uint32_t, int> out;
    int pure_cnt;
    int tot_cnt;
    uint64_t PRIME_FING_8BIT = MAXPRIME[8];

public:


    void create_array() {
        pure_cnt = 0;
        tot_cnt = 0;
        // id
        id = new uint64_t*[array_num];
        for (int i = 0; i < array_num; ++i) {
            id[i] = new uint64_t[entry_num * d[i]];
            memset(id[i], 0, entry_num * d[i] * sizeof(uint64_t));
        }
        
        // counter
        counter = new uint32_t*[array_num];
        for (int i = 0; i < array_num; ++i) {
            counter[i] = new uint32_t[entry_num * d[i]];
            memset(counter[i], 0, entry_num * d[i] * sizeof(uint32_t));
        }
    }

    void clear_array() {
        for (int i = 0; i < array_num; ++i)
            delete [] id[i];
        delete [] id;
        
        for (int i = 0; i < array_num; ++i)
            delete [] counter[i];
        delete [] counter;
    }

    // Fermat_8bit(int a, int e, bool _fing, uint32_t _init) : array_num(a), entry_num(e), use_fing(_fing), fingerprint(nullptr), hash_fp(0) {
    //     d = new uint32_t[array_num];
    //     create_array();
    //     // hash
    //     if (use_fing)
    //         hash_fp = _init;
    //     // hash_4_div = new BOBHash32(_init+2);
    //     hash = new uint32_t[array_num];
    //     for (int i = 0; i < array_num; ++i) {
    //         hash[i] = _init + (10 * i) + 1;
    //         d[i] = 1;
    //     }

    // }

    Fermat_8bit(int _memory, int a, uint32_t _init, int fing) : array_num(a), fingerprint(nullptr), hash_fp(0) {
        entry_num = _memory / (array_num * 16);
        // cout << PRIME_ID_8BIT << endl;
        // cout << entry_num << endl;
        d = new uint32_t[array_num];
        hash = new uint32_t[array_num];
        for (int i = 0; i < array_num; ++i) {
            hash[i] = _init + (10 * i) + 1;
            d[i] = 1;
        }   
        // cout << "construct fermat with " << entry_num << " entry" << endl;
        create_array();
        
        hash_fp = _init;
        use_fing = true;
        PRIME_FING_8BIT = (1 << fing);
    }

    ~Fermat_8bit() {
        clear_array();
        // clear_look_up_table();
        // delete hash_4_div;
        delete [] hash;
    }

    void Insert(uint32_t flow_id, uint32_t cnt) {
        // uint32_t div = hash_4_div->run((char*)&flow_id, sizeof(uint32_t));
        // div %= (d + d1 + d2 + 3);
        // cout << "ins" << endl;
        // cout << PRIME_ID << endl;
        // cout << PRIME_FING << endl;
        // cout << "flow_id " << flow_id << endl;
        if (use_fing) {
            uint64_t fing = MurmurHash3_x86_32((void*)&flow_id, sizeof(uint32_t), hash_fp) % PRIME_FING_8BIT;
            uint64_t newflow_id = (fing << 32) + flow_id;
            
            for (int i = 0; i < array_num; ++i) {
                uint32_t pos = MurmurHash3_x86_32((void*)&flow_id, sizeof(uint32_t), hash[i]) % (entry_num * d[i]);
                // cout << pos << endl;
                // if(id[i][pos] == 0) {
                //     cnt = 1;
                // }
                id[i][pos] = (mulMod64(newflow_id, cnt, PRIME_ID_8BIT) + (uint64_t)id[i][pos]) % PRIME_ID_8BIT;
                // cout << "before mulMod32" << endl;
                // fingerprint[i][pos] = ((uint64_t)fingerprint[i][pos] + mulMod32(fing, cnt, PRIME_FING)) % PRIME_FING;
                counter[i][pos] += cnt;
            }
        } else {
            for (int i = 0; i < array_num; ++i) {
                // cout << "i " << i << endl;
                uint32_t pos = MurmurHash3_x86_32((void*)&flow_id, sizeof(uint32_t), hash[i]) % (entry_num * d[i]);
                id[i][pos] = (mulMod64(flow_id, cnt, PRIME_ID) + (uint64_t)id[i][pos]) % PRIME_ID;
                counter[i][pos] += cnt;
            } 
            // cout << "done" << endl;
        }
    }
    
    void Insert_one(uint32_t flow_id) {
        if (use_fing) {
            uint32_t fing = MurmurHash3_x86_32((void*)&flow_id, sizeof(uint32_t), hash_fp) % PRIME_FING_8BIT;
            uint64_t newflow_id = (fing << 32) + flow_id;
            for (int i = 0; i < array_num; ++i) {
                uint32_t pos = MurmurHash3_x86_32((void*)&newflow_id, sizeof(uint64_t), hash[i]) % (entry_num * d[i]);
                id[i][pos] = (newflow_id + (uint64_t)id[i][pos]) % PRIME_ID_8BIT;
                counter[i][pos]++;
            }
        } else {
            for (int i = 0; i < array_num; ++i) {
                uint32_t pos = MurmurHash3_x86_32((void*)&flow_id, sizeof(uint32_t), hash[i]) % (entry_num * d[i]);
                id[i][pos] = ((uint64_t)id[i][pos] + (uint64_t)(flow_id % PRIME_ID)) % PRIME_ID;
                counter[i][pos]++;
            }
        }
    }

    void Delete_in_one_bucket(int row, int col, int pure_row, int pure_col) {
        // delete (flow_id, fing, cnt) in bucket (row, col)
        id[row][col] = ((uint64_t)PRIME_ID_8BIT + (uint64_t)id[row][col] - (uint64_t)id[pure_row][pure_col]) % PRIME_ID_8BIT;
        counter[row][col] -= counter[pure_row][pure_col];
    }

    bool verify(int row, int col, uint32_t &flow_id, uint32_t &fing) {
        int num;
        int col_len;
        num = counter[row][col];
        col_len = entry_num * d[row];
        uint64_t newflow_id;
        if(num & 0x80000000) {
            uint64_t temp = checkTable8Bit(~num + 1);
            newflow_id = mulMod64(PRIME_ID_8BIT - id[row][col], temp, PRIME_ID_8BIT);
        } else {
            uint64_t temp = checkTable8Bit(num);
            newflow_id = mulMod64(id[row][col], temp, PRIME_ID_8BIT);
        }
        uint32_t fing1 = newflow_id >> 32;
        flow_id = newflow_id & 0xffffffff;
        uint32_t finge = MurmurHash3_x86_32((void*)&flow_id, sizeof(uint32_t), hash_fp) % PRIME_FING_8BIT;
        // cout << "vn" << newflow_id << endl;
        // cout << "vf" << flow_id << endl;
        // flow_id = (id[row][col] * table[counter[row][col] % PRIME_ID]) % PRIME_ID;
        if (!(MurmurHash3_x86_32((void*)&flow_id, sizeof(uint32_t), hash[row]) % col_len == col)){
            // ++pure_cnt;
            return false;
        }
        if(finge != fing1){
            tot_cnt++;
            return false;
        }
        if(out.count(flow_id) != 0) {
            out[flow_id] += 1;
        } else {
            out[flow_id] = 1;
        }
        if(out[flow_id] > 1000)
            return false;
        // ++pure_cnt;
        return true;
    }

    void display() {
        cout << " --- display --- " << endl;
        for (int i = 0; i < array_num; ++i) {
            for (int j = 0; j < entry_num; ++j) {
                if (counter[i][j]) {
                    cout << i << "," << j << ":" << counter[i][j] << endl;
                }
            }
        }
    }

    bool Decode(unordered_map<uint32_t, int> &result) {
        queue<int> *candidate = new queue<int> [array_num];
        uint32_t flow_id = 0;
        uint32_t fing = 0;

        // first round
        for (int i = 0; i < array_num; ++i)
            for (int j = 0; j < entry_num * d[i]; ++j) {
                if (counter[i][j] == 0) {
                    continue;
                }
                else if (verify(i, j, flow_id, fing)) { 
                    // find pure bucket
                    if(result.count(flow_id) != 0) {
                        result[flow_id] += counter[i][j];
                    } else {
                        result[flow_id] = counter[i][j];
                    }
                    // delete flow from other rows
                    for (int t = 0; t < array_num; ++t) {
                        if (t == i) continue;
                        // uint64_t fing = MurmurHash3_x86_32((void*)&flow_id, sizeof(uint32_t), hash_fp) % PRIME_FING_8BIT;
                        // uint64_t newflow_id = (fing << 32) + flow_id;
                        uint32_t pos = MurmurHash3_x86_32((void*)&flow_id, sizeof(uint32_t), hash[t]) % (entry_num * d[t]);
                        // uint32_t pos = MurmurHash3_x86_32((void*)&newflow_id, sizeof(uint64_t), hash[t]) % (entry_num * d[t]);
                        Delete_in_one_bucket(t, pos, i, j);
                        candidate[t].push(pos);
                    }
                    Delete_in_one_bucket(i, j, i, j);
                }
            }

        bool pause;
        int acc = 0;
        while (true) {
            acc++;
            pause = true;
            for (int i = 0; i < array_num; ++i) {
                if (!candidate[i].empty()) pause = false;
                while (!candidate[i].empty()) {
                    int check = candidate[i].front();
                    candidate[i].pop();
                    // cout << i << " " << check << endl;
                    if (counter[i][check] == 0) {
                        continue;
                    }
                    else if (verify(i, check, flow_id, fing)) { 
                        // find pure bucket
                        if(result.count(flow_id) != 0) {
                            result[flow_id] += counter[i][check];
                        } else {
                            result[flow_id] = counter[i][check];
                        }
                        // delete flow from other rows
                        for (int t = 0; t < array_num; ++t) {
                            if (t == i) continue;
                            // uint64_t fing = MurmurHash3_x86_32((void*)&flow_id, sizeof(uint32_t), hash_fp) % PRIME_FING_8BIT;
                            // uint64_t newflow_id = (fing << 32) + flow_id;
                            uint32_t pos = MurmurHash3_x86_32((void*)&flow_id, sizeof(uint32_t), hash[t]) % (entry_num * d[t]);
                            // uint32_t pos = MurmurHash3_x86_32((void*)&newflow_id, sizeof(uint64_t), hash[t]) % (entry_num * d[t]);
                            Delete_in_one_bucket(t, pos, i, check);
                            candidate[t].push(pos);
                        }
                        Delete_in_one_bucket(i, check, i, check);
                    }
                }
            }
            if (pause)
                break;
            if(acc > 10000)
                break;
        }

        delete [] candidate;
        
        bool flag = true;
        for (int i = 0; i < array_num; ++i)
            for (int j = 0; j < entry_num * d[i]; ++j)
                if (counter[i][j] != 0) {
                    // cout << "undecoded  i " << i << " j " << j << endl;
                    // cout << counter[i][j] << endl;
                    flag = false;      
                }        
        for (auto p : result) {
            if(p.second == 0) {
                result.erase(p.first);
            }
        }
        // cout << "pure_cnt " << pure_cnt << endl;
        if(flag && tot_cnt != 0) {
            tot_cnt_tot++;
        }
        // if(flag == false) {
        //     display();
        // }
        return flag;
    }
};

#endif