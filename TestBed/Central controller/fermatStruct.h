#ifndef __FERMATPACKET_H__
#define __FERMATPACKET_H__

#include "params.h"
#include <stdint.h>

struct __attribute__((__packed__)) EtherHdr {
  uint8_t ethdstAddr[6];
  uint8_t ethsrcAddr[6];
  uint16_t ethtype;
};

struct __attribute__((__packed__)) CtrlMsg {
  uint16_t small_fermat_size;
  uint16_t median_fermat_size;
  uint16_t large_fermat_size;
  uint16_t small_fermat_base_offset;
  uint16_t median_fermat_base_offset;
  uint16_t large_fermat_base_offset;
  uint16_t small_fermat_mask;
  uint16_t median_fermat_mask;
  uint16_t large_fermat_mask;
  uint16_t sampling_rate;
  uint8_t padding[44];
};

struct __attribute__((__packed__)) FermatBucket {
  uint32_t ID_1;
  uint32_t ID_2;
  uint32_t ID_3;
  uint32_t ID_4;
  uint32_t counter;
};

struct __attribute__((__packed__)) FilterOne_t {
  uint16_t index;
  uint8_t slice[filter1_carry_num];
};

struct __attribute__((__packed__)) FilterTwo_t {
  uint16_t index;
  uint16_t slice[filter2_carry_num];
};

struct __attribute__((__packed__)) Fermat_t {
  uint16_t index;
  struct FermatBucket buckets[carry_idx_num * array_num];
};

struct FermatSketch {
  uint32_t ID_1;
  uint32_t ID_2;
  uint32_t ID_3;
  uint32_t ID_4;
  uint32_t counter;
};

struct FilterOne {
  uint8_t counter[filter1_len];
};

struct FilterTwo {
  uint16_t counter[filter2_len];
};

struct Flow {
  uint32_t counter;
  uint32_t ipsrc;
  uint32_t ipdst;
  uint16_t src_port;
  uint16_t dst_port;
  uint8_t protocol;

  bool operator<(const Flow& f) const {
    if (ipsrc != f.ipsrc) return (ipsrc < f.ipsrc);
    if (ipdst != f.ipdst) return (ipdst < f.ipdst);
    if (src_port != f.src_port) return (src_port < f.src_port);
    if (dst_port != f.dst_port) return (dst_port < f.dst_port);
    return (protocol < f.protocol);
  }
};

struct decodeResult {
  int flow_num;
  int fake_num;
  struct Flow f[ingress_entry_num * array_num * array_num];
};

struct __attribute__((__packed__)) AdjustPkt {
  uint8_t ethdstAddr[6];
  uint8_t ethsrcAddr[6];
  uint16_t ethtype;
  uint8_t switch_id;
  uint16_t zone_base[3];
  uint16_t zone_size[3];
  uint16_t mask[3];
  uint16_t sample_rate;
  uint8_t whether_range;
  uint16_t thresh[2];
  uint8_t padding[24];
};


#endif
