#ifndef _MEMORY_ADJUSTER_H_
#define _MEMORY_ADJUSTER_H_

#include "fermatStruct.h"
#include <math.h>
#include <stdlib.h>

int cmp(const void *a, const void *b) {
	return (*(int*)a - *(int *)b);
}

int estimateFlowNum(struct FermatSketch (*a)[ingress_entry_num], unsigned int base, unsigned int width) {
    int empty_bucket_num[array_num] = { 0 };
    for (int k = 0; k < array_num; ++k)
      for (int i = base; i < base + width; ++i)
        empty_bucket_num[k] += ((a[k][i].counter == 0)? 1 : 0);

    // printf("estimate %d %d %d\n", empty_bucket_num[0], empty_bucket_num[1], empty_bucket_num[2]);
    // unsigned int avg_empty_num = 0;
    // for (int i = 0; i < array_num; ++i)
    //   avg_empty_num += empty_bucket_num[i];
    // avg_empty_num /= array_num;
    // avg_empty_num = avg_empty_num > 1 ? avg_empty_num : 1;
    for (int i = 0; i < array_num; ++i)
	    for (int j = 0; j < array_num - 1 - i; ++j)
		    if (empty_bucket_num[j] > empty_bucket_num[j + 1]) {
			    int t = empty_bucket_num[j+1];
			    empty_bucket_num[j+1]= empty_bucket_num[j];
			    empty_bucket_num[j] = t;
		    }
    // printf("estimate %d %d %d\n", empty_bucket_num[0], empty_bucket_num[1], empty_bucket_num[2]);
    int avg_empty_num = empty_bucket_num[array_num / 2];
    avg_empty_num = avg_empty_num > 1 ? avg_empty_num : 1;
    double t = (-(double)width) * log((double) avg_empty_num / (double)width);
    return t;
}


#endif
