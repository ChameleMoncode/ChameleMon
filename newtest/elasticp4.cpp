#include "../src/ElasticP4/our_elastic.h"

int main()
{
    printf("Start accuracy measurement of Elastic-P4 version TOTAL_MEMORY %dKB, WIDTH %d, BUCKET %d, STAGES %d\n", TOT_MEM, ELASTIC_WL, ELASTIC_BUCKET, ELASTIC_HEAVY_STAGE);
    uint32_t totnum_packet = ReadTraces();

    ElasticSketch *sketches[2];
    ElasticSketch *elastic = NULL;
    double aveare = 0.0, aveaae = 0.0, ave_HH = 0.0, ave_HC = 0.0, ave_card_RE = 0.0, ave_WMRD = 0.0, ave_entr_RE = 0.0;
    double ave_HH_are = 0.0;
    unordered_map<uint32_t, int> hc_candidates[2];
    unordered_map<uint32_t, uint32_t> true_freqs[2];
    for (int times = 0; times < TIMES; times++)
    {
        sketches[0] = new ElasticSketch();
        sketches[1] = new ElasticSketch();
        true_freqs[0].clear();
        true_freqs[1].clear();
        printf("[INFO] %3d-th trace starts to be processed..\n", times);
        for (int iter_data = 0; iter_data < 2; ++iter_data)
        {
            unordered_map<uint32_t, uint32_t> &true_freq = true_freqs[iter_data];
            vector<int> true_dist(1);
            elastic = sketches[iter_data];
            int num_pkt = (int)traces[iter_data].size();

            for (int i = 0; i < num_pkt; ++i)
            {
                true_freq[*((uint32_t *)(traces[iter_data][i].key))]++;
                elastic->insert((uint8_t *)(traces[iter_data][i].key), 1);
            }
            printf("[INFO] End of insertion process...Num.flow:%d\n", (int)true_freq.size());
            /*-*-*-* End of packet insertion *-*-*-*/

            double ARE = 0;
            double AAE = 0;
            set<uint32_t> HH_true;
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
                uint32_t est_val = elastic->query(key);
                //cout << "true  est :" << it->second << "  " << est_val << endl;
                // if (it->second > est_val)
                //     cout<<elastic->light_part->query(key)<<endl;
                int dist = std::abs((int)it->second - (int)est_val);
                ARE += dist * 1.0 / (it->second);
                AAE += dist * 1.0;
                if (it->second > HH_THRESHOLD)
                    HH_true.insert(it->first);
            }
            ARE /= (int)true_freq.size();
            AAE /= (int)true_freq.size();
            if (iter_data == 0)
            {
                aveare += ARE;
                aveaae += AAE;
                printf("ARE : %.8lf, AAE : %.8lf\n", ARE, AAE);
            }
            /*-*-*-* End of count-query (ARE, AAE) *-*-*-*/
            set<uint32_t> elastic_hh;
            elastic->get_heavy_hitters(elastic_hh);

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
                HH_PR += elastic_hh.find(*itr) != elastic_hh.end();
            }
            for (itr = elastic_hh.begin(); itr != elastic_hh.end(); ++itr)
            {
                HH_RR_denom += 1;
                HH_RR += HH_true.find(*itr) != HH_true.end();
            }
            for (itr = HH_true.begin(); itr != HH_true.end(); ++itr)
            {
                uint32_t key = *itr;
                HH_are += std::abs((double)((int)true_freq[key] - (int)elastic->query((uint8_t *)&key))) / (double)true_freq[key];
                // cout << true_freq[key] <<"  "<< elastic -> query ((uint8_t *) &key)<<"  "<<
                // std::abs((double)((int)true_freq[key] - (int)elastic->query((uint8_t *)&key))) / (double)true_freq[key]<<endl;
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
            /*-*-*-* End of HH detection (F1-score) *-*-*-*/
            elastic->get_hc_candidates(hc_candidates[iter_data]);
            // cout << "true_freq.size()" << true_freq.size() << endl;
            if (iter_data)
            {
                set<uint32_t> fermattower_hc;
                set<uint32_t> est_hc;
                set<uint32_t> real_hc;
                double HC_precision = 0;
                int HC_PR = 0;
                int HC_PR_denom = 0;
                int HC_RR = 0;
                int HC_RR_denom = 0;
                // printf("hc_candidates[%d].size(): %ld\n", iter_data - 1, hc_candidates[iter_data - 1].size());
                for (auto f : hc_candidates[iter_data])
                {
                    if ((int)(sketches[iter_data]->query((uint8_t *)&f.first) -
                              sketches[iter_data - 1]->query((uint8_t *)&f.first)) >
                        (int)HC_THRESHOLD)
                        est_hc.insert(f.first);
                }
                for (auto f : hc_candidates[iter_data - 1])
                {
                    if ((int)(sketches[iter_data - 1]->query((uint8_t *)&f.first) -
                              sketches[iter_data]->query((uint8_t *)&f.first)) >
                        (int)HC_THRESHOLD)
                    {
                        // printf("%d, %d\n",sketches[iter_data - 1].query((uint8_t *)&f.first),sketches[iter_data].query((uint8_t *)&f.first));
                        est_hc.insert(f.first);
                    }
                }
                for (auto f : true_freqs[iter_data])
                {
                    if (!true_freqs[iter_data - 1].count(f.first))
                    {
                        if (f.second > HC_THRESHOLD)
                            real_hc.insert(f.first);
                    }
                    else if (int(f.second - true_freqs[iter_data - 1][f.first]) > HC_THRESHOLD)
                        real_hc.insert(f.first);
                }
                for (auto f : true_freqs[iter_data - 1])
                {
                    if (!true_freqs[iter_data].count(f.first))
                    {
                        if (f.second > HC_THRESHOLD)
                            real_hc.insert(f.first);
                    }
                    else if (int(f.second - true_freqs[iter_data][f.first]) > HC_THRESHOLD)
                        real_hc.insert(f.first);
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
            /*-*-*-* End of HC detection (F1-score) *-*-*-*/
            if (iter_data == 0)
            {
                int card = elastic->get_cardinality();
                double card_err = abs(card - int(true_freq.size())) / double(true_freq.size());
                printf("RE of cardinality : %2.6f (est=%8.0d, true=%8.0d)\n", card_err, card, (int)true_freq.size());
                ave_card_RE += card_err;
                /*-*-*-* End of cardinality estimation *-*-*-*/

                vector<double> dist;
                // compute WMRD
                double WMRD = 0;
                double WMRD_nom = 0;
                double WMRD_denom = 0;
                elastic->get_distribution(dist);
                for (int i = 1; i < true_dist.size(); ++i)
                {
                    if (true_dist[i] == 0)
                        continue;
                    WMRD_nom += std::abs(true_dist[i] - dist[i]);
                    WMRD_denom += double(true_dist[i] + dist[i]) / 2;
                }
                WMRD = WMRD_nom / WMRD_denom;
                printf("WMRD : %3.5f\n", WMRD);
                ave_WMRD += WMRD;
                // entropy initialization
                double entropy_err = 0;
                double entropy_est = elastic->get_entropy();

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
                printf("Entropy Relative Error (RE) = %f (true : %f, est : %f)\n", entropy_err, entropy_true, entropy_est);
                ave_entr_RE += entropy_err;
                /*-*-*-* End of flow size distribution and entropy *-*-*-*/
            }
            printf("\n\n");
        }
    }
    aveare /= TIMES;
    aveaae /= TIMES;
    ave_HH /= TIMES;
    ave_HC /= TIMES;
    ave_card_RE /= TIMES;
    ave_WMRD /= TIMES;
    ave_entr_RE /= TIMES;
    ave_HH_are /= TIMES;
    printf("average ARE : %.8lf, average AAE : %.8lf\n", aveare, aveaae);
    printf("average HH F1 score : %.8lf\n", ave_HH);
    printf("average HH ARE : %.8lf\n", ave_HH_are);
    printf("average HC F1 score : %.8lf\n", ave_HC);
    printf("average cardinality RE : %.8lf\n", ave_card_RE);
    printf("average distribution WMRD : %.8lf\n", ave_WMRD);
    printf("average entropy RE : %.8lf\n", ave_entr_RE);
    printf("END\n");
    printf("\n\n\n");
}