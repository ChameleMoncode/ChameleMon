#include "fermatDecode.h"
#include "fermatStruct.h"
#include "memoryAdjuster.h"
#include "threshAdjuster.h"
#include <assert.h>
#include <rte_atomic.h>
#include <rte_cycles.h>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_ether.h>
#include <rte_lcore.h>
#include <rte_mbuf.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include <set>


#define NUM_MBUFS 8192
#define MBUF_CACHE_SIZE 250
#define RX_RING_SIZE 2048
#define TX_RING_SIZE 1024
#define BURST_SIZE 128
#define LOAD_FACTOR (0.7)
#define MIN_LOAD_FACTOR (0.6)

#define MIN_HH_THRESH 0
// #define HL_THRESH 568

FILE *logs;
struct rte_mempool *mbuf_pool;
// static const struct rte_eth_conf port_conf_default = {
//     .rxmode =
//         {
//             .max_rx_pkt_len = ETHER_MAX_LEN,
//         },
// };

inline void dpdk_init_port_conf(struct rte_eth_conf *conf) {
  conf->rxmode.split_hdr_size = 0;
  conf->rxmode.max_rx_pkt_len = ETHER_MAX_LEN;
}
uint32_t poly[array_num] = {0x04C11DB7, 0x741B8CD7, 0xDB710641};
uint32_t poly_fp = 0x82608EDB;

const unsigned port_id = 0;
unsigned time_flag = 0;
bool first_packet = false;
bool fermat_avail[dev_num];
bool filter1_avail[dev_num];
bool filter2_avail[dev_num];
struct FermatSketch old_ingress_sketch[2][dev_num][array_num][ingress_entry_num];
struct FermatSketch old_egress_sketch[2][dev_num][array_num][ingress_entry_num];
struct FermatSketch ingress_sketch[dev_num][array_num][ingress_entry_num];
struct FermatSketch egress_sketch[dev_num][array_num][ingress_entry_num];
struct FermatSketch cur_ingress_sketch[dev_num][array_num][ingress_entry_num];
struct FermatSketch cur_egress_sketch[dev_num][array_num][ingress_entry_num];
struct FermatSketch ingress_sum_sketch[array_num][ingress_entry_num];
struct FermatSketch egress_sum_sketch[array_num][ingress_entry_num];
struct FilterOne filter1[dev_num];
struct FilterTwo filter2[dev_num];

bool configure = false;
uint16_t zone_base[3], zone_size[3], mask[3];
uint16_t sample_rate = 65535;
uint16_t estimate_flow_size[3];
uint8_t state = 0;
uint16_t HH_THRESH = 10, HL_THRESH = 0;
uint32_t filter_2_poly = 0x11111111, filter_1_poly = 0x04C11DB7;
double HH_portion = 0;

uint64_t small_loss_count[ingress_entry_num * array_num * array_num];
FILE *fp;
char *filename; 
char *dist;

// src_mac_to_dev???

/*
 * Initializes a given port using global settings and with the RX buffers
 * coming from the mbuf_pool passed as a parameter.
 */
static inline int port_init(uint16_t port, struct rte_mempool *mbuf_pool) {
  struct rte_eth_conf port_conf;
  dpdk_init_port_conf(&port_conf);
  const uint16_t rx_rings = 1, tx_rings = 1;
  uint16_t nb_rxd = RX_RING_SIZE;
  uint16_t nb_txd = TX_RING_SIZE;
  int retval;
  uint16_t q;
  struct rte_eth_dev_info dev_info;
  struct rte_eth_txconf txconf;

  if (!rte_eth_dev_is_valid_port(port))
    return -1;

  rte_eth_dev_info_get(port, &dev_info);
  if (dev_info.tx_offload_capa & DEV_TX_OFFLOAD_MBUF_FAST_FREE)
    port_conf.txmode.offloads |= DEV_TX_OFFLOAD_MBUF_FAST_FREE;

  /* Configure the Ethernet device. */
  retval = rte_eth_dev_configure(port, rx_rings, tx_rings, &port_conf);
  if (retval != 0)
    return retval;

  retval = rte_eth_dev_adjust_nb_rx_tx_desc(port, &nb_rxd, &nb_txd);
  if (retval != 0)
    return retval;

  printf("nb_rxd=%d,nb_txd=%d\n", nb_rxd, nb_txd);
  /* Allocate and set up 1 RX queue per Ethernet port. */
  for (q = 0; q < rx_rings; q++) {
    retval = rte_eth_rx_queue_setup(
        port, q, nb_rxd, rte_eth_dev_socket_id(port), NULL, mbuf_pool);
    if (retval < 0)
      return retval;
  }

  txconf = dev_info.default_txconf;
  txconf.offloads = port_conf.txmode.offloads;
  /* Allocate and set up 1 TX queue per Ethernet port. */
  for (q = 0; q < tx_rings; q++) {
    retval = rte_eth_tx_queue_setup(port, q, nb_txd,
                                    rte_eth_dev_socket_id(port), &txconf);
    if (retval < 0)
      return retval;
  }

  /* Start the Ethernet port. */
  retval = rte_eth_dev_start(port);
  if (retval < 0)
    return retval;

  char port_name[256];
  rte_eth_dev_get_name_by_port(port, port_name);
  printf("port name=%s\n", port_name);
  /* Display the port MAC address. */
  struct ether_addr addr;
  rte_eth_macaddr_get(port, &addr);
  printf("Port %u MAC: %02" PRIx8 " %02" PRIx8 " %02" PRIx8 " %02" PRIx8
         " %02" PRIx8 " %02" PRIx8 "\n",
         port, addr.addr_bytes[0], addr.addr_bytes[1], addr.addr_bytes[2],
         addr.addr_bytes[3], addr.addr_bytes[4], addr.addr_bytes[5]);

  /* Enable RX in promiscuous mode for the Ethernet device. */
  rte_eth_promiscuous_enable(port);

  return 0;
}

static bool check_all_avail(void) {
  for (int i = 0; i < dev_num; ++i) {
    if (!fermat_avail[i] || !filter1_avail[i] || !filter2_avail[i]) {
      return false;
    }
  }
  return true;
}

static int get_dev_id(struct EtherHdr *hdr) { return hdr->ethsrcAddr[5]; }

void calculate_cur_sketch() {
  for (int i = 0; i < dev_num; ++i) {
    for (int j = 0; j < array_num; ++j) {
      for (int k = 0; k < ingress_entry_num; ++k) {
        cur_ingress_sketch[i][j][k].ID_1 = ((uint64_t)ingress_sketch[i][j][k].ID_1 + PRIME - old_ingress_sketch[time_flag][i][j][k].ID_1) % PRIME;
        cur_ingress_sketch[i][j][k].ID_2 = ((uint64_t)ingress_sketch[i][j][k].ID_2 + PRIME - old_ingress_sketch[time_flag][i][j][k].ID_2) % PRIME;
        cur_ingress_sketch[i][j][k].ID_3 = ((uint64_t)ingress_sketch[i][j][k].ID_3 + PRIME - old_ingress_sketch[time_flag][i][j][k].ID_3) % PRIME;
        cur_ingress_sketch[i][j][k].ID_4 = ((uint64_t)ingress_sketch[i][j][k].ID_4 + PRIME - old_ingress_sketch[time_flag][i][j][k].ID_4) % PRIME;
        cur_ingress_sketch[i][j][k].counter = ingress_sketch[i][j][k].counter - old_ingress_sketch[time_flag][i][j][k].counter;
	// if (cur_ingress_sketch[i][j][k].counter)
	// 	printf("cur ingress: %d %d %d %d\n", i, j, k, cur_ingress_sketch[i][j][k].counter);
      }
    }
  }

  for (int i = 0; i < dev_num; ++i) {
    for (int j = 0; j < array_num; ++j) {
      for (int k = 0; k < ingress_entry_num; ++k) {
        cur_egress_sketch[i][j][k].ID_1 = ((uint64_t)egress_sketch[i][j][k].ID_1 + PRIME - old_egress_sketch[time_flag][i][j][k].ID_1) % PRIME;
        cur_egress_sketch[i][j][k].ID_2 = ((uint64_t)egress_sketch[i][j][k].ID_2 + PRIME - old_egress_sketch[time_flag][i][j][k].ID_2) % PRIME;
        cur_egress_sketch[i][j][k].ID_3 = ((uint64_t)egress_sketch[i][j][k].ID_3 + PRIME - old_egress_sketch[time_flag][i][j][k].ID_3) % PRIME;
        cur_egress_sketch[i][j][k].ID_4 = ((uint64_t)egress_sketch[i][j][k].ID_4 + PRIME - old_egress_sketch[time_flag][i][j][k].ID_4) % PRIME;
        cur_egress_sketch[i][j][k].counter = egress_sketch[i][j][k].counter - old_egress_sketch[time_flag][i][j][k].counter;
	// if (cur_egress_sketch[i][j][k].counter)
	// 	printf("cur egress: %d %d %d %d\n", i, j, k, cur_egress_sketch[i][j][k].counter);
      }
    }
  }
}

void aggregate_sketch() {
  memset(ingress_sum_sketch, 0, sizeof(struct FermatSketch) * array_num * ingress_entry_num);
  memset(egress_sum_sketch, 0, sizeof(struct FermatSketch) * array_num * ingress_entry_num);
  for (int i = 0; i < dev_num; ++i)
	  fermat_add(ingress_sum_sketch, cur_ingress_sketch[i]);
  for (int i = 0; i < dev_num; ++i)
	  fermat_add(egress_sum_sketch, cur_egress_sketch[i]);
}

uint16_t calculate_mask(uint16_t size) {
  uint16_t m = 0x8000;
  if (size == 0)
    return 0;
  for (; (m & size) == 0; m >>= 1);
  assert(m <= 0x1000);
  m <<= 3;
  m = ((m - 1) << 1) + 1;
  return m;
}

uint64_t tower_query(struct Flow *f) {
  uint32_t dev_id = (f->ipsrc % 256) / 2;
  assert(dev_id < dev_num);

  uint32_t id1 = f->ipsrc;
  uint32_t id2 = f->ipdst;
  uint32_t id3 = ((uint32_t)f->src_port << 16) | (uint32_t)f->dst_port;
  uint32_t id4 = (uint32_t) f->protocol | ((id1 >> 31) << 10) | ((id2 >> 31) << 9) | ((id3 >> 31) << 8);
  id1 &= 0x7fffffff;
  id2 &= 0x7fffffff;
  id3 &= 0x7fffffff;
  uint8_t data[14];
  combine(id1, id2, id3, id4, data);
  uint32_t index_2 = real_hash(filter_2_poly, data, 14, 16383);
  uint32_t index_1 = real_hash(filter_1_poly, data, 14, 32767);
  uint64_t result_1 = (uint64_t) filter1[dev_id].counter[index_1] < 0xff ? filter1[dev_id].counter[index_1] : 0xffffffff;
  uint64_t result_2 = (uint64_t) filter2[dev_id].counter[index_2] < 0xffff ? filter2[dev_id].counter[index_2] : 0xffffffff;
  // printf("tower_query: %d %d %d %d\n", index_1, index_2, result_1, result_2);
  return result_1 < result_2 ? result_1 : result_2;
}

void get_small_loss_counter(struct decodeResult *small_loss) {
  for (int i = 0; i < small_loss->flow_num; ++i) {
    small_loss_count[i] = tower_query(&small_loss->f[i]);
  }
}

void test_query(struct decodeResult *hh) {
  for (int i = 0; i < hh->flow_num && i < 10; ++i) {
    printf("%d-th hh: %d\n", i, tower_query(&hh->f[i]));
  }
}

void send_adjust_pkts(uint8_t whether_range, uint16_t thresh0, uint16_t thresh1) {
  struct rte_mbuf *bufs[dev_num];
  int retval;
  while ((retval = rte_pktmbuf_alloc_bulk(mbuf_pool, bufs, dev_num)) != 0) {
    usleep(10);
  }

  fprintf(fp, "send_adjust_pkts: %d %d %d %d %d %d %d\n", zone_size[0], zone_size[1], zone_size[2], sample_rate, whether_range, thresh0, thresh1);

  zone_base[0] = 0, zone_base[1] = zone_size[0], zone_base[2] = zone_base[1] + zone_size[1];

  mask[0] = calculate_mask(zone_size[0]);
  mask[1] = calculate_mask(zone_size[1]);
  mask[2] = calculate_mask(zone_size[2]);
  fprintf(fp, "%d %d %d\n", mask[0], mask[1], mask[2]);

  HH_portion = get_portion(HH_THRESH);

  for (uint8_t i = 0; i < dev_num; ++i) {
    struct AdjustPkt *hdr = (struct AdjustPkt *)rte_pktmbuf_append(bufs[i], sizeof(struct AdjustPkt));

    hdr->ethtype = htons(0x88b5);
    hdr->switch_id = i + 1;
    for (int j = 0; j < 3; ++j) {
      hdr->zone_size[j] = (zone_size[j]);
      hdr->zone_base[j] = (zone_base[j]);
      hdr->mask[j] = (mask[j]);

      hdr->whether_range = whether_range;
      hdr->thresh[0] = thresh0;
      hdr->thresh[1] = thresh1;
    }

    hdr->sample_rate = (sample_rate); // ???
  }
  const uint16_t nb_tx = rte_eth_tx_burst(port_id, 0, bufs, dev_num);
  assert(nb_tx == dev_num);
  configure = true;
}

void adjust_sampling (uint16_t small_flow_size, double ratio) {
  double rate = (double)zone_size[0] * ratio / (small_flow_size * 65536.0 / (1 + (double)sample_rate));
  rate = rate > 1.0 ? 1.0 : rate;
  unsigned conf = 65536 * rate;
  sample_rate = conf != 0 ? conf - 1 : conf;
}

// void start_heavy_loss() {
//   state = 1;
//   zone_size[0] = 2048;
//   zone_size[1] = 1024;
//   zone_size[2] = 1024;
// 
//   sample_rate = 65535;
// 
//   send_adjust_pkts(1, 40, 400); // thresh???
// }

uint16_t get_flow_size_2() {
  uint16_t max_HH_num = 0;
  for (uint8_t i = 0; i < dev_num; ++i) {
    int flow_num = estimateFlowNum(cur_ingress_sketch[i], zone_base[2], zone_size[2]);
    max_HH_num = max_HH_num > flow_num ? max_HH_num : flow_num;
  }
  return max_HH_num;
}

uint16_t get_flow_size_1 (struct FermatSketch (*loss)[ingress_entry_num]) {
  return estimateFlowNum(loss, zone_base[1], zone_size[1]);
}

uint16_t get_flow_size_0 (struct FermatSketch (*loss)[ingress_entry_num]) {
  return estimateFlowNum(loss, zone_base[0], zone_size[0]);
}

void ensureHH(double ratio) {
  double portion = (double)zone_size[2] * ratio / (estimate_flow_size[2] / (HH_portion));

  HH_THRESH = get_HH_thresh(portion);
  if (HH_THRESH < MIN_HH_THRESH)
	  HH_THRESH = MIN_HH_THRESH;

  send_adjust_pkts(1, HL_THRESH, HH_THRESH);
}

void ensureHL_0(double ratio, double min_ratio) {
  if (estimate_flow_size[1] <= 3072 * ratio) {
    zone_size[1] = estimate_flow_size[1] / ratio;
    zone_size[2] = ingress_entry_num - zone_size[1];
    zone_size[0] = 0;

    double portion = (double)zone_size[2] * ratio / (estimate_flow_size[2] / (HH_portion));
    // printf("portion: %lf, HH_portion: %lf\n", portion, HH_portion);
    HH_THRESH = get_HH_thresh(portion);
    if (HH_THRESH < MIN_HH_THRESH)
	    HH_THRESH = MIN_HH_THRESH;

    send_adjust_pkts(1, HL_THRESH, HH_THRESH);
  } else {
    state = 1;
    zone_size[2] = 1024, zone_size[1] = 2560, zone_size[0] = 512;
    sample_rate = 65535;
    double portion = (double)zone_size[2] * ratio / (estimate_flow_size[2] / (HH_portion));
    HH_THRESH = get_HH_thresh(portion);
    HL_THRESH = HH_THRESH;

    adjust_sampling(estimate_flow_size[1], ratio);
    send_adjust_pkts(1, HL_THRESH, HH_THRESH);
  }
}

void AdjustMem_0 (double ratio, double min_ratio) {
  if (estimate_flow_size[2] != 0 && estimate_flow_size[1] < zone_size[1] * min_ratio)
	  zone_size[1] = estimate_flow_size[1] / ratio > 512 ? estimate_flow_size[1] / ratio: 512;
  zone_size[2] = ingress_entry_num - zone_size[1];
  double portion = (double)zone_size[2] * ratio/ (estimate_flow_size[2] / (HH_portion));
  HH_THRESH = get_HH_thresh(portion);
  if (HH_THRESH < MIN_HH_THRESH) 
	  HH_THRESH = MIN_HH_THRESH;

  send_adjust_pkts(1, HL_THRESH, HH_THRESH);
}

void ensureLL_1(double ratio) {
  adjust_sampling(estimate_flow_size[0], ratio);
  send_adjust_pkts(0, HL_THRESH, HH_THRESH);
}

void ensureHL_1(struct decodeResult *small_loss, double ratio) {
  // HL_THRESH = HH_THRESH;
  // send_adjust_pkts(1, HL_THRESH, HH_THRESH);

  get_small_loss_counter(small_loss);
  int HL_cnt = 0;
  for (int i = 0; i < small_loss->flow_num; ++i)
    if (small_loss_count[i] > HL_THRESH)
      ++HL_cnt;

  int HL_l = HL_THRESH, HL_r = HH_THRESH;
  while (HL_r - HL_l >= 2) {
    int hl = (HL_l + HL_r) / 2;
    int cnt = 0; 
    for (int i = 0; i < small_loss->flow_num; ++i)
      if (small_loss_count[i] > hl) 
        ++cnt;
    // cnt *= (65536.0 / ((double)sample_rate + 1));
    // printf("ensureHL_1: %d %d\n", hl, cnt);
    if ((double)cnt * estimate_flow_size[1] / HL_cnt > zone_size[1] * ratio)
      HL_l = hl;
    else
      HL_r = hl;
  }
  
  HL_THRESH = HL_r;
  // adjust_sampling(small_loss->flow_num, ratio);
  send_adjust_pkts(1, HL_THRESH, HH_THRESH);
}

void AdjustMem_1(struct decodeResult *median_loss, struct decodeResult *small_loss, double ratio, double min_ratio) {
  std::set<Flow> t;
  for (int i = 0; i < median_loss->flow_num; ++i)
	  t.insert(median_loss->f[i]);
  int loss_sum = 0;
  for (int i = 0; i < small_loss->flow_num; ++i)
	  if (t.count(small_loss->f[i]) == 0)
		  ++loss_sum;
  loss_sum = loss_sum * 65536.0 / (1 + (double)sample_rate) + median_loss->flow_num;
  if (loss_sum < 3072 * ratio) {
    state = 0;
    sample_rate = 65535;
    HL_THRESH = 0;
    printf("loss num: %d\n", loss_sum);

    zone_size[0] = 0;
    zone_size[1] = loss_sum / ratio;
    zone_size[2] = ingress_entry_num - zone_size[1];
    double portion = (double)zone_size[2] * ratio / (estimate_flow_size[2] / (HH_portion));
    HH_THRESH = get_HH_thresh(portion);
    if (HH_THRESH < MIN_HH_THRESH) 
       HH_THRESH = MIN_HH_THRESH;
   send_adjust_pkts(1, HL_THRESH, HH_THRESH);
    return;
  }
  
 if (estimate_flow_size[2] != 0 && estimate_flow_size[2] < zone_size[2] * min_ratio) {
    double portion = (double)zone_size[2] * ratio / (estimate_flow_size[2] / (HH_portion));
    HH_THRESH = get_HH_thresh(portion);
    if (HH_THRESH < MIN_HH_THRESH)
            HH_THRESH = MIN_HH_THRESH;
    HL_THRESH = HH_THRESH;
    send_adjust_pkts(1, HL_THRESH, HH_THRESH);
    return;
  }

    // test_query(median_loss);
  if (estimate_flow_size[1] < zone_size[1] * min_ratio) {
    get_small_loss_counter(small_loss);
    int HL_l = 0, HL_r = HL_THRESH;
    while (HL_r - HL_l >= 2) {
      int hl = (HL_l + HL_r) / 2;
      int cnt = 0; 
      for (int i = 0; i < small_loss->flow_num; ++i)
        if (small_loss_count[i] > hl && small_loss_count[i] < HL_THRESH)
          ++cnt;
      cnt *= (65536.0 / ((double)sample_rate + 1));
      cnt += (estimate_flow_size[1]);
      if (cnt > zone_size[1] * ratio)
        HL_l = hl;
      else
        HL_r = hl;
    }
  
    // int cnt1 = 0, cnt2 = 0; 
    // for (int i = 0; i < small_loss->flow_num; ++i)
    //   if (small_loss_count[i] > HL_r && small_loss_count[i] < HL_THRESH)
    //     ++cnt1;
    //   else if (small_loss_count[i] >= HL_THRESH)
    //     ++cnt2;
    // printf("%d %d %d\n", HL_r, cnt1, cnt2);
  
    HL_THRESH = HL_r;
  
    // int small_cnt = 0;
    // for (int i = 0; i < small_loss->flow_num; ++i) {
    //   if (small_loss_count[i] <= HL_THRESH)
    //     ++small_cnt;
    // }
    // adjust_sampling(small_cnt, ratio);
    adjust_sampling(small_loss->flow_num, ratio);
    send_adjust_pkts(1, HL_THRESH, HH_THRESH);
    return;
  }

  if (estimate_flow_size[0] < zone_size[0] * min_ratio) {
    adjust_sampling(estimate_flow_size[0], ratio);
    send_adjust_pkts(0, HL_THRESH, HH_THRESH);
  }
}


/**
 * The lcore main. This is the main thread that
 */
static void lcore_main(void) {
  struct rte_mbuf *m1[BURST_SIZE];
  struct EtherHdr *eth_hdr;
  struct Fermat_t *sketch_t;
  struct FilterOne_t *filter1_t;
  struct FilterTwo_t *filter2_t;
  struct CtrlMsg *ctrlmsg_t;

  struct FermatSketch tmp;
  uint16_t nb_rx;
  int i;
  uint32_t cur = 0;
  int dev_id;

  printf("\nCore %u receiving packets. [Ctrl+C to quit]\n", rte_lcore_id());

  read_dist(dist);
  fp = fopen(filename, "w");

  zone_size[0] = 0;
  zone_size[1] = 512;
  zone_size[2] = 3584;
  HL_THRESH = 0, HH_THRESH = 0;
  send_adjust_pkts(1, HL_THRESH, HH_THRESH);

  while (1) {
    nb_rx = rte_eth_rx_burst(port_id, 0, &m1[0], BURST_SIZE);
    if (nb_rx > 0) {
      cur = cur + nb_rx;
    }
    for (i = 0; i < nb_rx; i++) {
      eth_hdr = rte_pktmbuf_mtod_offset(m1[i], struct EtherHdr *, 0);
      dev_id = get_dev_id(eth_hdr) - 1;

      if (eth_hdr->ethtype == ntohs(0x3334)) { // fermat
        sketch_t = rte_pktmbuf_mtod_offset(m1[i], struct Fermat_t *,
                                           sizeof(struct EtherHdr));
        uint16_t index = ntohs(sketch_t->index);
        index = index % ingress_entry_num;
	
        if (!first_packet) {
          first_packet = true;
          struct timeval tv;
          gettimeofday(&tv, NULL);
          fprintf(fp, "timestamp of first packet for fermat: %d %d\n", tv.tv_sec,
                tv.tv_usec);
        }

        int slice_idx = 0;
        for (int i = index; i > index - carry_idx_num; --i) {
          if (i < egress_entry_num) {
            for (int j = 0; j < array_num; ++j) {
              memmove(&tmp, &sketch_t->buckets[slice_idx++],
                      sizeof(struct FermatSketch));
              tmp.ID_1 = ntohl(tmp.ID_1);
              tmp.ID_2 = ntohl(tmp.ID_2);
              tmp.ID_3 = ntohl(tmp.ID_3);
              tmp.ID_4 = ntohl(tmp.ID_4);
              tmp.counter = ntohl(tmp.counter);

	      // if (tmp.counter != 0) {
	      //         printf("egress sketch: %d %d %d %d\n", dev_id, j, i, tmp.counter);
	      // }

              memmove(&egress_sketch[dev_id][j][i], &tmp,
                      sizeof(struct FermatSketch));
            }
          }

          for (int j = 0; j < array_num; ++j) {
            assert(i >= 0);
            memmove(&tmp, &sketch_t->buckets[slice_idx++],
                    sizeof(struct FermatSketch));
            tmp.ID_1 = ntohl(tmp.ID_1);
            tmp.ID_2 = ntohl(tmp.ID_2);
            tmp.ID_3 = ntohl(tmp.ID_3);
            tmp.ID_4 = ntohl(tmp.ID_4);
            tmp.counter = ntohl(tmp.counter);
	    
	    // if (tmp.counter != 0) {
	    //         printf("ingress sketch: %d %d %d %d\n", dev_id, j, i, tmp.counter);
            // }

            memmove(&ingress_sketch[dev_id][j][i], &tmp,
                    sizeof(struct FermatSketch));
          }

        }

        // if the pkt is the last one, set available
        if (index == ingress_entry_num - 1) {
        //   struct timeval tv;
        //   gettimeofday(&tv, NULL);
        //   printf("timestamp of last packet for fermat: %d %d\n", tv.tv_sec,
        //          tv.tv_usec);
           fermat_avail[dev_id] = true;
        }

      } else if (eth_hdr->ethtype == ntohs(0x2221)) { // filter 1
        filter1_t = rte_pktmbuf_mtod_offset(m1[i], struct FilterOne_t *,
                                            sizeof(struct EtherHdr));
        uint16_t index = ntohs(filter1_t->index);
        index = index % filter1_len;

        for (int i = index, slice_idx = 0; i > index - filter1_carry_num;
             --i, ++slice_idx) {
          filter1[dev_id].counter[i] = filter1_t->slice[slice_idx];
        }

        if (index == filter1_len - 1) {
         //  struct timeval tv;
         //  gettimeofday(&tv, NULL);
         //  printf("timestamp of last packet for filter1: %d %d\n", tv.tv_sec,
         //        tv.tv_usec);
          filter1_avail[dev_id] = true;
        }
      } else if (eth_hdr->ethtype == ntohs(0x2222)) { // filter 2
        filter2_t = rte_pktmbuf_mtod_offset(m1[i], struct FilterTwo_t *,
                                            sizeof(struct EtherHdr));
        uint16_t index = ntohs(filter2_t->index);
        index = index % filter2_len;

        for (int i = index, slice_idx = 0; i > index - filter2_carry_num;
             --i, ++slice_idx) {
          filter2[dev_id].counter[i] = ntohs(filter2_t->slice[slice_idx]);
	//  if (filter2[dev_id].counter[i] != 0) 
	//	  printf("%d %d %d\n", dev_id, i, filter2[dev_id].counter[i]);
        }

        if (index == filter2_len - 1) {
          // struct timeval tv;
          // gettimeofday(&tv, NULL);
          // printf("timestamp of last packet for filter2: %d %d\n", tv.tv_sec,
          //        tv.tv_usec);
          filter2_avail[dev_id] = true;
        }
      } 
      rte_pktmbuf_free(m1[i]);
    }

    if (check_all_avail()) {
        fprintf(fp, "start processing (sample rate=%d, size[0-2] = %d %d %d, HH = %d, HL = %d)...\n", sample_rate, zone_size[0], zone_size[1], zone_size[2], HH_THRESH, HL_THRESH);
        bool HH_error = false, median_loss_error = false, small_loss_error = false;
        struct timeval tv;
        gettimeofday(&tv, NULL);
        fprintf(fp, "timestamp of decode fermat: %d %d\n", tv.tv_sec,
              tv.tv_usec);

        calculate_cur_sketch();

	estimate_flow_size[2] = get_flow_size_2();

	uint16_t max_HH_num = 0;
        for (int i = 0; i < dev_num; ++i) {
          struct decodeResult *HH =
              decode(cur_ingress_sketch[i], poly, poly_fp, zone_base[2], zone_size[2], mask[2]);

          if (HH != NULL) {
            fprintf(fp, "%d dev has %d HH\n", i, HH->flow_num);
	    max_HH_num = HH->flow_num > max_HH_num ? HH->flow_num : max_HH_num;
            add_flows(cur_ingress_sketch[i], HH, poly, poly_fp, zone_base[1], zone_size[1], mask[1]);
	    // test_query(HH);
          } else {
	          HH_error = true;
		  fprintf(fp, "%d dev HH decode error!\n", i);
		      }
          free(HH);
        }

	if (!HH_error) {
	  estimate_flow_size[2] = max_HH_num;
	 }

        aggregate_sketch();

        struct FermatSketch (*loss)[ingress_entry_num] = sub(ingress_sum_sketch, egress_sum_sketch, 3072);
        estimate_flow_size[1] = get_flow_size_1(loss);
	estimate_flow_size[0] = get_flow_size_0(loss);

        struct decodeResult *small_loss = decode(loss, poly, poly_fp, zone_base[0], zone_size[0], mask[0]);
        struct decodeResult *median_loss = decode(loss, poly, poly_fp, zone_base[1], zone_size[1], mask[1]);

        if (small_loss != NULL) {
          fprintf(fp, "small loss: %d\n", small_loss->flow_num);
	  estimate_flow_size[0] = small_loss->flow_num;
        } else {
          small_loss_error = true;
	  fprintf(fp, "small loss decode error!\n");
	      }

        if (median_loss != NULL) {
          fprintf(fp, "median loss: %d\n", median_loss->flow_num);
	  estimate_flow_size[1] = median_loss->flow_num;
        } else {
          median_loss_error = true;
	  fprintf(fp, "median loss decode error!\n");
        }

        gettimeofday(&tv, NULL);
        fprintf(fp, "timestamp of decode fermat: %d %d\n", tv.tv_sec,
              tv.tv_usec);

	fprintf(fp, "estimate size: %d %d %d\n", estimate_flow_size[0], estimate_flow_size[1], estimate_flow_size[2]);
        
    
	double ratio = array_num * LOAD_FACTOR, min_ratio = array_num * MIN_LOAD_FACTOR;

        if (state == 0) {
          if (HH_error) {
            ensureHH(ratio);
          } else if (median_loss_error) {
            ensureHL_0(ratio, min_ratio);
          } else {
            AdjustMem_0(ratio, min_ratio);
          }
        } else {
          if (HH_error) {
            ensureHH(ratio);
          } else if (small_loss_error) {
            ensureLL_1(ratio);
          } else if (median_loss_error) {
            ensureHL_1(small_loss, ratio);
          } else {
            AdjustMem_1(median_loss, small_loss, ratio, min_ratio);
          }
        }

	// state = 1;
	// zone_size[0] = 512, zone_size[1] = 2560, zone_size[2] = 1024;
	// sample_rate = 13067;
	// HH_THRESH = 1996, HL_THRESH = 14;
	// send_adjust_pkts(1, HL_THRESH, HH_THRESH);

        gettimeofday(&tv, NULL);
        fprintf(fp, "timestamp of finishing analysis: %d %d\n\n", tv.tv_sec,
              tv.tv_usec);

        free(loss);
        free(small_loss);
        free(median_loss);

        memmove(old_ingress_sketch[time_flag], ingress_sketch, sizeof(struct FermatSketch) * dev_num * array_num * ingress_entry_num);
        memmove(old_egress_sketch[time_flag], egress_sketch, sizeof(struct FermatSketch) * dev_num * array_num * ingress_entry_num);

        memset(filter1_avail, 0, sizeof(bool) * dev_num);
        memset(filter2_avail, 0, sizeof(bool) * dev_num);
        memset(fermat_avail, 0, sizeof(bool) * dev_num);
        first_packet = 0;
        
        time_flag = 1 - time_flag;
	fflush(fp);
    }
  }
}

/**
 * The main function
 */
int main(int argc, char *argv[]) {
  unsigned nb_ports;

  logs = fopen("logs", "w");

  /* Initialize the Environment Abstraction Layer (EAL). */
  int ret = rte_eal_init(argc, argv);
  if (ret < 0)
    rte_exit(EXIT_FAILURE, "Error with EAL initialization\n");
  argc -= ret;
  argv += ret;

  dist = argv[1];
  filename = argv[2];

  /* Check that there is one port to send/receive on. */
  nb_ports = rte_eth_dev_count_avail();

  /* Creates a new mempool in memory to hold the mbufs. */
  mbuf_pool =
      rte_pktmbuf_pool_create("MBUF_POOL", NUM_MBUFS * nb_ports,
                              MBUF_CACHE_SIZE, 0, 9000, rte_socket_id());

  if (mbuf_pool == NULL)
    rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");

  /* Initialize all ports. */
  if (port_init(port_id, mbuf_pool) != 0)
    rte_exit(EXIT_FAILURE, "Cannot init port %" PRIu16 "\n", port_id);

  // TODO: initialize?

  lcore_main();

  rte_eth_dev_stop(port_id);
  rte_eth_dev_close(port_id);
  printf("Done.\n");

  return 0;
}
