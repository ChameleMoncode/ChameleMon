#include <bf_rt/bf_rt_info.hpp>
#include <bf_rt/bf_rt_init.hpp>
#include <bf_rt/bf_rt_common.h>
#include <bf_rt/bf_rt_table_key.hpp>
#include <bf_rt/bf_rt_table_data.hpp>
#include <bf_rt/bf_rt_table_operations.hpp>
#include <bf_rt/bf_rt_table.hpp>
#include <getopt.h>
#include<sys/time.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include<vector>
#include <arpa/inet.h>
extern "C"
{
    #include <bf_pm/bf_pm_intf.h>
    #include <pkt_mgr/pkt_mgr_intf.h>
    #include <bf_switchd/bf_switchd.h>
}
#define ALL_PIPES 0xffff
#define SWITCH_ID 1
uint64_t expected_time;
std::unique_ptr<bfrt::BfRtTableKey> bfrtTableKey;
std::unique_ptr<bfrt::BfRtTableData> bfrtTableData;

const bfrt::BfRtTable *get_map_index_table=nullptr;
bf_rt_id_t map_type_field_id = 0;
bf_rt_id_t get_map_index_action_id = 0;
bf_rt_id_t map_mask_field_id = 0;

const bfrt::BfRtTable *get_remap_index_table=nullptr;
bf_rt_id_t get_remap_index_action_id = 0;
bf_rt_id_t remap_mask_field_id = 0;

const bfrt::BfRtTable *get_drop_table=nullptr;
bf_rt_id_t get_drop_action_id = 0;
bf_rt_id_t up_field_id = 0;


const bfrt::BfRtTable *mem_zone_map_index_1_table=nullptr;
bf_rt_id_t mem_zone_map_type_1_field_id = 0;
bf_rt_id_t mem_zone_map_index_1_field_id = 0;
bf_rt_id_t mem_zone_map_index_1_action_id = 0;
bf_rt_id_t mem_zone_map_offset_1_field_id = 0;

const bfrt::BfRtTable *mem_zone_map_index_2_table=nullptr;
bf_rt_id_t mem_zone_map_type_2_field_id = 0;
bf_rt_id_t mem_zone_map_index_2_field_id = 0;
bf_rt_id_t mem_zone_map_index_2_action_id = 0;
bf_rt_id_t mem_zone_map_offset_2_field_id = 0;

const bfrt::BfRtTable *mem_zone_map_index_3_table=nullptr;
bf_rt_id_t mem_zone_map_type_3_field_id = 0;
bf_rt_id_t mem_zone_map_index_3_field_id = 0;
bf_rt_id_t mem_zone_map_index_3_action_id = 0;
bf_rt_id_t mem_zone_map_offset_3_field_id = 0;

const bfrt::BfRtTable *mem_zone_remap_index_1_table=nullptr;
bf_rt_id_t mem_zone_remap_index_1_field_id = 0;
bf_rt_id_t mem_zone_remap_index_1_action_id = 0;
bf_rt_id_t mem_zone_remap_offset_1_field_id = 0;

const bfrt::BfRtTable *mem_zone_remap_index_2_table=nullptr;
bf_rt_id_t mem_zone_remap_index_2_field_id = 0;
bf_rt_id_t mem_zone_remap_index_2_action_id = 0;
bf_rt_id_t mem_zone_remap_offset_2_field_id = 0;

const bfrt::BfRtTable *mem_zone_remap_index_3_table=nullptr;
bf_rt_id_t mem_zone_remap_index_3_field_id = 0;
bf_rt_id_t mem_zone_remap_index_3_action_id = 0;
bf_rt_id_t mem_zone_remap_offset_3_field_id = 0;


const bfrt::BfRtTable *filter_1_range_map_table=nullptr;
bf_rt_id_t filter_1_counter_field_id = 0;
bf_rt_id_t filter_1_range_map_action_id = 0;
bf_rt_id_t filter_1_type_field_id = 0;

const bfrt::BfRtTable *filter_2_range_map_table=nullptr;
bf_rt_id_t filter_2_counter_field_id = 0;
bf_rt_id_t filter_2_range_map_action_id = 0;
bf_rt_id_t filter_2_type_field_id = 0;

std::vector<std::unique_ptr<bfrt::BfRtTableKey>> ts_reg_key(1);
std::vector<std::unique_ptr<bfrt::BfRtTableData>> ts_reg_data(1);
const bfrt::BfRtTable *ts_reg = nullptr;
bf_rt_id_t ts_reg_data_id = 0;
uint64_t flag = 0;
bf_rt_target_t dev_tgt;
std::shared_ptr<bfrt::BfRtSession> session;

struct __attribute__((__packed__)) adjust_header
{
  uint8_t ethdstAddr[6];//6
  uint8_t ethsrcAddr[6];//6
  uint16_t ethtype;//2
  uint8_t tstamp;
  uint16_t zone_base[3];//6
  uint16_t zone_size[3];//6
  uint16_t mask[3];//6
  uint16_t drop_rate;//2
  uint8_t whether_range;//1
  uint16_t thresh[2];//6
  uint8_t payload[24];
};

typedef struct __attribute__((__packed__)) filter_t 
{
    uint8_t ethdstAddr[6];
    uint8_t ethsrcAddr[6];
    uint16_t ethtype;
    uint16_t index;
    uint32_t payload[12];
} filter;

typedef struct __attribute__((__packed__)) fermat_t 
{
    uint8_t ethdstAddr[6];
    uint8_t ethsrcAddr[6];
    uint16_t ethtype;
    uint16_t index;
    uint32_t payload[12];
} fermat;

size_t fermat_pkt_sz = sizeof(fermat);
size_t filter_pkt_sz = sizeof(filter);
bf_pkt_tx_ring_t tx_ring1 = BF_PKT_TX_RING_0;


void send_filter_packet (uint64_t timeflag, uint16_t type, uint16_t num) {
    
    
    for (int i=0 ; i< 8; i++)
    {   
        bf_pkt *bfpkt = NULL;
        filter pkt;
        while (1) 
        {
            if (bf_pkt_alloc(0, &bfpkt, filter_pkt_sz, (enum bf_dma_type_e)(17)) == 0)
                break;
            else printf("failed alloc\n");
        }
        uint8_t dstAddr[] = {0x3c, 0xfd, 0xfe, 0xad, 0x82, 0xe0};//{0x3c, 0xfd,0xfe, 0xb7, 0xe7, 0xf4};// {0xf4, 0xe7, 0xb7, 0xfe, 0xfd, 0x3c};
        uint8_t srcAddr[] = {0x00, 0x00, 0x00, 0x00, 0x00, SWITCH_ID};
        memcpy(pkt.ethdstAddr, dstAddr, 6);//{0x3c, 0xfd,0xfe, 0xb7, 0xe7, 0xf4}
        memcpy(pkt.ethsrcAddr, srcAddr, 6);//{0x3c, 0xfd,0xfe, 0xb7, 0xe7, 0xf4}
        pkt.ethtype = htons(type);
        if (timeflag == 1)
            pkt.index = htons(i*num/8);
        else
            pkt.index = htons(num+i*num/8);
        /*uint8_t * pkt_ptr = (uint8_t *) malloc(filter_pkt_sz);
        memcpy(pkt_ptr, &pkt, filter_pkt_sz);*/
        if (bf_pkt_data_copy(bfpkt, (uint8_t*)&pkt, filter_pkt_sz) != 0) {
            printf("Failed data copy\n");
        }
        bf_status_t stat = bf_pkt_tx(0, bfpkt, (bf_pkt_tx_ring_t)(0), (void *)bfpkt);
        if (stat  != BF_SUCCESS) 
        {
            printf("Failed to send packet status = %s\n", bf_err_str(stat));
            bf_pkt_free(0, bfpkt);
        }
    }
}


void send_fermat_packet (uint64_t timeflag, uint16_t type) {
   
    for (int i=0 ; i< 8; i++)
    {   
        bf_pkt *bfpkt = NULL;
        fermat pkt;
        while (1) 
        {
            if (bf_pkt_alloc(0, &bfpkt, fermat_pkt_sz, (enum bf_dma_type_e)(17)) == 0)
                break;
            else printf("failed alloc\n");
        }
        uint8_t dstAddr[] = {0x3c, 0xfd, 0xfe, 0xad, 0x82, 0xe0};//{0x3c, 0xfd,0xfe, 0xb7, 0xe7, 0xf4};// {0xf4, 0xe7, 0xb7, 0xfe, 0xfd, 0x3c};
        uint8_t srcAddr[] = {0x00, 0x00, 0x00, 0x00, 0x00, SWITCH_ID};
        memcpy(pkt.ethdstAddr, dstAddr, 6);//{0x3c, 0xfd,0xfe, 0xb7, 0xe7, 0xf4}
        memcpy(pkt.ethsrcAddr, srcAddr, 6);//{0x3c, 0xfd,0xfe, 0xb7, 0xe7, 0xf4}
        pkt.ethtype = htons(type);
        if (timeflag == 1)
            pkt.index = htons(4096*i/8);
        else
            pkt.index = htons(4096+4096*i/8);
        /*uint8_t * pkt_ptr = (uint8_t *) malloc(fermat_pkt_sz);
        memcpy(pkt_ptr, &pkt, fermat_pkt_sz);*/
        if (bf_pkt_data_copy(bfpkt, (uint8_t*)&pkt, fermat_pkt_sz) != 0) {
            printf("Failed data copy\n");
        }
        bf_status_t stat = bf_pkt_tx(0, bfpkt, (bf_pkt_tx_ring_t)(0), (void *)bfpkt);
        if (stat  != BF_SUCCESS) 
        {
            printf("Failed to send packet status = %s\n", bf_err_str(stat));
            bf_pkt_free(0, bfpkt);
        }
    }
}


namespace bfrt
{
    namespace examples
    {
        namespace Hierarchical_Fermat
        {
            const bfrt::BfRtInfo *bfrtInfo = nullptr;
            void init()
            {   dev_tgt.dev_id = 0;
                dev_tgt.pipe_id = ALL_PIPES;
                auto &devMgr = bfrt::BfRtDevMgr::getInstance();
                devMgr.bfRtInfoGet(dev_tgt.dev_id, "Hierarchical_Fermat", &bfrtInfo);
                // Create a session object
                session = bfrt::BfRtSession::sessionCreate();
                bfrtInfo->bfrtTableFromNameGet("Ingress.ts_reg", &ts_reg);

                bfrtInfo->bfrtTableFromNameGet("Ingress.get_map_index_t", &get_map_index_table);
                get_map_index_table->keyFieldIdGet("meta.type",&map_type_field_id);
                get_map_index_table->actionIdGet("Ingress.get_map_index_a",&get_map_index_action_id);
                get_map_index_table->dataFieldIdGet("mask",get_map_index_action_id, &map_mask_field_id);

                std::cout <<"******************"<<std::endl;
                std::cout<<map_type_field_id<<" "<<get_map_index_action_id<<" "<<map_mask_field_id<<std::endl;


                bfrtInfo->bfrtTableFromNameGet("Ingress.get_remap_index_t", &get_remap_index_table);
                get_remap_index_table->actionIdGet("Ingress.get_remap_index_a",&get_remap_index_action_id);
                get_remap_index_table->dataFieldIdGet("mask",get_remap_index_action_id, &remap_mask_field_id);

                std::cout <<"******************"<<std::endl;
                std::cout<<get_remap_index_action_id<<" "<<remap_mask_field_id<<std::endl;

                bfrtInfo->bfrtTableFromNameGet("Ingress.get_drop_t", &get_drop_table);
                get_drop_table->actionIdGet("Ingress.get_drop_a",&get_drop_action_id);
                get_drop_table->dataFieldIdGet("up",get_drop_action_id, &up_field_id);

                bfrtInfo->bfrtTableFromNameGet("Ingress.mem_zone_map_index_1_t", &mem_zone_map_index_1_table);
                mem_zone_map_index_1_table->keyFieldIdGet("meta.type", &mem_zone_map_type_1_field_id );
                mem_zone_map_index_1_table->keyFieldIdGet("hdr.bridge.index_1", &mem_zone_map_index_1_field_id );
                mem_zone_map_index_1_table->actionIdGet("Ingress.mem_zone_map_index_1_a", &mem_zone_map_index_1_action_id );
                mem_zone_map_index_1_table->dataFieldIdGet("offset",mem_zone_map_index_1_action_id, &mem_zone_map_offset_1_field_id );

                bfrtInfo->bfrtTableFromNameGet("Ingress.mem_zone_map_index_2_t", &mem_zone_map_index_2_table);
                mem_zone_map_index_2_table->keyFieldIdGet("meta.type", &mem_zone_map_type_2_field_id );
                mem_zone_map_index_2_table->keyFieldIdGet("hdr.bridge.index_2", &mem_zone_map_index_2_field_id );
                mem_zone_map_index_2_table->actionIdGet("Ingress.mem_zone_map_index_2_a", &mem_zone_map_index_2_action_id );
                mem_zone_map_index_2_table->dataFieldIdGet("offset",mem_zone_map_index_2_action_id, &mem_zone_map_offset_2_field_id );

                bfrtInfo->bfrtTableFromNameGet("Ingress.mem_zone_map_index_3_t", &mem_zone_map_index_3_table);
                mem_zone_map_index_3_table->keyFieldIdGet("meta.type", &mem_zone_map_type_3_field_id );
                mem_zone_map_index_3_table->keyFieldIdGet("hdr.bridge.index_3", &mem_zone_map_index_3_field_id );
                mem_zone_map_index_3_table->actionIdGet("Ingress.mem_zone_map_index_3_a", &mem_zone_map_index_3_action_id );
                mem_zone_map_index_3_table->dataFieldIdGet("offset",mem_zone_map_index_3_action_id, &mem_zone_map_offset_3_field_id );


                bfrtInfo->bfrtTableFromNameGet("Ingress.mem_zone_remap_index_1_t", &mem_zone_remap_index_1_table);
                mem_zone_remap_index_1_table->keyFieldIdGet("meta.re_index_1", &mem_zone_remap_index_1_field_id );
                mem_zone_remap_index_1_table->actionIdGet("Ingress.mem_zone_remap_index_1_a", &mem_zone_remap_index_1_action_id );
                mem_zone_remap_index_1_table->dataFieldIdGet("offset",mem_zone_remap_index_1_action_id, &mem_zone_remap_offset_1_field_id );


                bfrtInfo->bfrtTableFromNameGet("Ingress.mem_zone_remap_index_2_t", &mem_zone_remap_index_2_table);
                mem_zone_remap_index_2_table->keyFieldIdGet("meta.re_index_2", &mem_zone_remap_index_2_field_id );
                mem_zone_remap_index_2_table->actionIdGet("Ingress.mem_zone_remap_index_2_a", &mem_zone_remap_index_2_action_id );
                mem_zone_remap_index_2_table->dataFieldIdGet("offset",mem_zone_remap_index_2_action_id, &mem_zone_remap_offset_2_field_id );


                bfrtInfo->bfrtTableFromNameGet("Ingress.mem_zone_remap_index_3_t", &mem_zone_remap_index_3_table);
                mem_zone_remap_index_3_table->keyFieldIdGet("meta.re_index_3", &mem_zone_remap_index_3_field_id );
                mem_zone_remap_index_3_table->actionIdGet("Ingress.mem_zone_remap_index_3_a", &mem_zone_remap_index_3_action_id );
                mem_zone_remap_index_3_table->dataFieldIdGet("offset",mem_zone_remap_index_3_action_id, &mem_zone_remap_offset_3_field_id );


                bfrtInfo->bfrtTableFromNameGet("Ingress.filter_1_range_map_t", &filter_1_range_map_table);
                filter_1_range_map_table->keyFieldIdGet("meta.layer_1_filter_counter", &filter_1_counter_field_id );
                filter_1_range_map_table->actionIdGet("Ingress.filter_1_range_map_a", &filter_1_range_map_action_id );
                filter_1_range_map_table->dataFieldIdGet("type",filter_1_range_map_action_id, &filter_1_type_field_id );


                bfrtInfo->bfrtTableFromNameGet("Ingress.filter_2_range_map_t", &filter_2_range_map_table);
                filter_2_range_map_table->keyFieldIdGet("meta.layer_2_filter_counter", &filter_2_counter_field_id );
                filter_2_range_map_table->actionIdGet("Ingress.filter_2_range_map_a", &filter_2_range_map_action_id );
                filter_2_range_map_table->dataFieldIdGet("type",filter_2_range_map_action_id, &filter_2_type_field_id );


                




            }

            void register_init(const bfrt::BfRtTable* table, std::vector<std::unique_ptr<BfRtTableKey>>& keys, std::vector<std::unique_ptr<BfRtTableData>>& data, std::string data_name, uint32_t entry_num, bf_rt_id_t& data_id)
            {
                table->dataFieldIdGet(data_name, &data_id);
                //get data for table
                BfRtTable::keyDataPairs key_data_pairs;
                for (unsigned i = 0; i <entry_num; ++i) 
                {
                    table->keyAllocate(&keys[i]);
                    table->dataAllocate(&data[i]);
                }
                for (unsigned i = 1; i <  entry_num; ++i) 
                {
                    key_data_pairs.push_back(std::make_pair(keys[i].get(), data[i].get()));
                }
                auto flag = bfrt::BfRtTable::BfRtTableGetFlag::GET_FROM_HW;
                table->tableEntryGetFirst(*session, dev_tgt, flag, keys[0].get(), data[0].get());
                session->sessionCompleteOperations();
                if (entry_num>1)
                {  
                    uint32_t num_returned = 0;
                    table->tableEntryGetNext_n(*session, dev_tgt, *keys[0].get(), entry_num - 1, flag, &key_data_pairs, &num_returned);
                    session->sessionCompleteOperations();
                }
            }
            void filp_time(const bfrt::BfRtTable* table, std::vector<std::unique_ptr<BfRtTableKey>>& keys, std::vector<std::unique_ptr<BfRtTableData>>& data, bf_rt_id_t& data_id )
            {
                table->dataAllocate(&data[0]);
                data[0]->setValue(data_id, flag);
                table->tableEntryMod(*session, dev_tgt, *(keys[0].get()), *(data[0].get()));
                session->sessionCompleteOperations();
            }
        }
    }
}





void init_ports()
{
  system("./bfshell -f /mnt/onl/data/Hierarchical_Fermat/port-setup.txt");
}

void init_tables()
{
    system("pwd");
    system("./bfshell -b /mnt/onl/data/Hierarchical_Fermat/tableinit.py");
}

static bf_status_t switch_pktdriver_tx_complete(bf_dev_id_t device,
                                                bf_pkt_tx_ring_t tx_ring,
                                                uint64_t tx_cookie,
                                                uint32_t status) {

  bf_pkt *pkt = (bf_pkt *)(uintptr_t)tx_cookie;
  (void)device;
  (void)tx_ring;
  (void)tx_cookie;
  (void)status;
  bf_pkt_free(device, pkt);
  return 0;
}

uint64_t min(uint64_t a, uint64_t b)
{
    if (a>b)
    return b;
    else 
    return a;
}

void adjust(struct adjust_header &ah)
{
    std::ofstream fout("/mnt/onl/data/Hierarchical_Fermat/time_adjust",std::ios::app);
    int upper_bound = 65535;
    int true_up = 0;
    int num_entry[3]={0,0,0};
    int res = 0;
    uint64_t offsets[3][100];
    uint64_t indexes[3][100];
    std::cout<<ah.zone_base[0]<<" "<<ah.zone_base[1]<<" "<<ah.zone_base[2]<<" "<<ah.zone_size[0]<<" "<<ah.zone_size[1]<<" "<<ah.zone_size[2]<<" "<<ah.mask[0]<<" "<<ah.mask[1]<<" "<<ah.mask[2]<<" "<<ah.mask[3]<<" "<<ah.drop_rate<<" "<<ah.whether_range<<" "<<ah.thresh[0]<<" "<<ah.thresh[1]<<std::endl;
    auto start= std::chrono::system_clock::now();
    for (int j = 0 ; j < 3 ; j++)
    {   
        if (ah.zone_size[j]>0)
        {
            true_up = upper_bound & ah.mask[j];
            res = true_up % ah.zone_size[j];
            num_entry[j] = ((true_up - res)/ ah.zone_size[j]) + 1;
            for (int k = 0; k < num_entry[j]; k++)
            {
                indexes[j][k] = k * ah.zone_size[j];
                if (ah.zone_base[j] >= k*ah.zone_size[j])
                    offsets[j][k] = ah.zone_base[j] - k*ah.zone_size[j];
                else
                    offsets[j][k] = 65536 - k*ah.zone_size[j] + ah.zone_base[j];
            }
            indexes[j][num_entry[j]] = (uint64_t)true_up + 1;
        }
    }
    session->beginBatch();
  //adjust_mask
  
    get_map_index_table->tableClear(*session, dev_tgt);
    mem_zone_map_index_1_table->tableClear(*session, dev_tgt);
    mem_zone_map_index_2_table->tableClear(*session, dev_tgt);
    mem_zone_map_index_3_table->tableClear(*session, dev_tgt);
    mem_zone_remap_index_1_table->tableClear(*session, dev_tgt);
    mem_zone_remap_index_2_table->tableClear(*session, dev_tgt);
    mem_zone_remap_index_3_table->tableClear(*session, dev_tgt);
    if (ah.whether_range == 1)
    {
        filter_1_range_map_table->tableClear(*session, dev_tgt);
        filter_2_range_map_table->tableClear(*session, dev_tgt);
    }
    get_map_index_table->keyAllocate(&bfrtTableKey);
    get_map_index_table->dataAllocate(&bfrtTableData);
    for (int j=0;j<3;j++)
    {
        get_map_index_table->keyReset(bfrtTableKey.get());
        get_map_index_table->dataReset(get_map_index_action_id, bfrtTableData.get());
        bfrtTableKey->setValue(map_type_field_id, (uint64_t)(j+1));
        bfrtTableData->setValue(map_mask_field_id, (uint64_t)ah.mask[j]);
        get_map_index_table->tableEntryAdd(*session, dev_tgt, *bfrtTableKey, *bfrtTableData);
    }

    get_remap_index_table->dataAllocate(&bfrtTableData);
    get_remap_index_table->dataReset(get_remap_index_action_id, bfrtTableData.get());
    bfrtTableData->setValue(remap_mask_field_id, (uint64_t)ah.mask[1]);
    get_remap_index_table->tableDefaultEntrySet(*session, dev_tgt, *bfrtTableData);

  //adjust_loss_rate
    get_drop_table->dataAllocate(&bfrtTableData);
    get_drop_table->dataReset(get_drop_action_id, bfrtTableData.get());
    bfrtTableData->setValue(up_field_id, (uint64_t)ah.drop_rate);
    get_drop_table->tableDefaultEntrySet(*session, dev_tgt, *bfrtTableData);
  //adjust_offset   
        for (int j = 0; j < 3 ; j++)
        for (int l = 0; l < num_entry[j]; l++)
        {   
            mem_zone_map_index_1_table->keyAllocate(&bfrtTableKey);
            mem_zone_map_index_1_table->dataAllocate(&bfrtTableData);
            mem_zone_map_index_1_table->keyReset(bfrtTableKey.get());
            mem_zone_map_index_1_table->dataReset(mem_zone_map_index_1_action_id, bfrtTableData.get());
            bfrtTableKey->setValue(mem_zone_map_type_1_field_id, (uint64_t)(j+1));
            bfrtTableKey->setValueRange(mem_zone_map_index_1_field_id, indexes[j][l], indexes[j][l+1] - 1);
            bfrtTableData->setValue(mem_zone_map_offset_1_field_id, offsets[j][l]);
            mem_zone_map_index_1_table->tableEntryAdd(*session, dev_tgt, *bfrtTableKey, *bfrtTableData);

            mem_zone_map_index_2_table->keyAllocate(&bfrtTableKey);
            mem_zone_map_index_2_table->dataAllocate(&bfrtTableData);
            mem_zone_map_index_2_table->keyReset(bfrtTableKey.get());
            mem_zone_map_index_2_table->dataReset(mem_zone_map_index_2_action_id, bfrtTableData.get());
            bfrtTableKey->setValue(mem_zone_map_type_2_field_id, (uint64_t)(j+1));
            bfrtTableKey->setValueRange(mem_zone_map_index_2_field_id, indexes[j][l], indexes[j][l+1] - 1);
            bfrtTableData->setValue(mem_zone_map_offset_2_field_id, offsets[j][l]);
            mem_zone_map_index_2_table->tableEntryAdd(*session, dev_tgt, *bfrtTableKey, *bfrtTableData);

            mem_zone_map_index_3_table->keyAllocate(&bfrtTableKey);
            mem_zone_map_index_3_table->dataAllocate(&bfrtTableData);
            mem_zone_map_index_3_table->keyReset(bfrtTableKey.get());
            mem_zone_map_index_3_table->dataReset(mem_zone_map_index_3_action_id, bfrtTableData.get());
            bfrtTableKey->setValue(mem_zone_map_type_3_field_id, (uint64_t)(j+1));
            bfrtTableKey->setValueRange(mem_zone_map_index_3_field_id, indexes[j][l], indexes[j][l+1] - 1);
            bfrtTableData->setValue(mem_zone_map_offset_3_field_id, offsets[j][l]);
            mem_zone_map_index_3_table->tableEntryAdd(*session, dev_tgt, *bfrtTableKey, *bfrtTableData);

            if (j == 1)
            {
                mem_zone_remap_index_1_table->keyAllocate(&bfrtTableKey);
                mem_zone_remap_index_1_table->dataAllocate(&bfrtTableData);
                mem_zone_remap_index_1_table->keyReset(bfrtTableKey.get());
                mem_zone_remap_index_1_table->dataReset(mem_zone_remap_index_1_action_id, bfrtTableData.get());
                bfrtTableKey->setValueRange(mem_zone_remap_index_1_field_id, indexes[j][l], indexes[j][l+1] - 1);
                bfrtTableData->setValue(mem_zone_remap_offset_1_field_id, offsets[j][l]);
                mem_zone_remap_index_1_table->tableEntryAdd(*session, dev_tgt, *bfrtTableKey, *bfrtTableData);

                mem_zone_remap_index_2_table->keyAllocate(&bfrtTableKey);
                mem_zone_remap_index_2_table->dataAllocate(&bfrtTableData);
                mem_zone_remap_index_2_table->keyReset(bfrtTableKey.get());
                mem_zone_remap_index_2_table->dataReset(mem_zone_remap_index_2_action_id, bfrtTableData.get());
                bfrtTableKey->setValueRange(mem_zone_remap_index_2_field_id, indexes[j][l], indexes[j][l+1] - 1);
                bfrtTableData->setValue(mem_zone_remap_offset_2_field_id, offsets[j][l]);
                mem_zone_remap_index_2_table->tableEntryAdd(*session, dev_tgt, *bfrtTableKey, *bfrtTableData);

                mem_zone_remap_index_3_table->keyAllocate(&bfrtTableKey);
                mem_zone_remap_index_3_table->dataAllocate(&bfrtTableData);
                mem_zone_remap_index_3_table->keyReset(bfrtTableKey.get());
                mem_zone_remap_index_3_table->dataReset(mem_zone_remap_index_3_action_id, bfrtTableData.get());
                bfrtTableKey->setValueRange(mem_zone_remap_index_3_field_id, indexes[j][l], indexes[j][l+1] - 1);
                bfrtTableData->setValue(mem_zone_remap_offset_3_field_id, offsets[j][l]);
                mem_zone_remap_index_3_table->tableEntryAdd(*session, dev_tgt, *bfrtTableKey, *bfrtTableData);
            }
        }

  //adjust thresh
    if (ah.whether_range == 1)
    {
        filter_1_range_map_table->keyAllocate(&bfrtTableKey);
        filter_1_range_map_table->dataAllocate(&bfrtTableData);
        filter_1_range_map_table->keyReset(bfrtTableKey.get());
        filter_1_range_map_table->dataReset(filter_1_range_map_action_id, bfrtTableData.get());
    
        if (ah.thresh[0]>0)
        {
            bfrtTableKey->setValueRange(filter_1_counter_field_id, 0, min(254,ah.thresh[0]-1));
            bfrtTableData->setValue(filter_1_type_field_id, (uint64_t)1);
            filter_1_range_map_table->tableEntryAdd(*session, dev_tgt, *bfrtTableKey, *bfrtTableData);
        }
        if (ah.thresh[0]<=254)
        {
            filter_1_range_map_table->keyReset(bfrtTableKey.get());
            filter_1_range_map_table->dataReset(filter_1_range_map_action_id, bfrtTableData.get());
            bfrtTableKey->setValueRange(filter_1_counter_field_id, ah.thresh[0], min(254,ah.thresh[1]-1));
            bfrtTableData->setValue(filter_1_type_field_id, (uint64_t)2);
            filter_1_range_map_table->tableEntryAdd(*session, dev_tgt, *bfrtTableKey, *bfrtTableData);
        }
        filter_1_range_map_table->keyReset(bfrtTableKey.get());
        filter_1_range_map_table->dataReset(filter_1_range_map_action_id, bfrtTableData.get());
        bfrtTableKey->setValueRange(filter_1_counter_field_id, min(255,ah.thresh[1]), 255);
        bfrtTableData->setValue(filter_1_type_field_id, (uint64_t)3);
        filter_1_range_map_table->tableEntryAdd(*session, dev_tgt, *bfrtTableKey, *bfrtTableData);
        


        filter_2_range_map_table->keyAllocate(&bfrtTableKey);
        filter_2_range_map_table->dataAllocate(&bfrtTableData);
        filter_2_range_map_table->keyReset(bfrtTableKey.get());
        filter_2_range_map_table->dataReset(filter_2_range_map_action_id, bfrtTableData.get());
    
        if (ah.thresh[0]>0)
        {
            bfrtTableKey->setValueRange(filter_2_counter_field_id, 0, min(65534,ah.thresh[0]-1));
            bfrtTableData->setValue(filter_2_type_field_id, (uint64_t)1);
            filter_2_range_map_table->tableEntryAdd(*session, dev_tgt, *bfrtTableKey, *bfrtTableData);
        }
        if (ah.thresh[0]<=65534)
        {
            filter_2_range_map_table->keyReset(bfrtTableKey.get());
            filter_2_range_map_table->dataReset(filter_2_range_map_action_id, bfrtTableData.get());
            bfrtTableKey->setValueRange(filter_2_counter_field_id, ah.thresh[0], min(65534,ah.thresh[1]-1));
            bfrtTableData->setValue(filter_2_type_field_id, (uint64_t)2);
            filter_2_range_map_table->tableEntryAdd(*session, dev_tgt, *bfrtTableKey, *bfrtTableData);
        }
        filter_2_range_map_table->keyReset(bfrtTableKey.get());
        filter_2_range_map_table->dataReset(filter_2_range_map_action_id, bfrtTableData.get());
        bfrtTableKey->setValueRange(filter_2_counter_field_id, min(65535,ah.thresh[1]), 65535);
        bfrtTableData->setValue(filter_2_type_field_id, (uint64_t)3);
        filter_2_range_map_table->tableEntryAdd(*session, dev_tgt, *bfrtTableKey, *bfrtTableData);




    }
    session->endBatch(true);
    session->sessionCompleteOperations();
    auto end= std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = end-start;
    std::cout<<"time consumption: "<<elapsed_seconds.count()<<std::endl;
    fout<<elapsed_seconds.count()<<std::endl << std::flush;
}


bf_status_t rx_packet_callback (bf_dev_id_t dev_id,
                                bf_pkt *pkt,
                                void *cookie,
                                bf_pkt_rx_ring_t rx_ring) {
    (void)dev_id;
    (void)cookie;
    (void)rx_ring;
    printf("Packet received..\n");
    struct adjust_header ah;
    
    memcpy(&ah,pkt->pkt_data,sizeof(struct adjust_header));
    bf_pkt_free(0,pkt);
     //read
    adjust(ah);
    return 0;
}

void switch_pktdriver_callback_register(bf_dev_id_t device) {

    bf_pkt_tx_ring_t tx_ring;
    bf_pkt_rx_ring_t rx_ring;
    bf_status_t status;
    int cookie;
    /* register callback for TX complete */
    for (tx_ring = BF_PKT_TX_RING_0; tx_ring < BF_PKT_TX_RING_MAX; tx_ring = (bf_pkt_tx_ring_t)(tx_ring + 1)) {
        bf_pkt_tx_done_notif_register(device, switch_pktdriver_tx_complete, tx_ring);
    }
    /* register callback for RX */
    for (rx_ring = BF_PKT_RX_RING_0; rx_ring < BF_PKT_RX_RING_MAX; rx_ring = (bf_pkt_rx_ring_t)(rx_ring + 1)) {
        status = bf_pkt_rx_register(device, rx_packet_callback, rx_ring, (void *) &cookie);
    }
    printf("rx register done. stat = %d\n", status);
}
static void parse_options(bf_switchd_context_t *switchd_ctx,
                          int argc,
                          char **argv) {
  int option_index = 0;
  enum opts {
    OPT_INSTALLDIR = 1,
    OPT_CONFFILE,
    OPT_TIME,
  };
  static struct option options[] = {
      {"help", no_argument, 0, 'h'},
      {"install-dir", required_argument, 0, OPT_INSTALLDIR},
      {"conf-file", required_argument, 0, OPT_CONFFILE},
      {"expected-time", required_argument, 0, OPT_TIME}};

  while (1) {
    int c = getopt_long(argc, argv, "h", options, &option_index);

    if (c == -1) {
      break;
    }
    switch (c) {
      case OPT_INSTALLDIR:
        switchd_ctx->install_dir = strdup(optarg);
        printf("Install Dir: %s\n", switchd_ctx->install_dir);
        break;
      case OPT_CONFFILE:
        switchd_ctx->conf_file = strdup(optarg);
        printf("Conf-file : %s\n", switchd_ctx->conf_file);
        break;
      case OPT_TIME:
        expected_time = strtoull(strdup(optarg), NULL, 10);
        printf("expected_time : %lu\n", expected_time);
      break;

      case 'h':
      case '?':
        printf("bfrt_perf \n");
        printf(
            "Usage : bfrt_perf --install-dir <path to where the SDE is "
            "installed> --conf-file <full path to the conf file "
            "(bfrt_perf.conf)\n");
        exit(c == 'h' ? 0 : 1);
        break;
      default:
        printf("Invalid option\n");
        exit(0);
        break;
    }
  }
  if (switchd_ctx->install_dir == NULL) {
    printf("ERROR : --install-dir must be specified\n");
    exit(0);
  }

  if (switchd_ctx->conf_file == NULL) {
    printf("ERROR : --conf-file must be specified\n");
    exit(0);
  }
}

int main(int argc, char **argv) {
    bf_switchd_context_t *switchd_ctx;
    if ((switchd_ctx = (bf_switchd_context_t *)calloc(1, sizeof(bf_switchd_context_t))) == NULL) {
        printf("Cannot Allocate switchd context\n");
        exit(1);
    }
    parse_options(switchd_ctx, argc, argv);
    switchd_ctx->running_in_background = true;
    bf_status_t status = bf_switchd_lib_init(switchd_ctx);
    init_ports();
    init_tables();
    switch_pktdriver_callback_register(0);
    bfrt::examples::Hierarchical_Fermat::init();      
    bfrt::examples::Hierarchical_Fermat::register_init(ts_reg, ts_reg_key,ts_reg_data, "Ingress.ts_reg.f1", 1, ts_reg_data_id);
    struct timeval tmv;
    gettimeofday(&tmv, NULL);
    uint32_t interval = 50;
    uint32_t times = 0;
    std::cout << tmv.tv_sec << "   " << tmv.tv_usec << "  " << expected_time << std::endl;

    
    if (expected_time > 0)
    usleep((expected_time - tmv.tv_sec) * 1000000 - tmv.tv_usec);
    while (1)
    {
        //std::cout<< "start flipping!" << std::endl;
        flag = 1 - flag;
        bfrt::examples::Hierarchical_Fermat::filp_time(ts_reg, ts_reg_key, ts_reg_data, ts_reg_data_id);
        //std::cout<< "time flipping!" << std::endl;
        times++;
        
        std::cout<< "start filter" << std::endl;
        send_filter_packet(flag,0x2221,32768);
	//usleep(15000000);
        send_filter_packet(flag,0x2222,16384);
        usleep(10000);
	std::cout<< "start fermat" << std::endl;
        send_fermat_packet(flag, 0x3333);
        
        gettimeofday(&tmv, NULL);
        if (expected_time > 0)
            usleep(expected_time * 1000000 + times * interval * 1000 - tmv.tv_sec * 1000000 - tmv.tv_usec);
        else
            usleep(15000000);
    }

    return status;
}
