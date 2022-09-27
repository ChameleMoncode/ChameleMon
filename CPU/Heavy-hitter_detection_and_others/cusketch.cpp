#include "./src/CUSketch/CU.h"

int main()
{
	printf("Start accuracy measurement of CUSketch\n");
	uint32_t totnum_packet = ReadTraces();

	CUSketch<4, CU_DEPTH> *cu = NULL;
	double aveare = 0.0, aveaae = 0.0;
	for (int times = 0; times < TIMES; times++)
	{
		for (int iter_data = 0; iter_data < 1; ++iter_data)
		{
			printf("[INFO] %3d-th trace starts to be processed..\n", iter_data + 1);
			unordered_map<uint32_t, uint32_t> true_freq;
			vector<int> true_dist(1000000);
			cu = new CUSketch<4, CU_DEPTH>(CU_BYTES);
			int num_pkt = (int)traces[iter_data].size();

			for (int i = 0; i < num_pkt; ++i)
			{
				cu->insert((uint8_t *)(traces[iter_data][i].key), 1);
				true_freq[*((uint32_t *)(traces[iter_data][i].key))]++;
			}
			printf("[INFO] End of insertion process...\n");
			/*-*-*-* End of packet insertion *-*-*-*/

			double ARE = 0;
			double AAE = 0;
			set<uint32_t> HH_true;
			for (unordered_map<uint32_t, uint32_t>::iterator it = true_freq.begin(); it != true_freq.end(); ++it)
			{
				uint8_t key[4] = {0};					// srcIP-flowkey
				uint32_t temp_first = htonl(it->first); // convert uint32_t -> uint8_t * 4 array
				for (int i = 0; i < 4; ++i)
				{
					key[i] = ((uint8_t *)&temp_first)[3 - i];
				}
				if (it->second >= true_dist.size())
					true_dist.resize(it->second + 1, 0);
				true_dist[it->second] += 1;
				uint32_t est_val = cu->query(key);
				int dist = std::abs((int)it->second - (int)est_val);
				ARE += dist * 1.0 / (it->second);
				AAE += dist * 1.0;
				if (it->second > HH_THRESHOLD)
					HH_true.insert(it->first);
			}
			ARE /= (int)true_freq.size();
			AAE /= (int)true_freq.size();
			aveare += ARE;
			aveaae += AAE;
			printf("ARE : %.8lf, AAE : %.8lf\n", ARE, AAE);
			delete cu;
		}
	}
	aveare /= TIMES;
	aveaae /= TIMES;
	printf("average ARE : %.8lf, average AAE : %.8lf\n", aveare, aveaae);
	printf("END\n");
	printf("\n\n\n");
}
