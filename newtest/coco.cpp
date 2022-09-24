#include "../src/Coco/OurHard.h"
#include "../src/common_func.h"
#include <iostream>
using namespace std;
typedef OurHard<uint32_t> Coco;
int main()
{

    printf("Start accuracy measurement of Coco\n");
    uint32_t totnum_packet = ReadTraces();

    Coco *coco = NULL;
    Coco *sketches[2];
    unordered_map<uint32_t, uint32_t> true_freqs[2];
    unordered_map<uint32_t, int> hc_candidates[2];
    double aveare = 0.0, aveaae = 0.0, ave_HH = 0.0, ave_HC = 0.0, ave_card_RE = 0.0, ave_WMRD = 0.0, ave_entr_RE = 0.0;
    double ave_HH_are = 0.0;
    for (int times = 0; times < TIMES; times++)
    {
        sketches[0] = new Coco(TOT_MEM * 1024);
        sketches[1] = new Coco(TOT_MEM * 1024);
        true_freqs[0].clear();
        true_freqs[1].clear();
        hc_candidates[0].clear();
        hc_candidates[1].clear();
        for (int iter_data = 0; iter_data < 2; ++iter_data)
        {
            printf("[INFO] %3d-th trace starts to be processed..\n", iter_data + 1);
            unordered_map<uint32_t, uint32_t> &true_freq = true_freqs[iter_data];
            vector<int> true_dist(1000000);
            coco = sketches[iter_data];
            int num_pkt = (int)traces[iter_data].size();

            for (int i = 0; i < num_pkt; ++i)
            {
                coco->Insert(*(uint32_t *)traces[iter_data][i].key);
                true_freq[*((uint32_t *)(traces[iter_data][i].key))]++;
            }
            unordered_map<uint32_t, int> est_freq = coco->AllQuery();
            /*-*-*-* End of packet insertion *-*-*-*/
            // double ARE = 0;
            // double AAE = 0;

            unordered_map<uint32_t, int> HH_true;
            HH_true.clear();
            for (unordered_map<uint32_t, uint32_t>::iterator it = true_freq.begin(); it != true_freq.end(); ++it)
            {
                // uint8_t key[4] = {0};                   // srcIP-flowkey
                // uint32_t temp_first = htonl(it->first); // convert uint32_t -> uint8_t * 4 array
                // for (int i = 0; i < 4; ++i)
                // {
                //     key[i] = ((uint8_t *)&temp_first)[3 - i];
                // }
                // if (it->second >= true_dist.size())
                //     true_dist.resize(it->second + 1, 0);
                // true_dist[it->second] += 1;
                // uint32_t est_val = est_freq[it->first];
                // int dist = std::abs((int)it->second - (int)est_val);
                // printf("est val, true, dist : %ld, %ld, %ld\n", est_val, it->second, dist);
                // fflush(stdout);
                // ARE += dist * 1.0 / (it->second);
                // AAE += dist * 1.0;
                if (it->second > HH_THRESHOLD)
                {
                    uint32_t key = it->first;
                    HH_true.insert(make_pair(it->first, (int)it->second));
                }
                // accum_error[it->second] += dist * 1.0;
            }

            // ARE /= (int)true_freq.size();
            // AAE /= (int)true_freq.size();
            // printf("ARE : %.8lf, AAE : %.8lf\n", ARE, AAE);
            // aveare += ARE;
            // aveaae += AAE;
            // fflush(stdout);
            /*-*-*-* End of count-query (ARE, AAE) *-*-*-*/
            unordered_map<uint32_t, int> HH_est;
            for (auto f : est_freq)
            {
                if (f.second > HH_THRESHOLD)
                    HH_est.insert(make_pair(f.first, (int)f.second));
            }
            double HH_ARE = 0;
            double HH_precision = 0;
            int HH_PR = 0;
            int HH_PR_denom = 0;
            int HH_RR = 0;
            int HH_RR_denom = 0;
            unordered_map<uint32_t, int>::iterator itr;
            uint32_t key;
            for (itr = HH_true.begin(); itr != HH_true.end(); ++itr)
            {
                HH_PR_denom += 1;
                HH_PR += HH_est.count(itr->first) != 0;
                HH_ARE += std::abs((int)true_freq[itr->first] - est_freq[itr->first]) * 1.0 / true_freq[itr->first];
            }
            for (itr = HH_est.begin(); itr != HH_est.end(); ++itr)
            {
                HH_RR_denom += 1;
                HH_RR += HH_true.count(itr->first) != 0;
            }
            HH_precision = (2 * (double(HH_PR) / double(HH_PR_denom)) * (double(HH_RR) / double(HH_RR_denom))) / ((double(HH_PR) / double(HH_PR_denom)) + (double(HH_RR) / double(HH_RR_denom)));
            if (iter_data == 0)
            {
                printf("HH_precision : %3.5f\n", HH_precision);
                printf("HH_PR : %d, HH_PR_denom : %d, HH_RR : %d, HH_RR_denom : %d\n", HH_PR, HH_PR_denom, HH_RR, HH_RR_denom);
                HH_ARE /= HH_true.size();
                printf("HH_ARE : %3.5f\n", HH_ARE);
                ave_HH_are += HH_ARE;
                ave_HH += HH_precision;
            }

            /*-*-*-* End of HH detection (F1-score) *-*-*-*/
            for (auto f : est_freq)
            {
                if (f.second > HC_THRESHOLD)
                {
                    hc_candidates[iter_data].insert(f);
                }
            }
            if (iter_data)
            {

                unordered_map<uint32_t, int> est_hc;
                unordered_map<uint32_t, int> real_hc;
                double HC_precision = 0;
                int HC_PR = 0;
                int HC_PR_denom = 0;
                int HC_RR = 0;
                int HC_RR_denom = 0;
                for (auto f : hc_candidates[iter_data])
                {
                    if (int(f.second - sketches[iter_data - 1]->query(f.first)) > (int)HC_THRESHOLD)
                        est_hc.insert(f);
                }
                for (auto f : hc_candidates[iter_data - 1])
                {
                    if (int(f.second - sketches[iter_data]->query(f.first)) > (int)HC_THRESHOLD)
                        est_hc.insert(f);
                }
                for (auto f : true_freqs[iter_data])
                {
                    if (!true_freqs[iter_data - 1].count(f.first))
                    {
                        if (f.second > HC_THRESHOLD)
                            real_hc.insert(f);
                    }
                    else if (int(f.second - true_freqs[iter_data - 1][f.first]) > (int)HC_THRESHOLD)
                        real_hc.insert(f);
                }
                for (auto f : true_freqs[iter_data - 1])
                {
                    if (!true_freqs[iter_data].count(f.first))
                    {
                        if (f.second > HC_THRESHOLD)
                            real_hc.insert(f);
                    }
                    else if (int(f.second - true_freqs[iter_data][f.first]) > (int)HC_THRESHOLD)
                        real_hc.insert(f);
                }
                for (auto itr = real_hc.begin(); itr != real_hc.end(); ++itr)
                {
                    HC_PR_denom += 1;
                    HC_PR += est_hc.count(itr->first) != 0;
                }
                for (auto itr = est_hc.begin(); itr != est_hc.end(); ++itr)
                {
                    HC_RR_denom += 1;
                    HC_RR += real_hc.count(itr->first) != 0;
                }
                HC_precision = (2 * (double(HC_PR) / double(HC_PR_denom)) * (double(HC_RR) / double(HC_RR_denom))) / ((double(HC_PR) / double(HC_PR_denom)) + (double(HC_RR) / double(HC_RR_denom)));
                printf("HC_precision : %3.5f\n", HC_precision);
                printf("HC_PR : %d, HC_PR_denom : %d, HC_RR : %d, HC_RR_denom : %d\n", HC_PR, HC_PR_denom, HC_RR, HC_RR_denom);
                ave_HC += HC_precision;
            }
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
    printf("END\n");
    printf("\n\n\n");
}