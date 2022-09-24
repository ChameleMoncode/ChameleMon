#include <stdio.h>
#include <stdlib.h>
#include <unordered_map>
#include <vector>
#include <set>
#include <cmath>
#include "../src/CountHeap/CountHeap.h"
#include "../src/common_func.h"
using namespace std;

int main()
{
    printf("Start accuracy measurement of UnivMon with level: %d, k: %d, row: %d\n", UNIV_LEVEL, UNIV_K, UNIV_ROW);
    uint32_t totnum_packet = ReadTraces();
    CountHeap<4, 2000, 3> *sketches[2];
    CountHeap<4, 2000, 3> *ch = NULL;
    unordered_map<uint32_t, int> true_freqs[2];
    std::vector<std::pair<std::string, uint32_t>> hc_candidates[2];
    double ave_HH = 0.0, ave_HC = 0.0, ave_card_RE = 0.0, ave_entr_RE = 0.0;
    double ave_HH_are = 0.0;
    for (int times = 0; times < TIMES; times++)
    {
        true_freqs[0].clear();
        true_freqs[1].clear();
        hc_candidates[0].clear();
        hc_candidates[1].clear();
        sketches[0] = new CountHeap<4, 2000, 3>(TOT_MEM * 1024);
        sketches[1] = new CountHeap<4, 2000, 3>(TOT_MEM * 1024);
        for (int iter_data = 0; iter_data < 2; ++iter_data)
        {
            printf("[INFO] %3d-th trace starts to be processed..\n", iter_data + 1);
            unordered_map<uint32_t, int> &true_freq = true_freqs[iter_data];
            unordered_map<uint32_t, vector<uint8_t>> mapping_str_srcip;
            vector<int> true_dist(1000000);
            ch = sketches[iter_data];
            int num_pkt = (int)traces[iter_data].size();

            for (int i = 0; i < num_pkt; ++i)
            {
                uint32_t key;
                ch->insert((uint8_t *)(traces[iter_data][i].key));
                memcpy(&key, &traces[iter_data][i].key, 4);
                true_freq[key]++;
            }
            printf("[INFO] End of insertion process...\n");
            /*-*-*-* End of packet insertion *-*-*-*/

            set<uint32_t> HH_true;
            for (unordered_map<uint32_t, int>::iterator it = true_freq.begin(); it != true_freq.end(); ++it)
            {
                if (it->second > HH_THRESHOLD)
                {
                    HH_true.insert(it->first);
                }
            }
            printf("END\n\n");

            /****************** HEAVY HITTER *********************/
            std::vector<std::pair<std::string, uint32_t>> heavy_hitters;
            ch->get_heavy_hitters(HH_THRESHOLD, heavy_hitters);
            set<uint32_t> HH_estimate;
            for (int i = 0; i < heavy_hitters.size(); ++i)
            {
                if (heavy_hitters[i].second > HH_THRESHOLD)
                {
                    uint32_t key;
                    memcpy(&key, heavy_hitters[i].first.c_str(), 4);
                    HH_estimate.insert(key);
                }
            }

            double HH_precision = 0;
            double HH_are = 0;
            int HH_PR = 0;
            int HH_PR_denom = 0;
            int HH_RR = 0;
            int HH_RR_denom = 0;
            set<uint32_t>::iterator itr;
            for (itr = HH_true.begin(); itr != HH_true.end(); ++itr)
            {
                HH_PR_denom += 1;
                HH_PR += HH_estimate.find(*itr) != HH_estimate.end();
            }
            for (itr = HH_estimate.begin(); itr != HH_estimate.end(); ++itr)
            {
                HH_RR_denom += 1;
                HH_RR += HH_true.find(*itr) != HH_true.end();
            }
            for (itr = HH_true.begin(); itr != HH_true.end(); ++itr)
            {
                uint32_t key = *itr;
                HH_are += std::abs((double)((int)true_freq[key] - (int)ch->query((uint8_t *)&key))) / (double)true_freq[key];
            }
            HH_are /= HH_true.size();
            HH_precision = (2 * (double(HH_PR) / double(HH_PR_denom)) * (double(HH_RR) / double(HH_RR_denom))) / ((double(HH_PR) / double(HH_PR_denom)) + (double(HH_RR) / double(HH_RR_denom)));
            if (iter_data == 0)
            {
                printf("HH_precision : %3.5f\n", HH_precision);
                printf("HH_PR : %d, HH_PR_denom : %d, HH_RR : %d, HH_RR_denom : %d\n", HH_PR, HH_PR_denom, HH_RR, HH_RR_denom);
                printf("HH_are : %3.5f\n", HH_are);
                ave_HH += HH_precision;
                ave_HH_are += HH_are;
            }
            /****************** HEAVY CHANGE*************************/
            ch->get_heavy_hitters(HC_THRESHOLD, hc_candidates[iter_data]);

            if (iter_data)
            {
                uint32_t tmp;
                set<uint32_t>::iterator itr;
                set<uint32_t> est_hc;
                set<uint32_t> real_hc;
                double HC_precision = 0;
                int HC_PR = 0;
                int HC_PR_denom = 0;
                int HC_RR = 0;
                int HC_RR_denom = 0;
                for (auto f : hc_candidates[iter_data])
                {
                    if (f.second - sketches[iter_data - 1]->query((uint8_t *)&f.first) >
                        HC_THRESHOLD)
                    {
                        memcpy(&tmp, f.first.c_str(), 4);
                        est_hc.insert(tmp);
                    }
                }
                for (auto f : hc_candidates[iter_data - 1])
                {
                    if (f.second - sketches[iter_data]->query((uint8_t *)&f.first) >
                        HC_THRESHOLD)
                    {
                        memcpy(&tmp, f.first.c_str(), 4);
                        est_hc.insert(tmp);
                    }
                }
                for (auto f : true_freqs[iter_data])
                {
                    if (int(f.second - true_freqs[iter_data - 1][f.first]) > HC_THRESHOLD)
                    {
                        real_hc.insert(f.first);
                    }
                }
                for (auto f : true_freqs[iter_data - 1])
                {
                    if (int(f.second - true_freqs[iter_data][f.first]) > HC_THRESHOLD)
                    {
                        real_hc.insert(f.first);
                    }
                }
                for (itr = real_hc.begin(); itr != real_hc.end(); ++itr)
                {
                    HC_PR_denom += 1;
                    HC_PR += est_hc.find(*itr) != est_hc.end();
                }
                for (itr = est_hc.begin(); itr != est_hc.end(); ++itr)
                {
                    HC_RR_denom += 1;
                    HC_RR += real_hc.find(*itr) != real_hc.end();
                }
                HC_precision = (2 * (double(HC_PR) / double(HC_PR_denom)) * (double(HC_RR) / double(HC_RR_denom))) / ((double(HC_PR) / double(HC_PR_denom)) + (double(HC_RR) / double(HC_RR_denom)));
                printf("HC_precision : %3.5f\n", HC_precision);
                printf("HC_PR : %d, HC_PR_denom : %d, HC_RR : %d, HC_RR_denom : %d\n", HC_PR, HC_PR_denom, HC_RR, HC_RR_denom);
                ave_HC += HC_precision;
            }

            /********************CARDINALITY*************************/

            /************************************************/

            /***************************************************************/

            true_freq.clear();
        }
        delete sketches[0];
        delete sketches[1];
    }
    ave_HH /= TIMES;
    ave_HC /= TIMES;
    ave_HH_are /= TIMES;
    printf("average HH F1 score : %.8lf\n", ave_HH);
    printf("average HH ARE : %.8lf\n", ave_HH_are);
    printf("average HC F1 score : %.8lf\n", ave_HC);
    printf("End\n");
    printf("\n\n\n");
}
