#pragma once
#include "fermat.h"
#include "tower_sketch.h"
#include <map>
#include "EMFSD.h"
#include "../common_func.h"
using namespace std;
class Fermat_tower
{
public:
    int tot_memory;
    int fermatEleMem, fermatMiceMem;
    int towerfilterMem;
    Fermat *fermatEle, *fermatMice;
    TowerSketch *towerfilter;
    unordered_map<uint32_t, int> Eleresult, Miceresult;
    EMFSD *em = NULL;

public:
    Fermat_tower(int _tot_memory = TOT_MEMORY, int _fermatEleMem = ELE_BUCKET * (8 + 4 * USE_FING),
                 int _fermatMiceMem = MICE_BUCKET * (8 + 4 * USE_FING),
                 int _towerfilterMem = TOT_MEMORY - (ELE_BUCKET + MICE_BUCKET) * (8 + 4 * USE_FING),
                 bool usefing = USE_FING, uint32_t _init = INIT) : tot_memory(_tot_memory), fermatEleMem(_fermatEleMem), fermatMiceMem(_fermatMiceMem), towerfilterMem(_towerfilterMem)
    {
        fermatEle = new Fermat(fermatEleMem, usefing, _init);
        fermatMice = new Fermat(fermatMiceMem, usefing, _init);
        int w_d = towerfilterMem / 12;
        towerfilter = new TowerSketch(w_d);
    }
    void insert(const char *key, int f = 1)
    {
        if (!towerfilter->insert(key))
        {
            fermatEle->Insert(*(uint32_t *)key, f);
        }
        else
        {
            int newval = towerfilter->query(key);
            fermatMice->Insert(*(uint32_t *)key, f);
        }
    }
    void get_distribution(vector<double> &ed)
    {
        // em = new EMFSD;
        // uint32_t *countercpy;
        // countercpy = new uint32_t[towerfilter->line[1].width];
        // for (int i = 0; i < this->towerfilter->line[1].width; i++)
        // {
        //     countercpy[i] = towerfilter->line[1].index(i);
        // }
        // em->set_counters(this->towerfilter->line[1].width, countercpy);
        // for (int i = 0; i < 10; i++)
        // {
        //     printf("%d_epoch\n", i);
        //     fflush(stdout);
        //     em->next_epoch();
        // }
        // printf("%ld\n", em->ns.size());
        // ed.resize(em->ns.size());
        // for (int i = 1; i < em->ns.size(); i++)
        // {
        //     //printf("%d\n", i);
        //     ed[i] = em->ns[i];
        // }
        // ed.resize(100000);
        // for(auto i : Eleresult){
        //     ed[i.second+HH_THRESHOLD]++;
        // }
        // int cnt = 0;
        // for (auto i : ed)
        // {
        // 	cnt++;
        // 	printf("<%d, %d>    ", i.first, i.second);
        // 	if (cnt % 10 == 0)
        // 		printf("\n");
        // }
        towerfilter->get_distribution(ed);
        ed.resize(100000);
        for (auto i : Eleresult)
        {
            ed[i.second + 255]+=1.0;
            ed[255] = max(ed[255]-1.0, 0.0);
        }
    }
    int get_cardinality()
    {
        int used = 0, total = 0;
        return towerfilter->get_cardinality();
    }
    void decode()
    {
        // int cnt = 0;
        // for(int i=0;i<towerfilter->line[1].width;i++)
        // {
        //     if(towerfilter->line[1].index(i) >= HH_THRESHOLD)
        //     {
        //         printf("%d ",i);
        //         cnt++;
        //         if(cnt % 20 == 0)
        //         printf("\n");
        //     }
        // }
        // printf("cnt: %d\n", cnt);
        if (fermatEle->Decode(Eleresult))
            printf("ele decode ok\n");
        printf("Eleresult: %lu\n", Eleresult.size());
        if (fermatMice->Decode(Miceresult))
            printf("mice decode ok\n");
        printf("Miceresult: %lu\n", Miceresult.size());
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
        double entr = 0.0;
        int all = 0;
        for (int i = 1; i < distribution.size(); i++)
        {
            if(distribution[i] < 1.0)
            continue;
            all += i * (int)distribution[i];
        }
        for (int i = 1; i < distribution.size(); i++)
        {
            if (distribution[i] <= 0)
                continue;
            entr -= (1.0 * i * distribution[i] / (double)all) * log2((double)i / (double)all);
        }
        return entr;
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