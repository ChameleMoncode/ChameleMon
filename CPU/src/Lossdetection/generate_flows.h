#ifndef _GENERATE_FLOWS_H_
#define _GENERATE_FLOWS_H_

#include <map>
#include <random>
#include <unordered_map>
#include <iostream>
#include <vector>
#include <algorithm>

using namespace std;

// DCTCP_CDF
static const map<double, int> mp = {
{0, 0}, 
{0.15, 10}, 
{0.2, 20}, 
{0.3, 30},
{0.4, 50},
{0.53, 80},
{0.6, 200},
{0.7, 1e+03},
{0.8, 2e+03},
{0.9, 5e+03},
{0.97, 1e+04},
{1, 3e+04}
};

int get_stream_size(double d) {
    auto it = mp.lower_bound(d);
    auto next = it;
    if(it != mp.begin()) it --;
    double k = (next->second - it->second) / (next->first - it->first);
    return it->second + ((d - it->first) * k);
}

double _rand_double() {
    static default_random_engine e;
    uniform_real_distribution<double> u(0.0, 1.0);
    return u(e);
}

void loadCAIDA18(const char *filename, unordered_map<uint64_t, uint16_t> &data) {
    printf("Open %s \n", filename);
    FILE* pf = fopen(filename, "rb");
    if(!pf){
        printf("%s not found!\n", filename);
        exit(-1);
    }
    char trace[30];
    uint64_t flow_id; 
    while(fread(trace, 1, 21, pf)) {
        flow_id = *(uint64_t*) (trace); 
        flow_id &= (0x3fffffffffffffff);

        if (data.count(flow_id) == 0) 
            data[flow_id] = 1;
        else 
            ++data[flow_id];
    }
    fclose(pf);
}

void loadCAIDA18_32_old(const char *filename, unordered_map<uint32_t, uint16_t> &data) {
    printf("Open %s \n", filename);
    FILE* pf = fopen(filename, "rb");
    if(!pf){
        printf("%s not found!\n", filename);
        exit(-1);
    }
    char trace[30];
    uint32_t flow_id; 
    while(fread(trace, 1, 21, pf)) {
        flow_id = *(uint32_t*) (trace); 
        // flow_id &= (0x3fffffffffffffff);

        if (data.count(flow_id) == 0) 
            data[flow_id] = 1;
        else 
            ++data[flow_id];
    }
    fclose(pf);
}

void loadCAIDA18_32(const char *filename, unordered_map<uint32_t, uint16_t> &data) {
    printf("Open %s \n", filename);
    FILE* pf = fopen(filename, "rb");
    if(!pf){
        printf("%s not found!\n", filename);
        exit(-1);
    }
    char trace[30];
    uint32_t flow_id; 
    uint32_t cnt = 0;
    vector<int> v;
    while(fread(trace, 1, 21, pf)) {
        flow_id = *(uint32_t*) (trace); 
        // flow_id &= (0x3fffffffffffffff);

        if (data.count(flow_id) == 0) {
            data[flow_id] = 1;
            cnt++;
        }
        else 
            ++data[flow_id];
        if(cnt >= 100000)
            break;
    }
    fclose(pf);
    for(auto &p : data) {
        int f = p.second;
        v.push_back(f);
    }
    sort(v.rbegin(), v.rend());
    int i = 0;
    for(auto &p : data) {
        p.second = v[i++];
    }
}

struct CDF_flows
{
    int setting;
    uint32_t dropped_num, packet_num;
    unordered_map<uint32_t, uint16_t> flow_set, dropped_set;

    CDF_flows(int _set) : setting(_set), dropped_num(0), packet_num(0) {}

    void load_data(const char* filename) {
        unordered_map<uint64_t, uint16_t> data;
        loadCAIDA18(filename, data);

        if (setting == 0) {
            int cnt = 0;
            for (auto &p : data) {
                uint16_t flow_size = get_stream_size(_rand_double());
                while (flow_size == 0) 
                    flow_size = get_stream_size(_rand_double());
                
                flow_set[p.first] = flow_size;
                packet_num += flow_size;
                if (cnt < 100) {
                    dropped_set[p.first] = ceil((double)flow_size * 0.001);
                    dropped_num += dropped_set[p.first];
                }  
                ++cnt;
                if (cnt >= 1000)
                    break;
            }
        }
        else if (setting == 1) {
            int cnt = 0;
            for (auto &p : data) {
                uint16_t flow_size = get_stream_size(_rand_double());
                while (flow_size == 0) 
                    flow_size = get_stream_size(_rand_double());

                flow_set[p.first] = flow_size;
                packet_num += flow_size;
                if (cnt < 100) {
                    dropped_set[p.first] = flow_size;
                    dropped_num += flow_size;
                }
                ++cnt;
                if (cnt >= 10000)
                    break;
            }
        }
    }

    void load_data_32(const char* filename) {
        unordered_map<uint32_t, uint16_t> data;
        loadCAIDA18_32(filename, data);

        if (setting == 0) { // all flow loss 1%
            int cnt = 0;
            double percent = 0.01;
            int total = 10000;
            for (auto &p : data) {
                uint16_t flow_size = p.second;
                // while (flow_size == 0) 
                //     flow_size = get_stream_size(_rand_double());
                
                flow_set[p.first] = flow_size;
                packet_num += flow_size;
                dropped_set[p.first] = ceil((double)flow_size * percent);
                dropped_num += dropped_set[p.first];
                ++cnt;
                if (cnt >= total)
                    break;
            }
        }
        else if (setting == 1) { // 1% flow loss all
            int cnt = 0;
            int sam_cnt = 0;
            double percent = 1;
            int total = 10000;
            int total_sam = 100;
            for (auto &p : data) {
                uint16_t flow_size = p.second;
                // while (flow_size == 0) 
                //     flow_size = get_stream_size(_rand_double());

                flow_set[p.first] = flow_size;
                packet_num += flow_size;
                if (sam_cnt < total_sam && flow_size >= 100) {
                    dropped_set[p.first] = flow_size;
                    dropped_num += flow_size;
                    sam_cnt++;
                }
                ++cnt;
                if (cnt >= total)
                    break;
            }
            cnt = 0;
            for (auto &p : data) {
                if(sam_cnt >= total_sam)
                    break;
                uint16_t flow_size = p.second;

                if (dropped_set.count(p.first) == 0) {
                    dropped_set[p.first] = flow_size;
                    dropped_num += dropped_set[p.first];
                    sam_cnt++;
                }  
                ++cnt;
                if (cnt >= total)
                    break;
            }
        }
    }

    void generate_sim_data() {
        if (setting == 0) {
            for (uint32_t i = 0; i < 1000; ++i) {
                uint16_t flow_size = get_stream_size(_rand_double());
                while (flow_size == 0) 
                    flow_size = get_stream_size(_rand_double());

                flow_set[i] = flow_size;
                packet_num += flow_size;
                if (i < 100) {
                    dropped_set[i] = ceil(flow_size * 0.05);
                    dropped_num += dropped_set[i];
                }
            }
        }
        else if (setting == 1) {
            for (uint32_t i = 0; i < 10000; ++i) {
                uint16_t flow_size = get_stream_size(_rand_double());
                while (flow_size == 0) 
                    flow_size = get_stream_size(_rand_double());
                
                flow_set[i] = flow_size;
                packet_num += flow_size;
                if (i < 100) {
                    dropped_set[i] = flow_size;
                    dropped_num += flow_size;
                }
            }
        }
    }

    void load_data_flow(const char* filename, int flow_cnt) {
        unordered_map<uint32_t, uint16_t> data;
        loadCAIDA18_32(filename, data);

        int sam_cnt = 0;
        int cnt = 0;
        double percent = 0.01;
        int total_sam = 100;    
        for (auto &p : data) {
            uint16_t flow_size = p.second;
            // while (flow_size == 0) 
            //     flow_size = get_stream_size(_rand_double());
            
            flow_set[p.first] = flow_size;
            packet_num += flow_size;
            if (sam_cnt < total_sam && flow_size >= 10 && cnt < 1000) {
                dropped_set[p.first] = ceil((double)flow_size * percent);
                dropped_num += dropped_set[p.first];
                sam_cnt++;
            }  
            ++cnt;
            if (cnt >= flow_cnt)
                break;
        }
        cnt = 0;
        for (auto &p : data) {
            if(sam_cnt >= 100)
                break;
            uint16_t flow_size = p.second;

            if (dropped_set.count(p.first) == 0) {
                dropped_set[p.first] = ceil((double)flow_size * percent);
                dropped_num += dropped_set[p.first];
                sam_cnt++;
            }  
            ++cnt;
            if (cnt >= 1000)
                break;
        }
        cout << "sam_cnt:" << sam_cnt << endl;
    }

    void load_data_perc(const char* filename, int perc) {
        unordered_map<uint32_t, uint16_t> data;
        loadCAIDA18_32(filename, data);
        double percent = (double) perc / 100.0;

        int sam_cnt = 0;
        int cnt = 0;
        int total = 10000;
        int total_sam = 100;
        for (auto &p : data) {
            uint16_t flow_size = p.second;
            // while (flow_size == 0) 
            //     flow_size = get_stream_size(_rand_double());
            
            flow_set[p.first] = flow_size;
            packet_num += flow_size;
            if (sam_cnt < total_sam && flow_size >= 100) {
                if(ceil((double)flow_size * percent) != 0) {
                    dropped_set[p.first] = ceil((double)flow_size * percent);
                    dropped_num += dropped_set[p.first];
                }
                sam_cnt++;
            }  
            ++cnt;
            if (cnt >= total)
                break;
        }
        cnt = 0;
        for (auto &p : data) {
            if(sam_cnt >= 100)
                break;
            uint16_t flow_size = p.second;

            if (dropped_set.count(p.first) == 0) {
                if(ceil((double)flow_size * percent) != 0) {
                    dropped_set[p.first] = ceil((double)flow_size * percent);
                    dropped_num += dropped_set[p.first];
                }
                sam_cnt++;
            }  
            ++cnt;
            if (cnt >= total)
                break;
        }
    }

    void load_data_cnt(const char* filename, int flow_cnt) {
        unordered_map<uint32_t, uint16_t> data;
        loadCAIDA18_32(filename, data);

        int sam_cnt = 0;
        int cnt = 0;
        double percent = 0.1;   
        for (auto &p : data) {
            uint16_t flow_size = p.second;
            // while (flow_size == 0) 
            //     flow_size = get_stream_size(_rand_double());
            
            flow_set[p.first] = flow_size;
            packet_num += flow_size;
            if (sam_cnt < flow_cnt && flow_size >= 10 && cnt < flow_cnt) {
                dropped_set[p.first] = ceil((double)flow_size * percent);
                dropped_num += dropped_set[p.first];
                sam_cnt++;
            }  
            ++cnt;
            if (sam_cnt >= flow_cnt)
                break;
        }
        cnt = 0;
        for (auto &p : data) {
            if(sam_cnt >= flow_cnt)
                break;
            uint16_t flow_size = p.second;

            if (dropped_set.count(p.first) == 0) {
                dropped_set[p.first] = ceil((double)flow_size * percent);
                dropped_num += dropped_set[p.first];
                sam_cnt++;
            }  
            ++cnt;
            if (cnt >= flow_cnt)
                break;
        }
        cout << "sam_cnt:" << sam_cnt << endl;
    }

    void load_data_drop(const char* filename, int drop) {
        unordered_map<uint32_t, uint16_t> data;
        loadCAIDA18_32(filename, data);

        int sam_cnt = 0;
        int cnt = 0;
        int total_sam = drop;  
        int total = 10000; 
        if (setting == 0) {
            double percent = 0.01;
            for (auto &p : data) {
                uint16_t flow_size = p.second;
                // while (flow_size == 0) 
                //     flow_size = get_stream_size(_rand_double());
                
                flow_set[p.first] = flow_size;
                packet_num += flow_size;
                if (sam_cnt < total_sam && flow_size >= 100) {
                    dropped_set[p.first] = ceil((double)flow_size * percent);
                    dropped_num += dropped_set[p.first];
                    sam_cnt++;
                }  
                ++cnt;
                if (cnt >= total)
                    break;
            }
            cnt = 0;
            for (auto &p : data) {
                if(sam_cnt >= total_sam)
                    break;
                uint16_t flow_size = p.second;

                if (dropped_set.count(p.first) == 0) {
                    dropped_set[p.first] = ceil((double)flow_size * percent);
                    dropped_num += dropped_set[p.first];
                    sam_cnt++;
                }  
                ++cnt;
                if (cnt >= total)
                    break;
            }
        } else if (setting == 1){
            double percent = 0.1;
            for (auto &p : data) {
                uint16_t flow_size = p.second;
                // while (flow_size == 0) 
                //     flow_size = get_stream_size(_rand_double());
                
                flow_set[p.first] = flow_size;
                packet_num += flow_size;
                if (sam_cnt < total_sam && flow_size >= 100) {
                    dropped_set[p.first] = ceil((double)flow_size * percent);
                    dropped_num += dropped_set[p.first];
                    sam_cnt++;
                }  
                ++cnt;
                if (cnt >= total)
                    break;
            }
            cnt = 0;
            for (auto &p : data) {
                if(sam_cnt >= total_sam)
                    break;
                uint16_t flow_size = p.second;

                if (dropped_set.count(p.first) == 0) {
                    dropped_set[p.first] = ceil((double)flow_size * percent);
                    dropped_num += dropped_set[p.first];
                    sam_cnt++;
                }  
                ++cnt;
                if (cnt >= total)
                    break;
            }
        } else if (setting == 2){
            double percent = 0.5;
            for (auto &p : data) {
                uint16_t flow_size = p.second;
                // while (flow_size == 0) 
                //     flow_size = get_stream_size(_rand_double());
                
                flow_set[p.first] = flow_size;
                packet_num += flow_size;
                if (sam_cnt < total_sam && flow_size >= 100) {
                    // if(floor((double)flow_size * percent) != 0) {
                    dropped_set[p.first] = ceil((double)flow_size * percent);
                    dropped_num += dropped_set[p.first];
                    sam_cnt++;
                }  
                ++cnt;
                if (cnt >= total)
                    break;
            }
            cnt = 0;
            for (auto &p : data) {
                if(sam_cnt >= total_sam)
                    break;
                uint16_t flow_size = p.second;

                if (dropped_set.count(p.first) == 0) {
                    dropped_set[p.first] = ceil((double)flow_size * percent);
                    dropped_num += dropped_set[p.first];
                    sam_cnt++;
                }  
                ++cnt;
                if (cnt >= total)
                    break;
            }
        } else if (setting == 3){
            double percent = 0.75;
            for (auto &p : data) {
                uint16_t flow_size = p.second;
                // while (flow_size == 0) 
                //     flow_size = get_stream_size(_rand_double());
                
                flow_set[p.first] = flow_size;
                packet_num += flow_size;
                if (sam_cnt < total_sam && flow_size >= 100) {
                    if(floor((double)flow_size * percent) != 0) {
                        dropped_set[p.first] = floor((double)flow_size * percent);
                        dropped_num += dropped_set[p.first];
                    }
                    sam_cnt++;
                }  
                ++cnt;
                if (cnt >= total)
                    break;
            }
            cnt = 0;
            for (auto &p : data) {
                if(sam_cnt >= total_sam)
                    break;
                uint16_t flow_size = p.second;

                if (dropped_set.count(p.first) == 0) {
                    dropped_set[p.first] = ceil((double)flow_size * percent);
                    dropped_num += dropped_set[p.first];
                    sam_cnt++;
                }  
                ++cnt;
                if (cnt >= total)
                    break;
            }
        }
    }
};



#endif