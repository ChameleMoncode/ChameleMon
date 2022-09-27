#ifndef __FERMATDECODE_H__
#define __FERMATDECODE_H__

#include "crc32/crc32.h"
#include "fermatStruct.h"
#include "mod.h"
#include "params.h"
#include <arpa/inet.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/********************************************************************/
/************** implementation of queue in C ************************/
/********************************************************************/

struct MyQueue {
  int front, rear, size;
  unsigned int capacity;
  uint32_t *array;
};

struct MyQueue *createNQueue(unsigned int _capacity, int queue_num) {
  struct MyQueue *q =
      (struct MyQueue *)malloc(queue_num * sizeof(struct MyQueue));
  for (int i = 0; i < queue_num; ++i) {
    q[i].capacity = _capacity;
    q[i].front = q[i].size = 0;
    q[i].rear = _capacity - 1;
    q[i].array = (uint32_t *)malloc(_capacity * sizeof(uint32_t));
  }
  return q;
}

bool isFull(struct MyQueue *q) { return (q->size == q->capacity); }

bool isEmpty(struct MyQueue *q) { return (q->size == 0); }

void enqueue(struct MyQueue *q, uint32_t item) {
  if (isFull(q)) {
    printf("enqueue full queue\n");
    return;
  }
  q->rear = (q->rear + 1) % q->capacity;
  q->array[q->rear] = item;
  ++q->size;
}

int dequeue(struct MyQueue *q) {
  if (isEmpty(q)) {
    printf("dequeue empty queue\n");
    return -1;
  }
  int item = q->array[q->front];
  q->front = (q->front + 1) % q->capacity;
  --q->size;
  return item;
}

void clearnqueue(struct MyQueue *q, int num) {
  for (int i = 0; i < num; ++i)
    free(q[i].array);
  free(q);
}

/********************************************************************/
/********************************************************************/

uint32_t real_hash(uint32_t poly, uint8_t *data, size_t cnt, uint32_t mask) {
  return (crc32_hash(poly, data, cnt) & mask);
}

void combine(uint32_t srcip, uint32_t dstip, uint32_t sdport, uint32_t rest,
             uint8_t *result) {
  uint32_t srcip1 = htonl(srcip);
  uint32_t dstip1 = htonl(dstip);
  uint8_t *sipptr = (uint8_t *)&srcip1;
  uint8_t *dipptr = (uint8_t *)&dstip1;

  uint32_t sdport1 = htonl(sdport);
  uint8_t *llptr = (uint8_t *)&sdport1;

  uint16_t rest1 = htons((uint16_t)rest);
  uint8_t *restptr = (uint8_t *)&rest1;

  memcpy(result, sipptr, 4);
  memcpy(result + 4, dipptr, 4);
  memcpy(result + 8, llptr, 4);
  memcpy(result + 12, restptr, 2);
}

void fermat_add(struct FermatSketch a[][ingress_entry_num], struct FermatSketch b[][ingress_entry_num]) {
  for (int i = 0; i < array_num; ++i)
    for (int j = 0; j < ingress_entry_num; ++j) {
      a[i][j].ID_1 = ((uint64_t)a[i][j].ID_1 + b[i][j].ID_1) % PRIME;	
      a[i][j].ID_2 = ((uint64_t)a[i][j].ID_2 + b[i][j].ID_2) % PRIME;	
      a[i][j].ID_3 = ((uint64_t)a[i][j].ID_3 + b[i][j].ID_3) % PRIME;	
      a[i][j].ID_4 = ((uint64_t)a[i][j].ID_4 + b[i][j].ID_4) % PRIME;	
      a[i][j].counter += b[i][j].counter;
    }
}

void add_flows(struct FermatSketch a[][ingress_entry_num], struct decodeResult *flows, 
   uint32_t *poly, uint32_t poly_fp, unsigned base_offset, unsigned sketchWidth, uint32_t mask) {
  for (int i = 0; i < flows->flow_num; ++i) {
    uint32_t id1 = flows->f[i].ipsrc;
    uint32_t id2 = flows->f[i].ipdst;
    uint32_t id3 = ((uint32_t)flows->f[i].src_port << 16) | (uint32_t)flows->f[i].dst_port;
    uint32_t id4 = (uint32_t) flows->f[i].protocol | ((id1 >> 31) << 10) | ((id2 >> 31) << 9) | ((id3 >> 31) << 8);
    id1 &= 0x7fffffff;
    id2 &= 0x7fffffff;
    id3 &= 0x7fffffff;
    uint8_t data[14];
    combine(id1, id2, id3, id4, data);
    id4 |= (real_hash(poly_fp, data, 14, (1 << 20) - 1) << 11);

    for (int j = 0; j < array_num; ++j) {
      uint32_t pos = base_offset + (real_hash(poly[j], data, 14, mask) % sketchWidth);
      a[j][pos].ID_1 = ((uint64_t)a[j][pos].ID_1 + (uint64_t)id1 * flows->f[i].counter % PRIME) % PRIME;
      a[j][pos].ID_2 = ((uint64_t)a[j][pos].ID_2 + (uint64_t)id2 * flows->f[i].counter % PRIME) % PRIME;
      a[j][pos].ID_3 = ((uint64_t)a[j][pos].ID_3 + (uint64_t)id3 * flows->f[i].counter % PRIME) % PRIME;
      a[j][pos].ID_4 = ((uint64_t)a[j][pos].ID_4 + (uint64_t)id4 * flows->f[i].counter % PRIME) % PRIME;
      a[j][pos].counter += flows->f[i].counter;
    }
  }
}

struct FermatSketch (
    *sub(struct FermatSketch a[][ingress_entry_num],
         struct FermatSketch b[][ingress_entry_num], unsigned egress_width))[ingress_entry_num] {
  struct FermatSketch(*f)[ingress_entry_num] =
      (struct FermatSketch(*)[ingress_entry_num])malloc(
          sizeof(struct FermatSketch) * array_num * ingress_entry_num);
  memset(f, 0, sizeof(struct FermatSketch) * array_num * ingress_entry_num);

  assert(egress_width <= ingress_entry_num);
  for (int i = 0; i < array_num; ++i) {
    for (int j = 0; j < egress_width; ++j) {
      f[i][j].ID_1 = ((uint64_t)a[i][j].ID_1 + PRIME - b[i][j].ID_1) % PRIME;
      f[i][j].ID_2 = ((uint64_t)a[i][j].ID_2 + PRIME - b[i][j].ID_2) % PRIME;
      f[i][j].ID_3 = ((uint64_t)a[i][j].ID_3 + PRIME - b[i][j].ID_3) % PRIME;
      f[i][j].ID_4 = ((uint64_t)a[i][j].ID_4 + PRIME - b[i][j].ID_4) % PRIME;
      f[i][j].counter =
          a[i][j].counter - b[i][j].counter; // counter is unsighed int
    }
  }
  return f;
}

void extract(uint32_t id1, uint32_t id2, uint32_t id3, uint32_t id4,
             struct Flow *f) {
  
  // restore the original 5-tuple
  uint32_t sip = (id4 & 0x400) >> 10;
  uint32_t dip = (id4 & 0x200) >> 9;
  uint32_t ll = (id4 & 0x100) >> 8;
  uint8_t proto = (id4 & 0xff);
  ll = (ll << 31) | id3;

  f->ipsrc = (sip << 31) | id1;
  f->ipdst = (dip << 31) | id2;
  f->src_port = (uint16_t)(ll >> 16);
  f->dst_port = (uint16_t)ll;
  f->protocol = proto;
}

bool verify(struct FermatSketch f[][ingress_entry_num], uint32_t *poly,
            uint32_t poly_fp, int row, int col, unsigned base_offset,
            unsigned sketchWidth, uint32_t mask, struct MyQueue *q, bool first_round,
            struct decodeResult *res) {
  uint64_t counter_res;
  uint32_t id1, id2, id3, id4;
  uint32_t fp;
  if (f[row][col].counter & 0x80000000) { // negative
    counter_res = powMod32(~f[row][col].counter + 1, PRIME - 2, PRIME);
    id1 = mulMod32(PRIME - f[row][col].ID_1, counter_res, PRIME);
    id2 = mulMod32(PRIME - f[row][col].ID_2, counter_res, PRIME);
    id3 = mulMod32(PRIME - f[row][col].ID_3, counter_res, PRIME);
    id4 = mulMod32(PRIME - f[row][col].ID_4, counter_res, PRIME);
    fp = id4 >> 11;
    id4 = id4 & 0x7ff;
  } else { // positive
    counter_res = powMod32(f[row][col].counter, PRIME - 2, PRIME);
    id1 = mulMod32(f[row][col].ID_1, counter_res, PRIME);
    id2 = mulMod32(f[row][col].ID_2, counter_res, PRIME);
    id3 = mulMod32(f[row][col].ID_3, counter_res, PRIME);
    id4 = mulMod32(f[row][col].ID_4, counter_res, PRIME);
    fp = id4 >> 11;
    id4 = id4 & 0x7ff;
  }

  // rehash verification
  uint8_t data[14];
  combine(id1, id2, id3, id4, data);

  uint32_t hash_v =
      real_hash(poly[row], data, 14, mask) % sketchWidth + base_offset;
  if (hash_v != col) {
    return false;
  }

  uint32_t hash_fp = real_hash(poly_fp, data, 14, ((1 << 20) - 1));
  if (hash_fp != fp) {
    return false;
  }

  for (int k = 0; k < array_num; ++k) {
    if (k == row)
      continue;
    hash_v = real_hash(poly[k], data, 14, mask) % sketchWidth + base_offset;
    f[k][hash_v].ID_1 =
        ((uint64_t)f[k][hash_v].ID_1 + PRIME - f[row][col].ID_1) % PRIME;
    f[k][hash_v].ID_2 =
        ((uint64_t)f[k][hash_v].ID_2 + PRIME - f[row][col].ID_2) % PRIME;
    f[k][hash_v].ID_3 =
        ((uint64_t)f[k][hash_v].ID_3 + PRIME - f[row][col].ID_3) % PRIME;
    f[k][hash_v].ID_4 =
        ((uint64_t)f[k][hash_v].ID_4 + PRIME - f[row][col].ID_4) % PRIME;
    f[k][hash_v].counter = f[k][hash_v].counter - f[row][col].counter;

    if (first_round) {
      if (k < row)
        enqueue(q + k, hash_v);
    } else
      enqueue(q + k, hash_v);
  }

  res->f[res->flow_num].counter = f[row][col].counter;
  extract(id1, id2, id3, id4, &res->f[res->flow_num]);
  ++res->flow_num;
  memset(&f[row][col], 0, sizeof(struct FermatSketch));

  return true;
}

bool is_same_flow(struct Flow *f1, struct Flow *f2) {
  return ((f1->ipsrc == f2->ipsrc) && (f1->ipdst == f2->ipdst) &&
          (f1->src_port == f2->src_port) && (f1->dst_port == f2->dst_port) &&
          (f1->protocol == f2->protocol));
}

void merge_flows(struct decodeResult *res) {
  res->fake_num = 0;
  for (int i = 0; i < res->flow_num; ++i) {
    if ((res->f[i].counter & 0x80000000) != 0) {
      for (int j = 0; j < res->flow_num; ++j) {
        if (j == i)
          continue;
        if (is_same_flow(&res->f[i], &res->f[j])) {
          res->f[i].counter += res->f[j].counter;
          res->f[j].counter = 0;
          ++res->fake_num;
        }
      }
      if (res->f[i].counter == 0) {
        ++res->fake_num;
      }
    }
  }
}

bool decode_succeed(struct FermatSketch x[][ingress_entry_num],
                    unsigned base_offset, unsigned sketchWidth) {
  for (int i = 0; i < array_num; ++i) {
    for (int j = 0; j < sketchWidth; ++j) {
      if (x[i][base_offset + j].counter != 0)
        return false;
    }
  }
  return true;
}

struct decodeResult *decode(struct FermatSketch x[][ingress_entry_num],
                            uint32_t *poly, uint32_t poly_fp,
                            unsigned base_offset, unsigned sketchWidth, uint32_t mask) {
  struct MyQueue *candidate = createNQueue(sketchWidth * array_num, array_num);
  uint8_t data[14];
  struct decodeResult *res =
      (struct decodeResult *)malloc(sizeof(struct decodeResult));
  res->flow_num = 0;

  for (int i = 0; i < array_num; ++i) {
    for (int j = 0; j < sketchWidth; ++j) {
      if (x[i][base_offset + j].counter == 0) {
        continue;
      }
      verify(x, poly, poly_fp, i, base_offset + j, base_offset, sketchWidth, mask, candidate, true,
             res);
    }
  }

  bool pause = false;
  while (!pause && (res->flow_num < ingress_entry_num * array_num * array_num)) {
    pause = true;
    for (int i = 0; i < array_num; ++i) {
      if (!isEmpty(candidate + i))
        pause = false;
      while (!isEmpty(candidate + i) && (res->flow_num < ingress_entry_num * array_num * array_num)) {
        int check_id = dequeue(candidate + i);
        if (x[i][check_id].counter == 0) {
          continue;
        }
        verify(x, poly, poly_fp, i, check_id, base_offset, sketchWidth, mask,
               candidate, false, res);
      }
    }
  }

  clearnqueue(candidate, array_num);

  if (!decode_succeed(x, base_offset, sketchWidth)) {
    printf("decode error\n");
    free(res);
    return NULL;
  }

  merge_flows(res);

  return res;
}

#endif
