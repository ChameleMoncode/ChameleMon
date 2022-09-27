#ifndef _THRESHADJUSTER_H_
#define _THRESHADJUSTER_H_

#include <assert.h>
#include <stdio.h>

int dist_len = 0;
double dist_portion[100];
uint64_t dist_thresh[100];

void read_dist(const char *filename) {
    FILE *f = fopen(filename, "r");
    double portion;
    uint64_t thresh;
    while (fscanf(f, "%d %lf", &thresh, &portion) != EOF) {
	uint64_t t = ceil((double)thresh / 1500.0);
	if (dist_len != 0 && dist_thresh[dist_len - 1] == t)
          dist_portion[dist_len - 1] = portion;
	else {
	  dist_portion[dist_len] = portion;
          dist_thresh[dist_len] = t;
          ++dist_len;
	}
    }
}

uint16_t get_HH_thresh(double portion) {
	portion = 1 - portion;
    int i;
    for (i = 0; i < dist_len; ++i) {
        if (dist_portion[i] > portion) {
		printf("%d\n", i);
            break;
	}
    }
    if (i == 0)
	return 0;
    return dist_thresh[i - 1] + ceil((portion - dist_portion[i - 1]) * 
      (dist_thresh[i] - dist_thresh[i - 1]) / (dist_portion[i] - dist_portion[i - 1]));
}

double get_portion(uint64_t HH_thresh) {
    int i;
    for (i = 0; i < dist_len; ++i) {
        if (dist_thresh[i] > HH_thresh)
            break;
    }
    if (i == 0)
	    return 1;
    double portion = dist_portion[i - 1] + (HH_thresh - dist_thresh[i - 1]) * 
      (dist_portion[i] - dist_portion[i - 1]) / (dist_thresh[i] - dist_thresh[i - 1]);
    return 1 - portion;
}

#endif
