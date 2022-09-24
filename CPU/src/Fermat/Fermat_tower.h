#pragma once
#include "fermat.h"
#include "tower.h"
#include <map>
#include "../Common/EMFSD1.h"
#include "../common_func.h"
using namespace std;
class Fermat_tower
{
public:
    int tot_memory;
    int tot_packets;
    int fermatEleMem;
    int towerfilterMem;
    Fermat *fermatEle;
    TowerSketch *towerfilter;
    unordered_map<uint32_t, int> Eleresult;
    EMFSD1 *em = NULL;

public:
    Fermat_tower(int _tot_memory = TOT_MEMORY, int _fermatEleMem = ELE_BUCKET * (8 + 4 * USE_FING),
                 int _towerfilterMem = TOT_MEMORY - (ELE_BUCKET) * (8 + 4 * USE_FING),
                 bool usefing = USE_FING, uint32_t _init = INIT) : tot_memory(_tot_memory), fermatEleMem(_fermatEleMem), towerfilterMem(_towerfilterMem)
    {
        tot_packets = 0;
        fermatEle = new Fermat(fermatEleMem, usefing, _init);
        int w_d = towerfilterMem / 8;
        towerfilter = new TowerSketch(w_d);
    }
    void insert(const char *key, int f = 1)
    {
        if(!towerfilter->insert(key)){
            fermatEle->Insert(*(uint32_t*) key, f);
            //cout<<"ele"<<endl;
        }
        tot_packets++;
    }
    void get_distribution(vector<double> &ed)
    {
        em = new EMFSD1;
        uint32_t *countercpy;
        countercpy = new uint32_t[towerfilter->line[1].width];
        for (int i = 0; i < this->towerfilter->line[1].width; i++)
        {
            countercpy[i] = towerfilter->line[1].index(i);
            //cout << countercpy[i]<<endl;
        }
        em->set_counters(this->towerfilter->line[1].width, countercpy, 65535);
        for (int i = 0; i < FERMAT_EM_ITER; i++)
        {
            printf("%d_epoch\n", i);
            em->next_epoch();
        }
        // printf("%ld\n", em->ns.size());
        // fflush(stdout);
        ed.resize(em->ns.size());
        for (int i = 1; i < em->ns.size(); i++)
        {
            ed[i] = em->ns[i];
        }
        ed.resize(2000000);
        for(auto i : Eleresult){
            if(i.second + towerfilter->threshold > 65535){
                ed[i.second+towerfilter->threshold]++;
                if(ed[65535])
                    ed[65535]-=1;
            }
        }
        int sum = 0;
        for (int i = 1; i < 2000000; i++)
        {
            sum += i * ed[i];
        }
    }
    int get_cardinality()
    {
        int used = 0, total = 0;
        return towerfilter->get_cardinality();
    }
    double get_entropy()
    {
        int tot = 0;
        double entr = 0;
        towerfilter->get_entropy(tot, entr);

        set<uint32_t> checked_items;
        for(auto i : Eleresult){
            checked_items.insert(i.first);
        }
        for(auto it = checked_items.begin();it != checked_items.end();++it){
            uint32_t sum_val = 0;
            uint32_t heavy_val = 0;
            sum_val += Eleresult[*it];
            uint32_t key = *it;

            uint32_t light_val = towerfilter->query8bit((const char*)&key);
            if(light_val != 0){
                sum_val += light_val;
                tot -= light_val;
                entr -= light_val * log2(light_val);
            }
            if(sum_val){
                tot += sum_val;
                entr += sum_val * log2(sum_val);
            }
            
        }
        return -entr / tot + log2(tot);
    }
    void decode()
    {
        if (fermatEle->Decode(Eleresult))
            printf("ele decode ok\n");
        printf("Eleresult: %lu\n", Eleresult.size());
    }
    int query(const char *key)
    {
        if (Eleresult.count(*(uint32_t *)key))
        {
            return towerfilter->threshold + Eleresult[*(uint32_t *)key];
        }
        return towerfilter->query(key);
    }
    double get_entropy(vector<double> &distribution)
    {
        double entropy = 0.0;
        double tot = 0.0;
        double entr = 0.0;
        for (int i = 1; i < distribution.size(); i++)
        {
            if(distribution[i] < 1.0)
                continue;
            tot += i * (int)distribution[i];
            entr += i * distribution[i] * log2(i);
        }
        //tot = tot_packets;
        entropy = -entr / tot + log2(tot);
        return entropy;
    }
    void get_heavy_hitters(set<uint32_t> &hh)
    {
        for (auto i : Eleresult)
        {
            if (query((const char *)&i.first) >= HH_THRESHOLD)
                hh.insert(i.first);
        }
    }
};