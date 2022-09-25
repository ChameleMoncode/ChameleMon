#include "../src/Hashpipe/hashpipe.h"
#include "../src/common_func.h"
int main()
{
    printf("Start accuracy measurement of HashPipe TOTAL_MEMORY %dKB\n", TOT_MEM);
    uint32_t totnum_packet = ReadTraces();

    HashPipe *sketches[2];
    HashPipe *hashpipe = NULL;
    double ave_HH = 0.0;
    double ave_HH_are = 0.0;
    unordered_map<uint32_t, uint32_t> true_freqs[1];
    for (int times = 0; times < TIMES; times++)
    {
        true_freqs[0].clear();
        for (int iter_data = 0; iter_data < 1; ++iter_data)
        {
            printf("[INFO] %3d-th trace starts to be processed..\n", iter_data + 1);
            unordered_map<uint32_t, uint32_t> &true_freq = true_freqs[iter_data];
            vector<int> true_dist(1);
            hashpipe = sketches[iter_data];
            hashpipe = new HashPipe(TOT_MEM * 1024);
            int num_pkt = (int)traces[iter_data].size();

            for (int i = 0; i < num_pkt; ++i)
            {
                true_freq[*((uint32_t *)(traces[iter_data][i].key))]++;
                hashpipe->insert((uint8_t *)(traces[iter_data][i].key));
            }
            printf("[INFO] End of insertion process...Num.flow:%d\n", (int)true_freq.size());
            /*-*-*-* End of packet insertion *-*-*-*/

            unordered_map<uint32_t, int> HH_true;
            for (unordered_map<uint32_t, uint32_t>::iterator it = true_freq.begin(); it != true_freq.end(); ++it)
            {
                uint8_t key[4] = {0};                   // srcIP-flowkey
                uint32_t temp_first = htonl(it->first); // convert uint32_t -> uint8_t * 4 array
                for (int i = 0; i < 4; ++i)
                {
                    key[i] = ((uint8_t *)&temp_first)[3 - i];
                }
                if (it->second >= true_dist.size())
                    true_dist.resize(it->second + 1, 0);
                true_dist[it->second] += 1;
                if (it->second > HH_THRESHOLD)
                    HH_true.insert(make_pair(it->first, it->second));
            }

            /*-*-*-* End of count-query (ARE, AAE) *-*-*-*/
            vector<pair<uint32_t, int>> hashpipe_tmp;
            unordered_map<uint32_t, int> hashpipe_hh;
            hashpipe->get_heavy_hitters(HH_THRESHOLD, hashpipe_tmp);
            for (auto f : hashpipe_tmp)
            {
                hashpipe_hh.insert(f);
            }
            double HH_precision = 0;
            double HH_are = 0;
            int HH_PR = 0;
            int HH_PR_denom = 0;
            int HH_RR = 0;
            int HH_RR_denom = 0;
            unordered_map<uint32_t, int>::iterator itr;
            for (itr = HH_true.begin(); itr != HH_true.end(); ++itr)
            {
                HH_PR_denom += 1;
                HH_PR += hashpipe_hh.find(itr->first) != hashpipe_hh.end();
            }
            for (itr = hashpipe_hh.begin(); itr != hashpipe_hh.end(); ++itr)
            {
                HH_RR_denom += 1;
                HH_RR += HH_true.find(itr->first) != HH_true.end();
            }
            for (itr = HH_true.begin(); itr != HH_true.end(); ++itr)
            {
                uint32_t key = itr->first;
                HH_are += std::abs((double)((int)true_freq[key] - (int)hashpipe_hh[key])) / (double)true_freq[key];
            }
            HH_are /= HH_true.size();
            HH_precision = (2 * (double(HH_PR) / double(HH_PR_denom)) * (double(HH_RR) / double(HH_RR_denom))) / ((double(HH_PR) / double(HH_PR_denom)) + (double(HH_RR) / double(HH_RR_denom)));
            printf("HH_precision : %3.5f\n", HH_precision);
            printf("HH_PR : %d, HH_PR_denom : %d, HH_RR : %d, HH_RR_denom : %d\n", HH_PR, HH_PR_denom, HH_RR, HH_RR_denom);
            printf("HH_are : %3.5f\n", HH_are);
            ave_HH += HH_precision;
            ave_HH_are += HH_are;
            /*-*-*-* End of HH detection (F1-score) *-*-*-*/

            printf("\n\n");
        }
        delete sketches[0];
    }
    ave_HH /= TIMES;
    ave_HH_are /= TIMES;
    printf("average HH F1 score : %.8lf\n", ave_HH);
    printf("average HH ARE : %.8lf\n", ave_HH_are);
    printf("END\n");
    printf("\n\n\n");
}