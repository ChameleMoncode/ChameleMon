#include "generate_flows.h"
#include "../Fermat/fermat_level.h"
#include "../Fermat/fermat_8bit.h"
#include "../Lossradar/lossradar.h"
#include "../Flowradar/flowradar.h"
#include <chrono>
#include <fstream>
#include <iostream>

using namespace std;

#define Max_threshold 500000

struct retV
{
    bool flag1;
    bool flag2;
    bool flag3;
    bool flag4;
    bool flag5;
    double dcd_per1;
    double dcd_per2;
    double dcd_per3;
    double dcd_per4;
    double dcd_per5;
    long long time1;
    long long time2;
    long long time3;
    long long time4;
    long long time5;   
};

class TestFullFlow {
    int memory_size;
    Fermat fermat_rehash11;
    Fermat fermat_rehash12;
    Fermat fermat_rehash13;
    Fermat fermat_rehash14;
    Fermat fermat_rehash15;
    // Fermat_8bit fermat_fing;
    // Fermat_8bit fermat_8bit;
    // LossRadar lossradar;
    // FlowRadar flowradar1, flowradar2;
    uint32_t all[Max_threshold];
    unordered_map<uint32_t, int> realAns;

public:
    TestFullFlow(int _mem, int seed, int a) : memory_size(_mem), fermat_rehash11(_mem, a, true, seed, 11), fermat_rehash12(_mem, a, true, seed, 12), fermat_rehash13(_mem, a, true, seed, 13),fermat_rehash14(_mem, a, true, seed, 14),fermat_rehash15(_mem, a, true, seed, 15) {}

    void insert_flow(const CDF_flows &data) {
        for (auto p : data.flow_set) {
            if (data.dropped_set.count(p.first)) {
                fermat_rehash11.Insert(p.first, data.dropped_set.at(p.first));
                fermat_rehash12.Insert(p.first, data.dropped_set.at(p.first));
                fermat_rehash13.Insert(p.first, data.dropped_set.at(p.first));
                fermat_rehash14.Insert(p.first, data.dropped_set.at(p.first));
                fermat_rehash15.Insert(p.first, data.dropped_set.at(p.first));
            }
        }
    }

    // void insert_flow() {
    //     ifstream ifs;
    //     ifs.open("130000.dat", ios::in);
    //     if(!ifs.is_open()) {
    //         cout << "read fail" << endl;
    //         return;
    //     }
    //     cout << "i'm here" << endl;
    //     char src_id[4] = {0};
    //     char else_id[9] = {0};
    //     char timestamp[8] = {0};
    //     int j = 0;
    //     int cnt = 0;
    //     while (true) {
    //         ifs.read(src_id, 4);
    //         ifs.read(else_id, 9);
    //         ifs.read(timestamp, 8);
            
    //         all[j] = id;
    //         j++;
    //         if(realAns.find(id) != realAns.end()) {
    //             realAns[id] += 1;
    //         } else {
    //             realAns[id] = 1;
    //             cnt++;
    //             if(cnt == threshold)
    //                 break;
    //         }
    //     }
    //     bool flag;
    //     for (int i = 0; i < j; ++i) {
    //         // flag = tower.insert(all[i]);
    //         flowradar1.Insert(all[i]);
    //         if(!flag) {
    //             fermat_fing.Insert_one(all[i]);
    //             fermat_rehash.Insert_one(all[i]);
    //             lossradar.Insert_range_data(all[i], 1);
    //             flowradar2.Insert(all[i]);
    //         }
    //     }
    // }

    retV decode(const CDF_flows &data, ofstream &output) {
        retV r;
        bool flag = true;
        bool outflag = false;
        int flow_sum = data.dropped_set.size();
        // cout << "--- mem: " << memory_size << " --- " << endl;
        

        int decoded_flow_cnt1 = 0;
        int decoded_flow_cnt2 = 0;
        int decoded_flow_cnt3 = 0;
        int decoded_flow_cnt4 = 0;
        int decoded_flow_cnt5 = 0;
        unordered_map<uint32_t, int> res, res2;
    
        // decoded_flow_cnt = 0;
        // bool f;
        auto start1 = chrono::system_clock::now();
        fermat_rehash11.Decode(res);
        auto end1 = chrono::system_clock::now();
        // cout << "rehash dec"<< endl;
        auto time_decode1 = end1 - start1;

        for (auto &p : data.dropped_set) {
            if (res.count(p.first) != 0 && res[p.first] == p.second)
                ++decoded_flow_cnt1;
            // else 
            // cout << p.first << " " << p.second << " " << res[p.first] << endl;
        }
        // int err_cnt = 0;
        // for (auto &p : res) {
        //     if (p.second != 0 && data.dropped_set.count(p.first) == 0) {
        //         // cout << p.first << " " << p.second << " error decode" << endl;
        //         err_cnt++;
        //     }
        // }
        
        if(decoded_flow_cnt1 != flow_sum){ 
            flag = false;
            r.flag1 = false;
            // cout << "fermat rehash decode_cnt:" << decoded_flow_cnt << endl;
        } else {
            r.flag1 = true;
        }
        r.dcd_per1 = (double)decoded_flow_cnt1/(double)flow_sum;
        r.time1 = time_decode1.count();

        res.clear();
        start1 = chrono::system_clock::now();
        fermat_rehash12.Decode(res);
        end1 = chrono::system_clock::now();
        // cout << "rehash dec"<< endl;
        auto time_decode2 = end1 - start1;

        for (auto &p : data.dropped_set) {
            if (res.count(p.first) != 0 && res[p.first] == p.second)
                ++decoded_flow_cnt2;
            // else 
            //     cout << p.first << " " << p.second << " " << data.dropped_set.at(p.first) << endl;
        }
        
        if(decoded_flow_cnt2 != flow_sum){ 
            r.flag2 = false;
            flag = false;
            // cout << "fermat fing decode_cnt:" << decoded_flow_cnt << endl;
        } else {
            r.flag2 = true;
        }
        r.dcd_per2 = (double)decoded_flow_cnt2/(double)flow_sum;
        r.time2 = time_decode2.count();

        res.clear();
        start1 = chrono::system_clock::now();
        fermat_rehash13.Decode(res);
        end1 = chrono::system_clock::now();
        // cout << "rehash dec"<< endl;
        auto time_decode3 = end1 - start1;

        for (auto &p : data.dropped_set) {
            if (res.count(p.first) != 0 && res[p.first] == p.second)
                ++decoded_flow_cnt3;
            // else 
            //     cout << p.first << " " << p.second << " " << data.dropped_set.at(p.first) << endl;
        }
        
        if(decoded_flow_cnt3 != flow_sum){ 
            flag = false;
            r.flag3 = false;
            // cout << "fermat 8bit decode_cnt:" << decoded_flow_cnt << endl;
        } else {
            r.flag3 = true;
        }
        r.dcd_per3 = (double)decoded_flow_cnt3/(double)flow_sum;
        r.time3 = time_decode3.count();

        res.clear();
        start1 = chrono::system_clock::now();
        fermat_rehash14.Decode(res);
        end1 = chrono::system_clock::now();
        // cout << "rehash dec"<< endl;
        auto time_decode4 = end1 - start1;

        for (auto &p : data.dropped_set) {
            if (res.count(p.first) != 0 && res[p.first] == p.second)
                ++decoded_flow_cnt4;
            // else 
            //     cout << p.first << " " << p.second << " " << data.dropped_set.at(p.first) << endl;
        }
        
        if(decoded_flow_cnt4 != flow_sum){ 
            flag = false;
            r.flag4 = false;
            // cout << "fermat 8bit decode_cnt:" << decoded_flow_cnt << endl;
        } else {
            r.flag4 = true;
        }
        r.dcd_per4 = (double)decoded_flow_cnt4/(double)flow_sum;
        r.time4 = time_decode4.count();

        res.clear();
        start1 = chrono::system_clock::now();
        fermat_rehash15.Decode(res);
        end1 = chrono::system_clock::now();
        // cout << "rehash dec"<< endl;
        auto time_decode5 = end1 - start1;

        for (auto &p : data.dropped_set) {
            if (res.count(p.first) != 0 && res[p.first] == p.second)
                ++decoded_flow_cnt5;
            // else 
            //     cout << p.first << " " << p.second << " " << data.dropped_set.at(p.first) << endl;
        }
        
        if(decoded_flow_cnt5 != flow_sum){ 
            flag = false;
            r.flag5 = false;
            // cout << "fermat 8bit decode_cnt:" << decoded_flow_cnt << endl;
        } else {
            r.flag5 = true;
        }
        r.dcd_per5 = (double)decoded_flow_cnt5/(double)flow_sum;
        r.time5 = time_decode5.count();

        // if(outflag) {
        //     output << memory_size / 8 << "\t";
        //     output << (double)decoded_flow_cnt1 / flow_sum << "\t";
        //     output << (double)decoded_flow_cnt2 / flow_sum << "\t";
        //     output << (double)decoded_flow_cnt3 / flow_sum << "\t";
        //     output << time_decode1.count() << "\t";
        //     output << time_decode2.count() << "\t";
        //     output << time_decode3.count() << endl;
        // }
        
        return r;
    }
};

