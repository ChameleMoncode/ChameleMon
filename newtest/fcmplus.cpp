// #include "./common_func.h"
#include "../src/FCMPlus/FCMPlus.h"

int main()
{
    if ((FCMPLUS_WL3 * FCMPLUS_K_ARY != FCMPLUS_WL2) or (FCMPLUS_WL2 * FCMPLUS_K_ARY != FCMPLUS_WL1))
        throw std::invalid_argument( "FCM+TopK has inconsistent counter number for k-ary tree." );

	printf("Start accuracy measurement of FCM+TopK\n");
	uint32_t totnum_packet = ReadTraces();

	FCMPlus * fcmplus = NULL;
    FCMPlus sketches[NUM_TRACE];
    unordered_map<uint32_t, uint32_t> true_freqs[NUM_TRACE];
    unordered_map<uint32_t, int> hc_candidates[NUM_TRACE];
    double aveare = 0.0, aveaae = 0.0, ave_HH = 0.0, ave_HC = 0.0, ave_card_RE = 0.0, ave_WMRD = 0.0, ave_entr_RE = 0.0;
    double ave_HH_are = 0.0;
	for(int iter_data = 0; iter_data < NUM_TRACE; ++iter_data){
		printf("[INFO] %3d-th trace starts to be processed..\n", iter_data+1);
		unordered_map<uint32_t, uint32_t>& true_freq = true_freqs[iter_data];
		vector<int> true_dist(1000000);
        fcmplus = sketches + iter_data;
		int num_pkt = (int)traces[iter_data].size();

		for (int i = 0; i < num_pkt; ++i){
			fcmplus->insert((uint8_t*)(traces[iter_data][i].key), 1);
			true_freq[*((uint32_t*)(traces[iter_data][i].key))]++;
		}

		/*-*-*-* End of packet insertion *-*-*-*/

		double ARE = 0;
		double AAE = 0;
		set<uint32_t> HH_true;
		for(unordered_map<uint32_t, uint32_t>::iterator it = true_freq.begin(); it != true_freq.end(); ++it){
			uint8_t key[4] = {0}; // srcIP-flowkey
			uint32_t temp_first = htonl(it->first); // convert uint32_t -> uint8_t * 4 array
	        for (int i=0; i<4 ;++i){
            	key[i] = ((uint8_t*)&temp_first)[3-i];
	        }
	        if (it->second >= true_dist.size())
	        	true_dist.resize(it->second + 1, 0);
	        true_dist[it->second] += 1;
	        uint32_t est_val = fcmplus->query(key);
	        int dist = std::abs((int)it->second - (int)est_val);
	        //printf("est val, true, dist : %ld, %ld, %ld\n", est_val, it->second, dist);
	        ARE += dist * 1.0 / (it->second);
	        AAE += dist * 1.0;
	        if (it->second > HH_THRESHOLD)
	        	HH_true.insert(it->first);
            // accum_error[it->second] += dist * 1.0;
		}
        fflush(stdout);

		ARE /= (int)true_freq.size();
		AAE /= (int)true_freq.size();
		printf("ARE : %.8lf, AAE : %.8lf\n", ARE, AAE);
        aveare += ARE;
        aveaae += AAE;
        fflush(stdout);
		/*-*-*-* End of count-query (ARE, AAE) *-*-*-*/
        fcmplus->push_hh();
        set<uint32_t> HH_est;
        for(auto f : fcmplus->hh_candidates)
        {
            if (fcmplus->query((uint8_t*)&f) > HH_THRESHOLD)
                HH_est.insert(f);
        }
        double HH_ARE = 0;
		double HH_precision = 0;
        int HH_PR = 0;
        int HH_PR_denom = 0;
        int HH_RR = 0;
        int HH_RR_denom = 0;
        set<uint32_t>::iterator itr;
        uint32_t key;
        for (itr = HH_true.begin(); itr != HH_true.end(); ++itr)
        {
            HH_PR_denom += 1;
            HH_PR += HH_est.find(*itr) != HH_est.end();
            key = *itr;
            HH_ARE += std::abs((int)true_freq[*itr] - (int)fcmplus->query((uint8_t *)&key)) * 1.0 / true_freq[*itr];
        }
        for (itr = HH_est.begin(); itr != HH_est.end(); ++itr)
        {
            HH_RR_denom += 1;
            HH_RR += HH_true.find(*itr) != HH_true.end();
        }
        HH_precision = (2 * (double(HH_PR) / double(HH_PR_denom)) * (double(HH_RR) / double(HH_RR_denom))) / ((double(HH_PR) / double(HH_PR_denom)) + (double(HH_RR) / double(HH_RR_denom)));
        printf("HH_precision : %3.5f\n", HH_precision);
        printf("HH_PR : %d, HH_PR_denom : %d, HH_RR : %d, HH_RR_denom : %d\n", HH_PR, HH_PR_denom, HH_RR, HH_RR_denom);
        HH_ARE /= HH_true.size();
        printf("HH_ARE : %3.5f\n",HH_ARE);
        ave_HH_are += HH_ARE;
        ave_HH += HH_precision;
        fflush(stdout);

		/*-*-*-* End of HH detection (F1-score) *-*-*-*/
        for(auto f : fcmplus->hh_candidates)
        {
            if (fcmplus->query((uint8_t*)&f) > HC_THRESHOLD){
                hc_candidates[iter_data].insert(make_pair(f, fcmplus->query((uint8_t *)&f)));
            }
        }
        if(iter_data)
        {
            // cout<<hc_candidates[iter_data - 1].size()<<endl;
            // for(auto f : hc_candidates[iter_data - 1])
            //     cout << hex <<f.first <<"  "<<dec << f.second <<endl;
            set<uint32_t> est_hc;
            set<uint32_t> real_hc;
            double HC_precision = 0;
            int HC_PR = 0;
            int HC_PR_denom = 0;
            int HC_RR = 0;
            int HC_RR_denom = 0;
            for(auto f : hc_candidates[iter_data])
            {
                if (int(f.second - sketches[iter_data - 1].query((uint8_t *)&f.first)) > (int)HC_THRESHOLD)
                    est_hc.insert(f.first);
            }
            for(auto f : hc_candidates[iter_data - 1])
            {
                if (int(f.second - sketches[iter_data].query((uint8_t *)&f.first)) > (int)HC_THRESHOLD)
                    est_hc.insert(f.first);
            }
            for(auto f : true_freqs[iter_data])
            {
                if(!true_freqs[iter_data - 1].count(f.first)){
                    if(f.second > HC_THRESHOLD)
                        real_hc.insert(f.first);
                }
                else
                    if(int(f.second - true_freqs[iter_data - 1][f.first]) > (int)HC_THRESHOLD)
                        real_hc.insert(f.first);
            }
            for(auto f : true_freqs[iter_data - 1])
            {
                if(!true_freqs[iter_data].count(f.first)){
                    if(f.second >  HC_THRESHOLD)
                        real_hc.insert(f.first);
                }
                else
                    if(int(f.second - true_freqs[iter_data][f.first]) > (int)HC_THRESHOLD)
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
        /*-*-*-* End of HH detection (F1-score) *-*-*-*/
		int card = fcmplus->get_cardinality();
		double card_err = abs(card - int(true_freq.size())) / double(true_freq.size());
		printf("RE of cardinality : %2.6f (est=%8.0d, true=%8.0d)\n", card_err, card, (int)true_freq.size());
        ave_card_RE += card_err;

		// /*-*-*-* End of cardinality estimation *-*-*-*/
		      
        // // compute WMRD
        // double WMRD = 0;
        // double WMRD_nom = 0;
        // double WMRD_denom = 0;

        // // entropy initialization
        // double entropy_err = 0;
        // double entropy_est = 0;
        // double entropy_true = 0;


        // vector<double> dist;
        // fcmplus->get_distribution(dist);

        // for (int i = 1; i < true_dist.size(); ++i)
        // {
        //     if (true_dist[i] == 0)
        //         continue;
        //     WMRD_nom += std::abs(true_dist[i] - dist[i]);
        //     WMRD_denom += double(true_dist[i] + dist[i])/2;
        // }
        // WMRD = WMRD_nom / WMRD_denom;
        // printf("WMRD : %3.5f\n", WMRD);
        // ave_WMRD += WMRD;

        // double tot_est = 0;
        // double entr_est = 0;

        // for (int i = 1; i < dist.size(); ++i)
        // {
        //     if (dist[i] == 0)
        //         continue;
        //     tot_est += i * dist[i];
        //     entr_est += i * dist[i] * log2(i);
        // }
        // entropy_est = - entr_est / tot_est + log2(tot_est);

        // double tot_true = 0;
        // double entr_true = 0;
        // for (int i = 0; i < true_dist.size(); ++i)
        // {
        //     if (true_dist[i] == 0)
        //         continue;
        //     tot_true += i * true_dist[i];
        //     entr_true += i * true_dist[i] * log2(i);
        // }
        // entropy_true = - entr_true / tot_true + log2(tot_true);

        // entropy_err = std::abs(entropy_est - entropy_true) / entropy_true;
        // printf("Entropy Relative Error (RE) = %f (true : %f, est : %f)\n", entropy_err, entropy_true, entropy_est);
		// ave_entr_RE += entropy_err;
        /*-*-*-* End of flow size distribution and entropy *-*-*-*/


	}
    aveare /= NUM_TRACE;
    aveaae /= NUM_TRACE;
    ave_HH /= NUM_TRACE;
    ave_HC /= NUM_TRACE - 1;
    ave_card_RE /= NUM_TRACE;
    ave_WMRD /= NUM_TRACE;
    ave_entr_RE /= NUM_TRACE;
    ave_HH_are /= NUM_TRACE;
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