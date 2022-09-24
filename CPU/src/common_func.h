#ifndef _COMMON_FUNC_H_
#define _COMMON_FUNC_H_

#include <iostream>
#include <utility>
#include <unistd.h>
#include <stdint.h>
#include <vector>
#include <set>
#include <map>
#include <unordered_map>
#include <algorithm>
#include <fstream>
#include <arpa/inet.h>
#include <cstring>
#include <random>
#include <stdexcept>

using std::make_pair;
using std::map;
using std::pair;
using std::set;
using std::string;
using std::unordered_map;
using std::vector;

/************************** Loading Traces ******************************/

#define NUM_TRACE 12               // Number of traces in DATA directory
#define TIMES 10                   // Times of each algorithm measuring the same
                                   // trace using different hash function
#define DATA_ROOT_15s "/data/data" // NUM_TRACE = 1, CAIDA

struct SRCIP_TUPLE
{
  char key[4];
};
struct REST_TUPLE
{
  char key[9];
};

typedef vector<SRCIP_TUPLE> TRACE;

TRACE traces[NUM_TRACE];

uint32_t ReadTraces()
{
  double starttime, nowtime;
  uint32_t total_pck_num = 0;
  string filename130000 = "../data/130000.dat";
  char tmp[21] = {0};
  FILE *file1 = fopen(filename130000.c_str(), "r");
  if (file1 == NULL)
  {
    printf("[ERROR] file not open\n");
    exit(0);
  }
  SRCIP_TUPLE key;
  int window = 0;
  if (!fread(tmp, 21, 1, file1)){
    printf("[ERROR] file error!\n");
    exit(0);
  }
  starttime = *(double *)(tmp + 13);
  while (fread(tmp, 21, 1, file1))
  {
    nowtime = *(double *)(tmp + 13);
    if (nowtime - starttime >= 5.0)
    {
      window++;
      starttime = nowtime;
    }
    memcpy(&key, tmp, 4);
    traces[window].push_back(key);
    total_pck_num++;
  }
  printf("[INFO] 12 windows scanned, packets number:\n");
  for (int i = 0; i < 12; i++)
    printf("[INFO] window %02d has %ld packets\n", i, traces[i].size());
  printf("\n\n");
  return total_pck_num;
}
/************************** PREDEFINED NUMBERS***********************/
#define HH_THRESHOLD 500 // 20,000,000 * 0.0005 (0.05%)
#define HC_THRESHOLD 250
#define TOT_MEM 500
/************************** COMMON FUNCTIONS*************************/
#define ROUND_2_INT(f) ((int)(f >= 0.0 ? (f + 0.5) : (f - 0.5)))

/********************************************************************/

// Fermat_tower
#define TOT_MEMORY TOT_MEM * 1024 
#define ELE_BUCKET 2500
#define ELE_THRESHOLD 250
#define USE_FING 0
#define INIT ((uint32_t)random() % 800)
#define FERMAT_EM_ITER 15
/********************************************************************/

// FCM+TopK (16-ary)
// Here, we consider the actual hardware implementation on Tofino.
// The actual register size of each bucket in each Top-K entry is (8 * 3 + 4) = 28 Byte,
// which is composed of 1 val_all (4B) + 3 key-value pairs (4 + 4) = 28 Byte.

#define JUDGE_IF_SWAP_FCMPLUS_P4(min_val, guard_val) ((guard_val >> 5) >= min_val)
#define FCMPLUS_DEPTH 2     // number of trees
#define FCMPLUS_LEVEL 3     // number of layer in trees
#if TOT_MEM >= 200
#define FCMPLUS_BUCKET 3072 // 2^12, num of entries for key-value pairs
#elif TOT_MEM == 150
#define FCMPLUS_BUCKET 1536
#elif TOT_MEM == 100
#define FCMPLUS_BUCKET 1024
#elif TOT_MEM == 50
#define FCMPLUS_BUCKET 512
#endif
#define FCMPLUS_K_ARY 16    // k-ary tree

#if FCMPLUS_K_ARY == 2
#define FCMPLUS_K_POW 1 // 2^1 = 2
#elif FCMPLUS_K_ARY == 4
#define FCMPLUS_K_POW 2 // 2^2 = 4
#elif FCMPLUS_K_ARY == 8
#define FCMPLUS_K_POW 3 // 2^3 = 8
#elif FCMPLUS_K_ARY == 16
#define FCMPLUS_K_POW 4 // 2^4 = 16
#elif FCMPLUS_K_ARY == 32
#define FCMPLUS_K_POW 5 // 2^5 = 32
#endif

// Config using 1.25MB
#define FCMPLUS_HEAVY_STAGE 4
#if TOT_MEM == 1000
#define FCMPLUS_WL1 384000 // width of layer 1 (number of registers)
#define FCMPLUS_WL2 24000   // width of layer 2 (number of registers)
#define FCMPLUS_WL3 1500    // width of layer 3 (number of registers)
#elif TOT_MEM == 800
#define FCMPLUS_WL1 294400
#define FCMPLUS_WL2 18400
#define FCMPLUS_WL3 1150
#elif TOT_MEM == 600
#define FCMPLUS_WL1 204800
#define FCMPLUS_WL2 12800
#define FCMPLUS_WL3 800
#elif TOT_MEM == 500
#define FCMPLUS_WL1 160000
#define FCMPLUS_WL2 10000
#define FCMPLUS_WL3 625
#elif TOT_MEM == 400
#define FCMPLUS_WL1 115200
#define FCMPLUS_WL2 7200
#define FCMPLUS_WL3 450
#elif TOT_MEM == 300
#define FCMPLUS_WL1 70400
#define FCMPLUS_WL2 4400
#define FCMPLUS_WL3 275
#elif TOT_MEM == 200
#define FCMPLUS_WL1 25600
#define FCMPLUS_WL2 1600
#define FCMPLUS_WL3 100
#elif TOT_MEM == 150
#define FCMPLUS_WL1 35072
#define FCMPLUS_WL2 2192
#define FCMPLUS_WL3 137
#elif TOT_MEM == 100
#define FCMPLUS_WL1 23296
#define FCMPLUS_WL2 1456
#define FCMPLUS_WL3 91
#elif TOT_MEM == 50
#define FCMPLUS_WL1 11776
#define FCMPLUS_WL2 736
#define FCMPLUS_WL3 46
#endif

typedef uint8_t FCMPLUS_C1; // 8-bit
#define FCMPLUS_THL1 254
typedef uint16_t FCMPLUS_C2; // 16-bit
#define FCMPLUS_THL2 65534
typedef uint32_t FCMPLUS_C3; // 32-bit
#define FCMPLUS_EM_ITER 15   // Num.iteration of EM-Algorithm. You can control.
/********************************************************************/

#define ELASTIC_BUCKET 3072
#define ELASTIC_HEAVY_STAGE 4
#define ELASTIC_WL TOT_MEM * 1024 - ELASTIC_BUCKET * ELASTIC_HEAVY_STAGE * 12
#define ELASTIC_TOFINO 0 
#define JUDGE_IF_SWAP_ELASTIC_P4(min_val, guard_val) ((guard_val >> 5) >= min_val)
#define ELASTIC_EM_ITER 15 // Num.iteration of EM-Algorithm. You can control.
struct Bucket
{
  uint32_t key;
  uint32_t val;
  uint32_t guard_val;
};
/********************************************************************/

// Count-Min
#define CM_BYTES TOT_MEM * 1024 
#define CM_DEPTH 3       // depth of CM
/********************************************************************/

// MRAC
#define MRAC_BYTES TOT_MEM * 1024 
#define MRAC_EM_ITER 15   // Num.iteration of EM-Algorithm. You can control.
/********************************************************************/

// HYPERLOGLOG
#define HLL_B 20       
#define HLL_REG_SIZE 8 // 8-bit register size
/********************************************************************/

// CUSKETCH (Count-Min + Conservative Update scheme)
#define CU_BYTES TOT_MEM * 1024 
#define CU_DEPTH 3       // depth of CU
/********************************************************************/

// PyramidSketch + Count-Min (PCMSketch)
#define MAX_HASH_NUM 20
#define LOW_HASH_NUM 4 // depth of PCM
typedef long long lint;
typedef unsigned int uint;
#define PCM_BYTES 1572864 // 1.5 * 1024 * 1024 = 1.5MB
/********************************************************************/

// UnivMon
#define UNIV_BYTES TOT_MEM * 1024 // 1.5 * 1024 * 1024 = 1.5MB
#define UNIV_BYTES_HC TOT_MEM * 1024
#define UNIV_LEVEL 14
#define UNIV_K 1000
#define UNIV_ROW 5
//#define UNIV_BYTES 1048576

/********************************************************************/

// Sieving
#define SIEVING_MEM TOT_MEM * 1024

/********************************************************************/
#endif
