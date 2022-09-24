#ifndef _FCMSKETCH_H_
#define _FCMSKETCH_H_

#include "./Counter.h"
#include "../Common/BOBHash32.h"
#include "../Common/EM_FCM.h"

class FCMSketch
{
public: // change to private
	Counter<FCMSK_WL1, FCMSK_C1> * C1; // emulate register arrays
	Counter<FCMSK_WL2, FCMSK_C2> * C2; // emulate register arrays
	Counter<FCMSK_WL3, FCMSK_C3> * C3; // emulate register arrays
	BOBHash32* hash[FCMSK_DEPTH] = {NULL}; // BobHash

	set<uint32_t> hh_candidates; // fp = *((uint32_t*)key);
	uint32_t cumul_l2;
	uint32_t cumul_l3;
	
	FCMSketch(){		
		// // check register sizes consistent?
		// if (FCMSK_WL2 != (FCMSK_WL1 >> FCMSK_K_POW) or
		// 	FCMSK_WL3 != (FCMSK_WL2 >> FCMSK_K_POW)){
		// 	printf("[ERROR] Register sizes are not consistent...\n");
		// }			

		// set registers
		C1 = new Counter<FCMSK_WL1, FCMSK_C1>[FCMSK_DEPTH];
		C2 = new Counter<FCMSK_WL2, FCMSK_C2>[FCMSK_DEPTH];
		C3 = new Counter<FCMSK_WL3, FCMSK_C3>[FCMSK_DEPTH];

		// set hash functions
		uint32_t total_memory = 0;
		for (int d = 0; d < FCMSK_DEPTH; ++d){
			hash[d] = new BOBHash32(d + 750);
			total_memory += C1[d].get_memory();
			total_memory += C2[d].get_memory();
			total_memory += C3[d].get_memory();
		}
		printf("[INFO-FCMSketch] Total Memory Usage : %10dbit (%2.2fMB)\n", total_memory, total_memory / 1024.0 / 1024.0);
		printf("[INFO_FCMSketch] Width : {%d, %d, %d}\n", FCMSK_WL1, FCMSK_WL2, FCMSK_WL3);

		// preinstall constants for reducing operation cost
		cumul_l2 = std::numeric_limits<FCMSK_C1>::max() - 1;
		cumul_l3 = cumul_l2 + std::numeric_limits<FCMSK_C2>::max() - 1;
	}
	~FCMSketch(){
		delete[] C1;
		delete[] C2;
		delete[] C3;
		for (int d = 0; d < FCMSK_DEPTH; ++d)
			delete hash[d];
	}

	void insert(uint8_t * key, uint32_t f=1) {
		/* we emulate the pipeline process on PISA */
		int hash_index[FCMSK_DEPTH]; // hash index
		uint32_t ret_val[FCMSK_DEPTH]; // metadata fields
		bool hh_flag = 1; // flag of heavy hitter

		// compute hash index
		for (int d = 0; d < FCMSK_DEPTH; ++d)
			hash_index[d] = init_hashing(key, d);

		// insert packet to counters
		for (int d = 0; d < FCMSK_DEPTH; ++d){
			// stage 1
			ret_val[d] = C1[d].increment(hash_index[d]);
			if (C1[d].check_overflow(hash_index[d])){
				// stage 2
				hash_index[d] = hash_index[d] >> FCMSK_K_POW;
				ret_val[d] = C2[d].increment(hash_index[d]) + cumul_l2; // not overflow at stage 2, and add precomputed accumulated-counts for count-query

				if(C2[d].check_overflow(hash_index[d])){
					// stage 3
					hash_index[d] = hash_index[d] >> FCMSK_K_POW;
					ret_val[d] = C3[d].increment(hash_index[d]) + cumul_l3; // at stage 3, and add precomputed accumulated-counts for count-query
				}
			}

			// check the query is over HH_THRESHOLD (10000)
			if (ret_val[d] <= HH_THRESHOLD)
				hh_flag = 0;
		}

		// report heavy hitter
		if (hh_flag){
			hh_candidates.insert(*((uint32_t*)key));
		}
	}


	uint32_t query(uint8_t * key){
		int hash_index[FCMSK_DEPTH]; // hash index
		uint32_t ret_val[FCMSK_DEPTH] = {}; // metadata fields
		uint32_t count_query = 1 << 30;
		// compute hash index
		for (int d = 0; d < FCMSK_DEPTH; ++d)
			hash_index[d] = init_hashing(key, d);

		for (int d = 0; d < FCMSK_DEPTH; ++d){
			// stage 1
			ret_val[d] = C1[d].query(hash_index[d]);	
			if (C1[d].check_overflow(hash_index[d])){
				// stage 2
				hash_index[d] = hash_index[d] >> FCMSK_K_POW;
				ret_val[d] = C2[d].query(hash_index[d]) + cumul_l2;
				if (C2[d].check_overflow(hash_index[d])){
					// stage 3
					hash_index[d] = hash_index[d] >> FCMSK_K_POW;
					ret_val[d] = C3[d].query(hash_index[d]) + cumul_l3;
				}
			}
			count_query = count_query > ret_val[d] ? ret_val[d] : count_query;
		}
		return count_query;
	}

	int init_hashing(uint8_t * key, int d){
		return (hash[d]->run((const char *)key, 4)) % FCMSK_WL1;
	}


	int get_cardinality(){
		double avgnum_empty_counter = 0;
		for (int d = 0; d < FCMSK_DEPTH; ++d)
			avgnum_empty_counter += C1[d].num_empty;
		return (int)(FCMSK_WL1 * std::log(double(FCMSK_WL1) * FCMSK_DEPTH / avgnum_empty_counter));
	}


    void get_distribution(vector<double> &dist){    	
        /* Making summary note for conversion algorithm (4-tuple summary)
		Each dimension is for (tree, layer, width, tuple)
		*/
		vector<int> get_width{ FCMSK_WL1, FCMSK_WL2, FCMSK_WL3 };
        vector<vector<vector<vector<uint32_t> > > > summary(FCMSK_DEPTH);

        /* To track how paths are merged along the layers */
        vector<vector<vector<vector<vector<uint32_t> > > > > track_thres(FCMSK_DEPTH);

        /*
		When tracking paths, save <a,b,c> in track_thres, where
		a : layer (where paths have met)
		b : number of path that have met (always less or equal than degree of virtual counter)
		c : accumulated max-count value of overflowed registers (e.g., 254, 65534+254, 65534+254+254, etc)
		*/

        for (int d = 0; d < FCMSK_DEPTH; ++d){
            summary[d].resize(FCMSK_LEVEL);
            track_thres[d].resize(FCMSK_LEVEL);
            for (uint32_t i = 0; i < FCMSK_LEVEL; ++i){
                summary[d][i].resize(get_width[i], vector<uint32_t>(4, 0)); //initialize
                track_thres[d][i].resize(get_width[i]); //initialize
                for (int w = 0; w < get_width[i]; ++w){
                	if (i == 0){ // stage 1
	                    summary[d][i][w][2] = 1; // default
	                    summary[d][i][w][3] = C1[d].query(w); // depth 0
	                    if (C1[d].check_overflow(w) == 0){ // not overflow
	                        summary[d][i][w][0] = 1;
	                    }
	                    else { // if counter is overflow
	                        track_thres[d][i][w].push_back(vector<uint32_t>{0, 1, summary[d][i][w][3]});
                    	}
                	}                	
                	else if (i == 1) { // stage 2
                		summary[d][i][w][2] = std::pow(FCMSK_K_ARY,i);
                		for (int t = 0; t < FCMSK_K_ARY; ++t)
                            summary[d][i][w][1] += summary[d][i-1][FCMSK_K_ARY*w+t][0] + summary[d][i-1][FCMSK_K_ARY*w+t][1];
                        	summary[d][i][w][3] = C2[d].query(w);

                        // if child is overflow, then accumulate both "value" and "threshold"
                        for (int k = 0; k < FCMSK_K_ARY; ++k)
                        	if (C1[d].check_overflow(FCMSK_K_ARY*w + k) == 1) { // if child is overflow
                        		summary[d][i][w][3] += summary[d][i-1][FCMSK_K_ARY*w + k][3];
                        		track_thres[d][i][w].insert(track_thres[d][i][w].end(), track_thres[d][i-1][FCMSK_K_ARY*w + k].begin(), track_thres[d][i-1][FCMSK_K_ARY*w + k].end()); 
                        	}

                        if (C2[d].check_overflow(w) == 0) // non-overflow, end of path
                        	summary[d][i][w][0] = summary[d][i][w][2] - summary[d][i][w][1];
                        else { 
                        	// if overflow, then push new threshold <layer, #path, value>
                        	track_thres[d][i][w].push_back(vector<uint32_t>{i, summary[d][i][w][2] - summary[d][i][w][1], summary[d][i][w][3]});
                        }
                    }
                    else if (i == 2) { // stage 3
                		summary[d][i][w][2] = std::pow(FCMSK_K_ARY,i);
                		for (int t = 0; t < FCMSK_K_ARY; ++t)
                            summary[d][i][w][1] += summary[d][i-1][FCMSK_K_ARY*w+t][0] + summary[d][i-1][FCMSK_K_ARY*w+t][1];
                        	summary[d][i][w][3] = C3[d].query(w);

                        // if child is overflow, then accumulate both "value" and "threshold"
                        for (int k = 0; k < FCMSK_K_ARY; ++k)
                        	if (C2[d].check_overflow(FCMSK_K_ARY*w + k) == 1) { // if child is overflow
                        		summary[d][i][w][3] += summary[d][i-1][FCMSK_K_ARY*w + k][3];
                        		track_thres[d][i][w].insert(track_thres[d][i][w].end(), track_thres[d][i-1][FCMSK_K_ARY*w + k].begin(), track_thres[d][i-1][FCMSK_K_ARY*w + k].end()); 
                        	}

                        if (C3[d].check_overflow(w) == 0) // non-overflow, end of path
                        	summary[d][i][w][0] = summary[d][i][w][2] - summary[d][i][w][1];
                        else { 
                        	// if overflow, then push new threshold <layer, #path, value>
                        	track_thres[d][i][w].push_back(vector<uint32_t>{i, summary[d][i][w][2] - summary[d][i][w][1], summary[d][i][w][3]});
                        }
                    }
                    else {
                    	printf("[ERROR] DEPTH(%d) is not mathcing with allocated counter arrays...\n", FCMSK_DEPTH);
                    	return;
                    }
                }
            }
        }

        // make new sketch with specific degree, (depth, degree, value)    
        vector<vector<vector<uint32_t> > > newsk(FCMSK_DEPTH);

        // (depth, degree, vector(layer)<vector(#.paths)<threshold>>>)
        vector<vector<vector<vector<vector<uint32_t> > > > > newsk_thres(FCMSK_DEPTH);

        for (int d = 0; d < FCMSK_DEPTH; ++d){   
            // size = all possible degree
            newsk[d].resize(std::pow(FCMSK_K_ARY,FCMSK_LEVEL-1) + 1);  // maximum degree : k^(L-1) + 1 (except 0 index)
            newsk_thres[d].resize(std::pow(FCMSK_K_ARY,FCMSK_LEVEL-1) + 1); // maximum degree : k^(L-1) + 1 (except 0 index)
            for (int i = 0; i < FCMSK_LEVEL; ++i){
                for (int w = 0; w < get_width[i]; ++w){
                    if (i == 0) // lowest level, degree 1
                    {
                        if (summary[d][i][w][0] > 0 and summary[d][i][w][3] > 0) { // not full and nonzero
                            newsk[d][summary[d][i][w][0]].push_back(summary[d][i][w][3]); 
                            newsk_thres[d][summary[d][i][w][0]].push_back(track_thres[d][i][w]);
                        }
                    }
                    else // upper level
                    {
                        if (summary[d][i][w][0] > 0) { // the highest node that paths could reach
                            newsk[d][summary[d][i][w][0]].push_back(summary[d][i][w][3]);
                            newsk_thres[d][summary[d][i][w][0]].push_back(track_thres[d][i][w]);
                        }
                    }
                }
            }
        }

        // just for debugging, 1 for print, 0 for not print.
        if (0){
            int maximum_val = 0;
            for (int d = 0; d < FCMSK_DEPTH; ++d){
                for (int i = 0; i < newsk[d].size(); ++i){
                    if (newsk[d][i].size() > 0){
                        printf("degree : %d - %lu\n", i, newsk[d][i].size());
                        if (newsk_thres[d][i].size() != newsk[d][i].size()){
                            printf("[Error] newsk and newsk_thres sizes are different!!!\n\n");
                            return;
                        }
                        for (int j = 0; j < newsk[d][i].size(); ++j){
                            printf("[Depth:%d, Degree:%d, index:%d] ==> ", d, i, j);
                            printf("val:%d //", newsk[d][i][j]);
                            for (int k = 0; k < newsk_thres[d][i][j].size(); ++k){
                                printf("<%d, %d, %d>, ", newsk_thres[d][i][j][k][0], newsk_thres[d][i][j][k][1], newsk_thres[d][i][j][k][2]);
                            }
                            maximum_val = (maximum_val < newsk[d][i][j] ? newsk[d][i][j] : maximum_val);
                            printf("\n");
                        }
                    }
                }
                printf("[Depth : %d] Maximum counter value : %d\n\n\n", d, maximum_val);
            }
        }

        EM_FCM<FCMSK_DEPTH, FCMSK_WL1, FCMSK_THL1, FCMSK_THL2> * em_fsd_algo = new EM_FCM<FCMSK_DEPTH, FCMSK_WL1, FCMSK_THL1, FCMSK_THL2>(); // new

        /* now, make the distribution of each degree */
        em_fsd_algo->set_counters(newsk, newsk_thres); // new

        for (int i = 0; i < FCMSK_EM_ITER; ++i){
            em_fsd_algo->next_epoch();
        }

        dist = em_fsd_algo->ns; // vector<double> dist

        // if (0){ // For debugging, 1 if printing final distribution
	       //  for(int i = 0, j = 0; i < (int)dist.size(); ++i){
	       //      if(ROUND_2_INT(dist[i]) > 0)
	       //      {
	       //          printf("<%4d, %4d>", i, ROUND_2_INT(dist[i]));
	       //          if(++j % 10 == 0)
	       //              printf("\n");
	       //          else printf("\t");
	       //      }
	       //  }
	       //  printf("\n\n\n");
        // }
        delete em_fsd_algo;
    }

    // void print_nonempty_ratio()
    // {
    //     uint32_t num_nonzero;
    //     vector<vector<double> > ratio_vec(FCMSK_DEPTH);
    //     for (int d = 0; d < FCMSK_DEPTH; ++d){
    //         num_nonzero = 0; // initialize, // depth d, layer 1
    //         for (int i = 0; i < FCMSK_WL1; ++i)
    //             num_nonzero += (C1[d].get_reg(i) > 0 ? 1 : 0);
    //         printf("D(%d)L(1) Ratio:%f\n", d, num_nonzero * 1.0 / FCMSK_WL1);
    //         ratio_vec[d].push_back(num_nonzero * 1.0 / FCMSK_WL1);

    //         num_nonzero = 0; // initialize, // depth d, layer 2
    //         for (int i = 0; i < FCMSK_WL2; ++i)
    //             num_nonzero += (C2[d].get_reg(i) > 0 ? 1 : 0);
    //         printf("D(%d)L(2) Ratio:%f\n", d, num_nonzero * 1.0 / FCMSK_WL2);
    //         ratio_vec[d].push_back(num_nonzero * 1.0 / FCMSK_WL2);

    //         num_nonzero = 0; // initialize, // depth d, layer 3
    //         for (int i = 0; i < FCMSK_WL3; ++i)
    //             num_nonzero += (C3[d].get_reg(i) > 0 ? 1 : 0);
    //         printf("D(%d)L(3) Ratio:%f\n", d, num_nonzero * 1.0 / FCMSK_WL3);
    //         ratio_vec[d].push_back(num_nonzero * 1.0 / FCMSK_WL3);
    //     }
    //     return;
    // }
};

#endif
