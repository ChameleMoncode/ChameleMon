#include "generate_flows.h"
// #include "fermat.h"
#include "../Lossradar/lossradar.h"
#include "../Flowradar/flowradar.h"
#include <chrono>
#include <fstream>
#include <iostream>

using namespace std;

#define Max_threshold 500000

class TestFullFlow {
    int memory_size;
    // Fermat fermat_rehash, fermat_fing;
    LossRadar lossradar;
    FlowRadar flowradar1, flowradar2;
    uint32_t all[Max_threshold];
    unordered_map<uint32_t, int> realAns;

public:
    TestFullFlow(int _mem, int seed) : memory_size(_mem), 
        lossradar(3, _mem, seed + 40), flowradar1(_mem, 3, seed + 60), flowradar2(_mem, 3, seed + 60) {}

    void insert_flow(const CDF_flows &data) {
        for (auto p : data.flow_set) {
            for (uint16_t i = 0; i < p.second; ++i)
                flowradar1.Insert(p.first);
            if (data.dropped_set.count(p.first)) {
                // fermat_fing.Insert(p.first, data.dropped_set.at(p.first));
                // fermat_rehash.Insert(p.first, data.dropped_set.at(p.first));
                lossradar.Insert_range_data(p.first, data.dropped_set.at(p.first));
                for (uint16_t i = data.dropped_set.at(p.first); i < p.second; ++i)
                    flowradar2.Insert(p.first);
            }
            else {
                for (uint16_t i = 0; i < p.second; ++i)
                    flowradar2.Insert(p.first);
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

    bool decode(const CDF_flows &data, ofstream &output) {
        bool flag = true;
        int flow_sum = data.dropped_set.size();
        // cout << "--- mem: " << memory_size << " --- " << endl;
        output << memory_size << "\t";

        int decoded_flow_cnt = 0;
        unordered_map<uint32_t, int> res, res2;
        // auto start1 = chrono::system_clock::now();
        // fermat_fing.Decode(res);
        // auto end1 = chrono::system_clock::now();
        // auto time_decode1 = end1 - start1;
        // // cout << "fing dec"<< endl;
        // for (auto &p : data.dropped_set) {
        //     if (res.count(p.first) != 0 && res[p.first] == p.second)
        //         ++decoded_flow_cnt;
        //     // else {
        //     //     cout << p.first << " " << p.second << endl;
        //     // }
        // }
        // output << (double)decoded_flow_cnt / flow_sum << "\t";
        // if(decoded_flow_cnt != flow_sum) 
        //     flag = false;

        // res.clear();
        // decoded_flow_cnt = 0;
        // start1 = chrono::system_clock::now();
        // fermat_rehash.Decode(res);
        // end1 = chrono::system_clock::now();
        // // cout << "rehash dec"<< endl;
        // auto time_decode2 = end1 - start1;

        // for (auto &p : data.dropped_set) {
        //     if (res.count(p.first) != 0 && res[p.first] == p.second)
        //         ++decoded_flow_cnt;
        //     // else 
        //     //     cout << p.first << " " << p.second << " " << data.dropped_set.at(p.first) << endl;
        // }
        // output << (double)decoded_flow_cnt / flow_sum << "\t";
        // if(decoded_flow_cnt != flow_sum) 
        //     flag = false;

        // res.clear();
        // decoded_flow_cnt = 0;
        // auto start1 = chrono::system_clock::now();
        // lossradar.Decode(res);
        // auto end1 = chrono::system_clock::now();
        // // cout << "loss dec"<< endl;
        // auto time_decode3 = end1 - start1;

        // for (auto &p : data.dropped_set) {
        //     if (res.count(p.first) != 0 && res[p.first] == p.second)
        //         ++decoded_flow_cnt;
        // }
        // output << (double)decoded_flow_cnt / flow_sum << "\t";
        // if(decoded_flow_cnt != flow_sum) 
        //     flag = false;

        res.clear();
        decoded_flow_cnt = 0;

        auto start1 = chrono::system_clock::now();
        flowradar1.SingleDecode(res);
        flowradar2.SingleDecode(res2);
        auto end1 = chrono::system_clock::now();
        // cout << "flow dec" << endl;
        auto time_decode4 = end1 - start1;

        for (auto &p : data.dropped_set) {
            if (res2.count(p.first) == 0 && res[p.first] == p.second)
                ++decoded_flow_cnt;
            else if (res[p.first] - res2[p.first] == p.second)
                ++decoded_flow_cnt;
        }

        output << (double)decoded_flow_cnt / flow_sum << "\t";
        if(decoded_flow_cnt != flow_sum) 
            flag = false;

        // output << time_decode1.count() << "\t";
        // output << time_decode2.count() << "\t";
        // output << time_decode3.count() << "\t";
        output << time_decode4.count() << endl;
        // output << flow_sum << endl;
        return flag;
    }
};

