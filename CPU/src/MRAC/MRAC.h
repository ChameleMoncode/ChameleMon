#ifndef STREAMMEASUREMENTSYSTEM_MRAC_H
#define STREAMMEASUREMENTSYSTEM_MRAC_H

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "../Common/BOBHash32.h"
#include "../Common/EMFSD.h"


/* Key Len = 4 Bytes (Source-IP) */
class MRAC {
    uint32_t w;
    uint32_t * counters;
    BOBHash32 *bob_hash;

    double est_cardinality = 0;

    EMFSD *em_fsd_algo = NULL;

  public:
    string name;

    MRAC(int TOT_MEM_IN_BYTES) {
        w = TOT_MEM_IN_BYTES / 4;
        srand(time(0));
        counters = new uint32_t[w];
        memset(counters, 0, sizeof(uint32_t) * w);
        bob_hash = new BOBHash32(rand() % MAX_PRIME32);
        printf("width : %d\n", w);
    }

    ~MRAC(){
        delete[] counters;
        delete bob_hash;
    }

    void insert(uint8_t *item) {
        uint32_t pos = bob_hash->run((const char *)item, 4) % w;
        counters[pos] += 1;
    }

    void collect_fsd() {
        em_fsd_algo = new EMFSD();
        em_fsd_algo->set_counters(w, counters);
    }

    void next_epoch() { em_fsd_algo->next_epoch(); }
    void get_distribution(vector<double> &dist_est) {
        collect_fsd();
        for (int i = 0; i < MRAC_EM_ITER; ++i)
        {
            next_epoch();
            printf("[EM] %d th epoch...with cardinality : %8.2f\n", i, em_fsd_algo->n_sum);
        }
        dist_est = em_fsd_algo->ns; // vector<double> dist
        
    }

    double get_cardinality() {
        return em_fsd_algo->n_sum;
    }
};

#endif // STREAMMEASUREMENTSYSTEM_MRAC_H
