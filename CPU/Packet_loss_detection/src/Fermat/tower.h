#include "../Common/BOBHash32.h"
#include <string>
#include <iostream>
#include "../common_func.h"
using namespace std;
uint8_t mask1 = 255;
uint16_t mask2 = 65535;
class Counters
{
public:
    int mem; 
    uint8_t *counters;
    int width; 
    int counter_w;
    Counters(int WIDTH, int COUNTER_W)
    {
        mem = WIDTH * COUNTER_W;
        width = WIDTH;
        counter_w = COUNTER_W;
        counters = new uint8_t[mem];
        memset(counters, 0, mem);
    }
    ~Counters()
    {
        delete[] counters;
    }
    uint32_t index(int idx)
    {
        if (counter_w == 1)
            return (uint32_t)counters[idx];
        else if (counter_w == 2)
            return (uint32_t) * (((uint16_t *)counters) + idx);
        return 0;
    }
    void increment(int idx)
    {
        if (counter_w == 1)
            counters[idx] += 1;
        else if (counter_w == 2)
            *(((uint16_t *)counters) + idx) += 1;
    }
};
enum Type
{
    CM,
    CU,
    half_CU
};
class TowerSketch
{
public:
    Counters *line;
    BOBHash32 *hash;
    Type type;
    int idx[2];
    //mem_per_line
    int mem;
    uint32_t threshold;
    TowerSketch(int w_d, Type _type = CM, uint32_t T = ELE_THRESHOLD, int _init = INIT)
    {
        mem = w_d * 4;
        threshold = T;
        line = new Counters[2]{{mem, 1}, {mem / 2, 2}};
        type = _type;
        hash = new BOBHash32[2];
        hash[0].initialize(_init);
        hash[1].initialize(_init + 1);
    }
    ~TowerSketch()
    {
        delete[] line;
    }
    void clear()
    {
        delete[] line;
    }
    bool insert(const char *key, int f = 1)
    {
        if (type == CM)
            return insertCM(key, f);
        else if (type == CU)
            return insertCU(key, f);
        else
            return inserthalf_CU(key, f);
    }
    bool insertCM(const char *key, int f = 1)
    {
        bool flag = false;
        idx[0] = hash[0].run(key, 4) % line[0].width;
        idx[1] = hash[1].run(key, 4) % line[1].width;
        uint32_t val0 = line[0].index(idx[0]);
        uint32_t val1 = line[1].index(idx[1]);
        if (val0 < threshold){
            //cout<<val0<<endl;
            flag = true;
        }
        if (val1 < threshold){      
            flag = true;
            //cout<<val1<<endl;
        }
        if (val0 != mask1)
            line[0].increment(idx[0]);
        if (val1 != mask2)
            line[1].increment(idx[1]);
        return flag;
    }
    bool insertCU(const char *key, int f = 1)
    {
        bool flag = false;
        idx[0] = hash[0].run(key, 4) % line[0].width;
        idx[1] = hash[1].run(key, 4) % line[1].width;
        uint32_t val0 = line[0].index(idx[0]);
        uint32_t val1 = line[1].index(idx[1]);
        if (val0 < threshold)
            flag = true;
        if (val0 == mask1)
            val0 = UINT32_MAX;
        if (val1 < threshold)
        {
            flag = true;
        }
        if (val0 < val1)
        {
            line[0].increment(idx[0]);
        }
        else if (val1 < val0)
        {
            if (val1 < threshold)
                line[1].increment(idx[1]);
        }
        else
        {
            line[0].increment(idx[0]);
            line[1].increment(idx[1]);
        }

        return flag;
    }
    bool inserthalf_CU(const char *key, int f = 1)
    {
        bool flag = false;
        idx[0] = hash[0].run(key, 4) % line[0].width;
        idx[1] = hash[1].run(key, 4) % line[1].width;
        uint32_t val0 = line[0].index(idx[0]);
        uint32_t val1 = line[1].index(idx[1]);
        if (val0 != mask1)
        {
            flag = true;
            line[0].increment(idx[0]);
        }
        else
            val0 = UINT32_MAX;
        if (val1 <= val0)
        {
            if (val1 < threshold)
                line[1].increment(idx[1]);
        }
        if (val1 < threshold)
            flag = true;
        return flag;
    }
    int query(const char *key)
    {
        idx[0] = hash[0].run(key, 4) % line[0].width;
        idx[1] = hash[1].run(key, 4) % line[1].width;
        uint32_t val0 = line[0].index(idx[0]);
        uint32_t val1 = line[1].index(idx[1]);
        if (val0 == mask1)
            val0 = 1 << 30;
        int ret = min(val0, val1);
        return ret;
    }
    uint32_t query8bit(const char *key)
    {
        idx[0] = hash[0].run(key, 4) % line[0].width;
        return line[0].index(idx[0]);
    }
    int get_cardinality()
    {
        int empty = 0;
        for (int i = 0; i < line[1].width; i++)
        {
            if (!line[1].index(i))
            {
                empty++;
            }
        }
        return (int)((double)line[1].width * log((double)line[1].width / (double)empty));
    }
    void get_entropy(int & tot, double & entr){
        int mice_dist[256] = {0};
        for(int i = 0; i < line[0].width ; i++){
            mice_dist[line[0].index(i)]++;
        }
        for(int i= 1;i<256;i++){
            tot += mice_dist[i] * i;
            entr += mice_dist[i] * i * log2(i);
        }
    }
};
