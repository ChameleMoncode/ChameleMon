#include <stdio.h>
#include <stdlib.h>
#include <unordered_map>
#include <vector>
#include <set>
#include <cmath>
#include "./src/UnivMon/UnivMon.h"
using namespace std;

int main()
{
    printf("Start accuracy measurement of UnivMon with level: %d, k: %d, row: %d\n", UNIV_LEVEL, UNIV_K, UNIV_ROW);
    fflush(stdout);
    uint32_t totnum_packet = ReadTraces();
    UnivMon<4> *sketches[2];
    UnivMon<4> *um = NULL;
    unordered_map<string, int> true_freqs[2];
    std::vector<std::pair<uint32_t, int>> hc_candidates[2];
    double ave_HH = 0.0, ave_HC = 0.0, ave_card_RE = 0.0, ave_entr_RE = 0.0;
    double ave_HH_are = 0.0;
    for (int times = 0; times < TIMES; times++)
    {
        sketches[0] = new UnivMon<4>(UNIV_BYTES);
        sketches[1] = new UnivMon<4>(UNIV_BYTES);
        true_freqs[0].clear();
        true_freqs[1].clear();
        hc_candidates[0].clear();
        hc_candidates[1].clear();
        for (int iter_data = 0; iter_data < 2; ++iter_data)
        {
            printf("[INFO] %3d-th trace starts to be processed..\n", iter_data + 1);
            fflush(stdout);
            unordered_map<string, int> &true_freq = true_freqs[iter_data];
            unordered_map<string, vector<uint8_t>> mapping_str_srcip;
            vector<int> true_dist(1000000);
            um = sketches[iter_data];
            int num_pkt = (int)traces[iter_data].size();

            for (int i = 0; i < num_pkt; ++i)
            {
                um->insert((uint8_t *)(traces[iter_data][i].key));
                string str((const char *)(traces[iter_data][i].key), 4);
                true_freq[str]++;

                uint8_t key[4];
                memcpy(key, str.c_str(), 4);
                vector<uint8_t> key_vec;
                key_vec.assign(key, key + 4);
                mapping_str_srcip.insert({str, key_vec});
            }
            printf("[INFO] End of insertion process...\n");
            /*-*-*-* End of packet insertion *-*-*-*/
            fflush(stdout);
            set<vector<uint8_t>> HH_true;
            for (unordered_map<string, int>::iterator it = true_freq.begin(); it != true_freq.end(); ++it)
            {
                uint8_t key[4];                      // 32 bits
                memcpy(key, (it->first).c_str(), 4); // flow ID
                if (it->second >= true_dist.size())
                    true_dist.resize(it->second + 1, 0);
                true_dist[it->second] += 1;
                if (it->second > HH_THRESHOLD)
                {
                    vector<uint8_t> temp_key;
                    temp_key.assign(key, key + 4);
                    HH_true.insert(temp_key);
                }
            }
            printf("END\n\n");
            fflush(stdout);
            /****************** HEAVY HITTER *********************/
            std::vector<std::pair<std::string, int>> heavy_hitters;
            um->get_heavy_hitters(HH_THRESHOLD, heavy_hitters);
            set<vector<uint8_t>> HH_estimate;
            for (int i = 0; i < heavy_hitters.size(); ++i)
            {
                if (heavy_hitters[i].second > HH_THRESHOLD)
                    HH_estimate.insert(mapping_str_srcip[heavy_hitters[i].first]);
            }

            double HH_precision = 0;
            double HH_are = 0;
            int HH_PR = 0;
            int HH_PR_denom = 0;
            int HH_RR = 0;
            int HH_RR_denom = 0;
            set<vector<uint8_t>>::iterator itr;
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
                uint8_t key[4];
                string k = "";
                for (int i = 0; i < 4; i++)
                {
                    key[i] = (*itr)[i];
                    k += key[i];
                }
                HH_are += std::abs((double)((int)true_freq[k] - (int)um->query(key))) / (double)true_freq[k];
            }
            if (iter_data == 0)
            {
                HH_precision = (2 * (double(HH_PR) / double(HH_PR_denom)) * (double(HH_RR) / double(HH_RR_denom))) / ((double(HH_PR) / double(HH_PR_denom)) + (double(HH_RR) / double(HH_RR_denom)));
                HH_are /= HH_true.size();
                printf("HH_precision : %3.5f\n", HH_precision);
                printf("HH_PR : %d, HH_PR_denom : %d, HH_RR : %d, HH_RR_denom : %d\n", HH_PR, HH_PR_denom, HH_RR, HH_RR_denom);
                ave_HH += HH_precision;
                printf("HH_are : %3.5f\n", HH_are);
                ave_HH_are += HH_are;
                fflush(stdout);
            }
            fflush(stdout);
            /****************** HEAVY CHANGE*************************/
            um->get_heavy_hitters(HC_THRESHOLD, hc_candidates[iter_data]);

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
                    uint32_t key = f.first;
                    if (f.second - sketches[iter_data - 1]->query((uint8_t *)&key) >
                        HC_THRESHOLD)
                    {
                        est_hc.insert(f.first);
                    }
                }
                for (auto f : hc_candidates[iter_data - 1])
                {
                    uint32_t key = f.first;
                    if (f.second - sketches[iter_data]->query((uint8_t *)&key) >
                        HC_THRESHOLD)
                        est_hc.insert(f.first);
                }
                for (auto f : true_freqs[iter_data])
                {
                    if (!true_freqs[iter_data - 1].count(f.first))
                    {
                        if (f.second > HC_THRESHOLD)
                        {
                            memcpy(&tmp, f.first.c_str(), 4);
                            real_hc.insert(tmp);
                        }
                    }
                    else if (int(f.second - true_freqs[iter_data - 1][f.first]) > HC_THRESHOLD)
                    {
                        memcpy(&tmp, f.first.c_str(), 4);
                        real_hc.insert(tmp);
                    }
                }
                for (auto f : true_freqs[iter_data - 1])
                {
                    if (!true_freqs[iter_data].count(f.first))
                    {
                        if (f.second > HC_THRESHOLD)
                        {
                            memcpy(&tmp, f.first.c_str(), 4);
                            real_hc.insert(tmp);
                        }
                    }
                    else if (int(f.second - true_freqs[iter_data][f.first]) > HC_THRESHOLD)
                    {
                        memcpy(&tmp, f.first.c_str(), 4);
                        real_hc.insert(tmp);
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
                fflush(stdout);
            }

            /********************CARDINALITY*************************/
            if (iter_data == 0)
            {
                double card = um->get_cardinality();
                double card_error = abs(card - int(true_freq.size())) / double(true_freq.size());
                printf("Real cardinality: %d\n", int(true_freq.size()));
                printf("Est_cardinality : %f, RE : %f\n", card, card_error);
                ave_card_RE += card_error;
                /************************************************/

                double entropy_err = 0;
                double entropy_est = um->get_entropy();
                double entropy_true = 0;

                double tot_true = 0;
                double entr_true = 0;
                for (int i = 0; i < true_dist.size(); ++i)
                {
                    if (true_dist[i] == 0)
                        continue;
                    tot_true += i * true_dist[i];
                    entr_true += i * true_dist[i] * log2(i);
                }
                entropy_true = -entr_true / tot_true + log2(tot_true);

                entropy_err = std::abs(entropy_est - entropy_true) / entropy_true;
                printf("ENTROPY ERROR ( true : %f, est : %f) = %f\n", entropy_true, entropy_est, entropy_err);
                ave_entr_RE += entropy_err;
                /***************************************************************/
            }
        }
        delete sketches[0];
        delete sketches[1];
    }
    ave_HH /= TIMES;
    ave_card_RE /= TIMES;
    ave_entr_RE /= TIMES;
    ave_HC /= TIMES;
    ave_HH_are /= TIMES;
    printf("average HH F1 score : %.8lf\n", ave_HH);
    printf("average HH ARE : %.8lf\n", ave_HH_are);
    printf("average HC F1 score : %.8lf\n", ave_HC);
    printf("average cardinality RE : %.8lf\n", ave_card_RE);
    printf("average entropy RE : %.8lf\n", ave_entr_RE);
    printf("End\n");
    printf("\n\n\n");
}
