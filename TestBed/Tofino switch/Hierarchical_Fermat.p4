/* -*- P4_16 -*- */
//need to handle ARP
//line clean_layer_## L ##_filter.execute(meta.filter_index_## L ##); need to modify meta.filter_index_## L ##, now line 102
#include <core.p4>
#include <tna.p4>
//#include "macros.p4"
#define PRIME 4294967291
#define SWITCH_ID 1
#define RECIRCULATE_PORT_0 28
#define RECIRCULATE_PORT_1 196
#define OUTPUT_PORT 16
#define CPU_PORT 192
//???? a 32-bitnumber
#define entry_num 8192
#define LARGE_OFFSET 4096
#define SMALL 1
#define LARGE 0
#define MASK_ID 0x7fffffff
//hash_## R ##.get({1w0,hdr.ipv4.src_addr[30:0],1w0,hdr.ipv4.dst_addr[30:0],1w0,meta.ll[30:0],5w0,hdr.ipv4.src_addr[31:31],hdr.ipv4.dst_addr[31:31],meta.ll[31:31],hdr.ipv4.protocol});     \

#define CAL_HASH(R)     \
action cal_hash_## R ##_a(){  \
    hdr.bridge.index_## R ## = hash_## R ##.get({1w0,hdr.ipv4.src_addr[30:0],1w0,hdr.ipv4.dst_addr[30:0],1w0,meta.ll[30:0],5w0,hdr.ipv4.src_addr[31:31],hdr.ipv4.dst_addr[31:31],meta.ll[31:31],hdr.ipv4.protocol}); \
} \
@stage(0) table cal_hash_## R ##_t {           \
    actions = {  \
        cal_hash_## R ##_a; \
    } \
    default_action = cal_hash_## R ##_a; \
} \


//hash_filter_## R ##.get({1w0,hdr.ipv4.src_addr[30:0],1w0,hdr.ipv4.dst_addr[30:0],1w0,meta.ll[30:0],5w0,hdr.ipv4.src_addr[31:31],hdr.ipv4.dst_addr[31:31],meta.ll[31:31],hdr.ipv4.protocol}); \


#define CAL_FILTER_HASH(R,C)     \
action cal_filter_hash_## R ##_a(){  \
    meta.filter_index_## R ##[## C ##:0] = hash_filter_## R ##.get({1w0,hdr.ipv4.src_addr[30:0],1w0,hdr.ipv4.dst_addr[30:0],1w0,meta.ll[30:0],5w0,hdr.ipv4.src_addr[31:31],hdr.ipv4.dst_addr[31:31],meta.ll[31:31],hdr.ipv4.protocol}); \
} \
@stage(0) table cal_filter_hash_## R ##_t {           \
    actions = {  \
        cal_filter_hash_## R ##_a; \
    } \
    default_action = cal_filter_hash_## R ##_a; \
} \

#define CAL_DIF(R)     \
action cal_DIF_a(){  \
    meta.dif_1 = meta.P - hdr.bridge.ID_1;              \
    meta.dif_2 = meta.P - hdr.bridge.ID_2;              \
    meta.dif_3 = meta.P - hdr.bridge.ID_3;              \
    meta.dif_4 = meta.P - hdr.bridge.ID_4;              \
} \
@stage(## R ##) table cal_DIF_t {           \
    actions = {  \
        cal_DIF_a; \
    } \
    default_action = cal_DIF_a; \
} \


#define MEM_ZONE_MAP_INDEX(R)     \
action mem_zone_map_index_## R ##_a(bit<16> offset){  \
    hdr.bridge.index_## R ## = hdr.bridge.index_## R ##  + offset;              \
} \
@stage(6) table mem_zone_map_index_## R ##_t { \
    key = { meta.type : exact; hdr.bridge.index_## R ## : range;} \
    actions = {  \
        mem_zone_map_index_## R ##_a;\
    } \
    default_action = mem_zone_map_index_## R ##_a(0); \
} \

#define MEM_ZONE_REMAP_INDEX(R)     \
action mem_zone_remap_index_## R ##_a(bit<16> offset){  \
    meta.re_index_## R ## = meta.re_index_## R ## + offset;              \
} \
@stage(4) table mem_zone_remap_index_## R ##_t { \
    key = { meta.re_index_## R ## : range;} \
    actions = {  \
        mem_zone_remap_index_## R ##_a;\
    } \
    default_action = mem_zone_remap_index_## R ##_a(0); \
} \

#define MAP_IN_HASH(C)     \
action map_in_hash_a(){  \
    meta.index_1 = hdr.bridge.index_1 + ## C ##;              \
    meta.index_2 = hdr.bridge.index_2 + ## C ##;              \
    meta.index_3 = hdr.bridge.index_3 + ## C ##;              \
    hdr.ipv4.type = meta.type; \
} \
action directmap_a(){  \
    meta.index_1 = hdr.bridge.index_1;              \
    meta.index_2 = hdr.bridge.index_2;              \
    meta.index_3 = hdr.bridge.index_3;              \
    hdr.ipv4.type = meta.type; \
} \
@stage(7) table map_in_hash_t { \
    key = {meta.tstamp : exact;} \
    actions = {  \
        map_in_hash_a; directmap_a;\
    } \
    default_action = directmap_a; \
} \

#define MAP_OUT_HASH(C)     \
action map_out_hash_a(){  \
    meta.index_1 = hdr.bridge.index_1 + ## C ##;              \
    meta.index_2 = hdr.bridge.index_2 + ## C ##;              \
    meta.index_3 = hdr.bridge.index_3 + ## C ##;              \
} \
action directmap_a(){  \
    meta.index_1 = hdr.bridge.index_1;              \
    meta.index_2 = hdr.bridge.index_2;              \
    meta.index_3 = hdr.bridge.index_3;              \
} \
@stage(3) table map_out_hash_t { \
    key = {hdr.ipv4.res : exact;} \
    actions = {  \
        map_out_hash_a; directmap_a;\
    } \
    default_action = directmap_a; \
} \

#define MAP_FILTER_HASH(R,C)     \
action map_filter_hash_## R ##_a(){  \
    meta.filter_index_## R ## = meta.filter_index_## R ## + ## C ##;              \
} \
@stage(1) table map_filter_hash_## R ##_t { \
    key = {meta.tstamp : exact;} \
    actions = {  \
        map_filter_hash_## R ##_a; NoAction;\
    } \
    default_action = NoAction; \
} \


#define Filter(L,R,S,E,P)     \
Register<bit<## R ##>, bit<16>>(## E ##) layer_## L ##_filter; \
RegisterAction<bit<## R ##>, bit<16>, bit<## R ##>>(layer_## L ##_filter) insert_layer_## L ##_filter = \
{ \
    void apply(inout bit<## R ##> register_data, out bit<## R ##> result) { \ 
        result = register_data; \
        if (meta.in_flag > 0) \
        { \ 
            register_data = register_data |+| 1; \ 
        } \ 
        else \ 
        { \ 
            register_data = 0; \ 
        } \
    } \ 
}; \
action insert_layer_## L ##_filter_a(){  \
    meta.layer_## L ##_filter_counter = insert_layer_## L ##_filter.execute(meta.filter_index_## L ##);              \
} \
@stage(## S ##) table insert_layer_## L ##_filter_t {           \
    actions = {  \
        insert_layer_## L ##_filter_a; \
    } \
    default_action = insert_layer_## L ##_filter_a; \
} \








#define Fermat_Ingress(L,R,S,E)     \
Register<bit<32>, bit<16>>(## E ##) layer_## L ##_part_## R ##; \
RegisterAction<bit<32>, bit<16>, bit<32>>(layer_## L ##_part_## R ##) insert_layer_## L ##_part_## R ## = \
{ \
    void apply(inout bit<32> register_data, out bit<32> result) { \ 
        result = register_data; \
        if (register_data < meta.dif_## R ##) \
        { \ 
            register_data = register_data + hdr.bridge.ID_## R ##; \ 
        } \ 
        else \ 
        { \ 
            register_data = register_data - meta.dif_## R ##; \ 
        } \
    } \ 
}; \
action insert_layer_## L ##_part_## R ##_a(){  \
    hdr.output_layer_## L ##.ID_## R ## = insert_layer_## L ##_part_## R ##.execute(meta.index_## L ##);              \
} \
@stage(## S ##) table insert_layer_## L ##_part_## R ##_t {           \
    actions = {  \
        insert_layer_## L ##_part_## R ##_a; \
    } \
    default_action = insert_layer_## L ##_part_## R ##_a; \
} \

#define Fermat_Egress(L,R,S,E)     \
Register<bit<32>, bit<16>>(## E ##) layer_## L ##_part_## R ##; \
RegisterAction<bit<32>, bit<16>, bit<32>>(layer_## L ##_part_## R ##) insert_layer_## L ##_part_## R ## = \
{ \
    void apply(inout bit<32> register_data, out bit<32> result) { \ 
        result = register_data; \
        if (register_data < meta.dif_## R ##) \
        { \ 
            register_data = register_data + hdr.bridge.ID_## R ##; \ 
        } \ 
        else \ 
        { \ 
            register_data = register_data - meta.dif_## R ##; \ 
        } \
    } \ 
}; \
action insert_layer_## L ##_part_## R ##_a(){  \
    hdr.output_layer_## L ##.ID_## R ## = insert_layer_## L ##_part_## R ##.execute(meta.index_## L ##);              \
} \
@stage(## S ##) table insert_layer_## L ##_part_## R ##_t {           \
    actions = {  \
        insert_layer_## L ##_part_## R ##_a; \
    } \
    default_action = insert_layer_## L ##_part_## R ##_a; \
} \

#define Counter_Ingress(L,S,E)     \
Register<bit<32>, bit<16>>(## E ##) layer_## L ##_counter; \
RegisterAction<bit<32>, bit<16>, bit<32>>(layer_## L ##_counter) insert_layer_## L ##_counter = \
{ \
    void apply(inout bit<32> register_data, out bit<32> result) { \ 
        result = register_data; \
        register_data = register_data + meta.counter; \ 
    } \ 
}; \
action insert_layer_## L ##_counter_a(){  \
    hdr.output_layer_## L ##.counter = insert_layer_## L ##_counter.execute(meta.index_## L ##);              \
} \
@stage(## S ##) table insert_layer_## L ##_counter_t {           \
    actions = {  \
        insert_layer_## L ##_counter_a; \
    } \
    default_action = insert_layer_## L ##_counter_a; \
} \

#define Counter_Egress(L,S,E)     \
Register<bit<32>, bit<16>>(## E ##) layer_## L ##_counter; \
RegisterAction<bit<32>, bit<16>, bit<32>>(layer_## L ##_counter) insert_layer_## L ##_counter = \
{ \
    void apply(inout bit<32> register_data, out bit<32> result) { \ 
        result = register_data; \
        register_data = register_data + meta.counter; \ 
    } \ 
}; \
action insert_layer_## L ##_counter_a(){  \
    hdr.output_layer_## L ##.counter = insert_layer_## L ##_counter.execute(meta.index_## L ##);              \
} \
@stage(## S ##) table insert_layer_## L ##_counter_t {           \
    actions = {  \
        insert_layer_## L ##_counter_a; \
    } \
    default_action = insert_layer_## L ##_counter_a; \
} \
/*************************************************************************
 ************* C O N S T A N T S    A N D   T Y P E S  *******************
*************************************************************************/
enum bit<16> ether_type_t {
    TPID       = 0x8100,
    IPV4       = 0x0800,
    ARP        = 0x0806,
    FERMAT     = 0x3333,
    FERMAT_END = 0x3334,
    FERMAT_NEW = 0x3335,
    CLEAN_1    = 0x2221,
    CLEAN_2    = 0x2222,
    CTRL    = 0x88b5
}

enum bit<8>  ip_proto_t {
    ICMP  = 1,
    IGMP  = 2,
    TCP   = 6,
    UDP   = 17
}


type bit<48> mac_addr_t;

/*************************************************************************
 ***********************  H E A D E R S  *********************************
 *************************************************************************/
/*  Define all the headers the program will recognize             */
/*  The actual sets of headers processed by each gress can differ */

/* Standard ethernet header */
header ethernet_h {
    mac_addr_t    dst_addr;
    mac_addr_t    src_addr;
    bit<16>  ether_type;
}

header vlan_tag_h {
    bit<3>        pcp;
    bit<1>        cfi;
    bit<12>       vid;
    ether_type_t  ether_type;
}

header arp_h {
    bit<16>       htype;
    bit<16>       ptype;
    bit<8>        hlen;
    bit<8>        plen;
    bit<16>       opcode;
    mac_addr_t    hw_src_addr;
    bit<32>       proto_src_addr;
    mac_addr_t    hw_dst_addr;
    bit<32>       proto_dst_addr;
}

header ipv4_h {
    bit<4>       version;
    bit<4>       ihl;
    bit<1>       res;
    bit<2>       type;
    bit<3>       dscp;
    bit<2>       ecn;
    bit<16>      total_len;
    bit<16>      identification;
    bit<3>       flags;
    bit<13>      frag_offset;
    bit<8>       ttl;
    bit<8>       protocol;
    bit<16>      hdr_checksum;
    bit<32>      src_addr;
    bit<32>      dst_addr;
}

header icmp_h {
    bit<16>  type_code;
    bit<16>  checksum;
}

header igmp_h {
    bit<16>  type_code;
    bit<16>  checksum;
}

header tcp_h {
    bit<16>  src_port;
    bit<16>  dst_port;
    bit<32>  seq_no;
    bit<32>  ack_no;
    bit<4>   data_offset;
    bit<4>   res;
    bit<8>   flags;
    bit<16>  window;
    bit<16>  checksum;
    bit<16>  urgent_ptr;
}

header udp_h {
    bit<16>  src_port;
    bit<16>  dst_port;
    bit<16>  len;
    bit<16>  checksum;
}
header bridge_h
{   
    bit<16> index_1;
    bit<16> index_2;
    bit<16> index_3;
    bit<32> ID_1;
    bit<32> ID_2;
    bit<32> ID_3;
    bit<32> ID_4;
}
header clean_mirror_h
{
    bit<48> dst;
    bit<48> src;
    bit<16> type;
    bit<16> index;
}
header fermat_mirror_h
{
    bit<48> dst;
    bit<48> src;
    bit<16> type;
    bit<16> index;
}
header fermat_h
{
    bit<16> index;
}
header clean_1_h
{
    bit<16> index;
}
header clean_2_h
{
    bit<16> index;
}
header output_clean_1_h
{
    bit<8> counter;
}
header output_clean_2_h
{
    bit<16> counter;
}
header output_layer_1_h
{
    bit<32> ID_1;
    bit<32> ID_2;
    bit<32> ID_3;
    bit<32> ID_4;
    bit<32> counter;

}
header output_layer_2_h
{
    bit<32> ID_1;
    bit<32> ID_2;
    bit<32> ID_3;
    bit<32> ID_4;
    bit<32> counter;

}
header output_layer_3_h
{
    bit<32> ID_1;
    bit<32> ID_2;
    bit<32> ID_3;
    bit<32> ID_4;
    bit<32> counter;

}
/*************************************************************************
 **************  I N G R E S S   P R O C E S S I N G   *******************
 *************************************************************************/
 
    /***********************  H E A D E R S  ************************/

struct my_ingress_headers_t {
    ethernet_h         ethernet;
    fermat_h           fermat;
    clean_1_h          clean_1;
    clean_2_h          clean_2;
    arp_h              arp;
    vlan_tag_h[2]      vlan_tag;
    ipv4_h             ipv4;
    bridge_h           bridge;
    icmp_h             icmp;
    igmp_h             igmp;
    tcp_h              tcp;
    udp_h              udp;
    output_clean_1_h   output_clean_1;
    output_clean_2_h   output_clean_2;
    output_layer_1_h   output_layer_1;
    output_layer_2_h   output_layer_2;
    output_layer_3_h   output_layer_3;

    
}


    /******  G L O B A L   I N G R E S S   M E T A D A T A  *********/


struct my_ingress_metadata_t {
    clean_mirror_h clean_mirror;
    bit<2> type;
    bit<8> layer_1_filter_counter;
    bit<16> layer_2_filter_counter;
    bit<1> layer_1_permission;
    bit<1> layer_2_permission;
    bit<1> layer_3_permission;
    bit<1> ecmp;
    bit<1> in_flag;
    bit<2> type_flag_1;
    bit<2> type_flag_2;
    bit<32> ll;
    bit<1> tstamp;
    bit<32> P;   
    bit<16> index_1;
    bit<16> index_2;
    bit<16> index_3;
    bit<16> re_index_1;
    bit<16> re_index_2;
    bit<16> re_index_3;
    bit<16> filter_index_1;
    bit<16> filter_index_2;
    bit<32> dif_1;
    bit<32> dif_2;
    bit<32> dif_3;
    bit<32> dif_4;
    bit<32> counter;
    bit<16> drop_hash;
    bit<16> drop_up;
    bit<1> ctrl;
    MirrorId_t session_id;
}


    /***********************  P A R S E R  **************************/

parser IngressParser(packet_in        pkt,
    /* User */
    out my_ingress_headers_t          hdr,
    out my_ingress_metadata_t         meta,
    /* Intrinsic */
    out ingress_intrinsic_metadata_t  ig_intr_md)
{
    /* This is a mandatory state, required by Tofino Architecture */
    state start {
        pkt.extract(ig_intr_md);
        pkt.advance(PORT_METADATA_SIZE);
        transition meta_init;
    }

    state meta_init {
        meta.ctrl = 0;
        hdr.bridge.ID_1 = 0;
        hdr.bridge.ID_2 = 0;
        hdr.bridge.ID_3 = 0;
        hdr.bridge.ID_4 = 0;
        meta.layer_1_permission = 0;
        meta.layer_2_permission = 0;
        meta.layer_3_permission = 0;
        meta.P = PRIME;
        meta.counter = 0;
        transition parse_ethernet;
    }
    
    state parse_ethernet {
        pkt.extract(hdr.ethernet);
        /* 
         * The explicit cast allows us to use ternary matching on
         * serializable enum
         */        
        transition select((bit<16>)hdr.ethernet.ether_type) {
            (bit<16>)ether_type_t.TPID &&& 0xEFFF :  parse_vlan_tag;
            (bit<16>)ether_type_t.IPV4            :  parse_ipv4;
            (bit<16>)ether_type_t.ARP             :  parse_arp;
            (bit<16>)ether_type_t.FERMAT          :  parse_fermat;
            (bit<16>)ether_type_t.FERMAT_NEW      :  parse_fermat;
            (bit<16>)ether_type_t.CLEAN_1          :  parse_clean_1;
            (bit<16>)ether_type_t.CLEAN_2          :  parse_clean_2;
            (bit<16>)ether_type_t.CTRL          :  parse_ctrl;
            default                                :accept;
        }
    }
    state parse_ctrl {
        meta.ctrl = 1;
        transition accept;
    }
    state parse_arp {
        pkt.extract(hdr.arp);
        transition accept;
    }
    state parse_fermat{
        pkt.extract(hdr.fermat);
        meta.index_1 = hdr.fermat.index;
        meta.index_2 = hdr.fermat.index;
        meta.index_3 = hdr.fermat.index;
        hdr.output_layer_1.setValid();
        hdr.output_layer_2.setValid();
        hdr.output_layer_3.setValid();
        hdr.bridge.setValid();
        transition accept;
    }
    state parse_clean_1
    {
        pkt.extract(hdr.clean_1);
        hdr.output_clean_1.setValid();
        meta.filter_index_1 = hdr.clean_1.index;
        transition accept;
    }
    state parse_clean_2
    {
        pkt.extract(hdr.clean_2);
        hdr.output_clean_2.setValid();
        meta.filter_index_2 = hdr.clean_2.index;
        transition accept;
    }
    state parse_vlan_tag {
        pkt.extract(hdr.vlan_tag.next);
        transition select(hdr.vlan_tag.last.ether_type) {
            ether_type_t.TPID :  parse_vlan_tag;
            ether_type_t.IPV4 :  parse_ipv4;
            default: accept;
        }
    }

    state parse_ipv4 {
        pkt.extract(hdr.ipv4);
        meta.ll = pkt.lookahead<bit<32>>();
        transition select(hdr.ipv4.protocol) {
            1 : parse_icmp;
            2 : parse_igmp;
            6 : parse_tcp;
           17 : parse_udp;
            default : accept;
        }
    }


    state parse_icmp {
       
        pkt.extract(hdr.icmp);
        transition accept;
    }
    
    state parse_igmp {
     
        pkt.extract(hdr.igmp);
        transition accept;
    }
    
    state parse_udp {
        
        pkt.extract(hdr.udp);
        hdr.bridge.setValid();
        hdr.bridge.ID_4 = 17;
        transition accept;
    }
    
    state parse_tcp {
     
        pkt.extract(hdr.tcp);
        transition accept;
    }


}

@pa_no_overlay("ingress","hdr.ethernet.ether_type")
@pa_no_overlay("egress","hdr.ethernet.ether_type")
@pa_no_overlay("ingress","meta.session_id")
@pa_no_overlay("ingress","meta.in_flag")
@pa_no_overlay("ingress","hdr.fermat.index")
@pa_atomic("ingress","meta.layer_1_permission")
@pa_atomic("ingress","meta.layer_2_permission")
@pa_atomic("ingress","meta.layer_3_permission")
@pa_atomic("ingress","hdr.fermat.index")
// @pa_atomic("ingress","hdr.output.counter")
// @pa_no_overlay("ingress","hdr.output.counter")
// @pa_no_overlay("ingress","hdr.output.ID_1")
// @pa_no_overlay("ingress","hdr.output.ID_2")
// @pa_no_overlay("ingress","hdr.output.ID_3")
// @pa_no_overlay("ingress","hdr.output.ID_4")
@pa_no_overlay("ingress","hdr.bridge.index_1")
@pa_no_overlay("ingress","hdr.bridge.index_2")
@pa_no_overlay("ingress","hdr.bridge.index_3")

@pa_no_overlay("ingress","hdr.bridge.counter")
@pa_no_overlay("ingress","hdr.bridge.ID_1")
@pa_no_overlay("ingress","hdr.bridge.ID_2")
@pa_no_overlay("ingress","hdr.bridge.ID_3")
@pa_no_overlay("ingress","hdr.bridge.ID_4")
 @pa_no_overlay("ingress","meta.layer_1_filter_counter")
  @pa_no_overlay("ingress","meta.layer_2_filter_counter")
   @pa_solitary("ingress","meta.layer_1_filter_counter")
  @pa_solitary("ingress","meta.layer_2_filter_counter")
    @pa_container_size("ingress","meta.layer_2_filter_counter",16)
    @pa_container_size("ingress","meta.layer_1_filter_counter",8)
// @pa_no_overlay("ingress","hdr.bridge.dif_1")
// @pa_no_overlay("ingress","hdr.bridge.dif_2")
// @pa_no_overlay("ingress","hdr.bridge.dif_3")
// @pa_no_overlay("ingress","hdr.bridge.dif_4")

// @pa_atomic("egress","hdr.fermat.index")
// @pa_no_overlay("egress","hdr.fermat.index")
// @pa_no_overlay("egress","hdr.fermat.layer")
// // @pa_no_overlay("egress","hdr.output.counter")
// // @pa_no_overlay("egress","hdr.output.ID_1")
// // @pa_no_overlay("egress","hdr.output.ID_2")
// // @pa_no_overlay("egress","hdr.output.ID_3")
// // @pa_no_overlay("egress","hdr.output.ID_4")
// @pa_no_overlay("egress","hdr.bridge.counter")
// @pa_no_overlay("egress","hdr.bridge.ID_1")
// @pa_no_overlay("egress","hdr.bridge.ID_2")
// @pa_no_overlay("egress","hdr.bridge.ID_3")
// @pa_no_overlay("egress","hdr.bridge.ID_4")
// @pa_no_overlay("egress","meta.fermat_mirror.layer")
// @pa_no_overlay("egress","meta.fermat_mirror.index")
// @pa_no_overlay("egress","meta.fermat_mirror.output_or_clean")
// @pa_no_overlay("egress","hdr.bridge.index_1")
// @pa_no_overlay("egress","hdr.bridge.index_2")
// @pa_no_overlay("egress","hdr.bridge.index_3")
control Ingress(/* User */
    inout my_ingress_headers_t                       hdr,
    inout my_ingress_metadata_t                      meta,
    /* Intrinsic */
    in    ingress_intrinsic_metadata_t               ig_intr_md,
    in    ingress_intrinsic_metadata_from_parser_t   ig_prsr_md,
    inout ingress_intrinsic_metadata_for_deparser_t  ig_dprsr_md,
    inout ingress_intrinsic_metadata_for_tm_t        ig_tm_md)
{





//bit<32> errorcode=0;
    CRCPolynomial<bit<32>>(0x04C11DB7,false,false,false,32w0xFFFFFFFF,32w0xFFFFFFFF) crc32a;
    CRCPolynomial<bit<32>>(0x741B8CD7,false,false,false,32w0xFFFFFFFF,32w0xFFFFFFFF) crc32b;
    CRCPolynomial<bit<32>>(0xDB710641,false,false,false,32w0xFFFFFFFF,32w0xFFFFFFFF) crc32c;
    CRCPolynomial<bit<32>>(0x82608EDB,false,false,false,32w0xFFFFFFFF,32w0xFFFFFFFF) crc32fp;
    CRCPolynomial<bit<32>>(0x12345678,false,false,false,32w0xFFFFFFFF,32w0xFFFFFFFF) crc32_ecmp;
    CRCPolynomial<bit<32>>(0x11111111,false,false,false,32w0xFFFFFFFF,32w0xFFFFFFFF) crc32_f2;
    CRCPolynomial<bit<32>>(0x04C11DB7,false,false,false,32w0xFFFFFFFF,32w0xFFFFFFFF) crc32_f1;
    CRCPolynomial<bit<32>>(0xC526B931,false,false,false,32w0xFFFFFFFF,32w0xFFFFFFFF) crc32_drop;

    Hash<bit<16>>(HashAlgorithm_t.CUSTOM,crc32a) hash_1;
    Hash<bit<16>>(HashAlgorithm_t.CUSTOM,crc32b) hash_2;
    Hash<bit<16>>(HashAlgorithm_t.CUSTOM,crc32c) hash_3;
    Hash<bit<1>>(HashAlgorithm_t.CUSTOM,crc32_ecmp) hash_ecmp;
    Hash<bit<20>>(HashAlgorithm_t.CUSTOM,crc32fp) hash_fp;
    Hash<bit<15>>(HashAlgorithm_t.CUSTOM,crc32_f1) hash_filter_1;
    Hash<bit<14>>(HashAlgorithm_t.CUSTOM,crc32_f2) hash_filter_2;
    Hash<bit<16>>(HashAlgorithm_t.CUSTOM,crc32_drop) hash_drop;



//transmission block
    action unicast_send(PortId_t port) {
        ig_tm_md.ucast_egress_port = port;
        hdr.ipv4.ttl = hdr.ipv4.ttl - 1;
    }
    action drop() {
        ig_dprsr_md.drop_ctl = 1;
    }
    @stage(0) table arp_host {
        key = { hdr.arp.proto_dst_addr : exact; }
        actions = { unicast_send; drop; }
        default_action = drop();
    }
    action cal_ecmp_a()
    {
        meta.ecmp = hash_ecmp.get({hdr.ipv4.src_addr,hdr.ipv4.dst_addr,meta.ll,hdr.ipv4.protocol});
        
    }
    @stage(0) table cal_ecmp_t {
        actions = { cal_ecmp_a; }
        default_action = cal_ecmp_a;
    }
    @stage(1) table ipv4_host {
        key = { hdr.ipv4.dst_addr : exact; meta.ecmp : exact;}
        actions = { unicast_send; drop;}
        default_action = drop;
    }
//transmission block

// local timestamp 
    Register<bit<1>, bit<16>>(1) ts_reg;
    RegisterAction<bit<1>, bit<16>, bit<1>>(ts_reg) get_ts=
    {
        void apply(inout bit<1> register_data, out bit<1> result) {
        	
            result = register_data;
        }
    };
    action get_ts_a()
    {
        meta.tstamp = get_ts.execute(0);
    }
    @stage(0) table get_ts_t {
        actions = { get_ts_a;}
        default_action = get_ts_a;
    }

    action get_drop_a(bit<16> up)
    {
        meta.drop_up = up;
    }
    @stage(0) table get_drop_t {
        actions = { get_drop_a;}
        default_action = get_drop_a(65535);
    }

    action cal_drop_a()
    {
        meta.drop_hash = hash_drop.get({hdr.ipv4.src_addr,hdr.ipv4.dst_addr,meta.ll,hdr.ipv4.protocol,meta.tstamp});
    }
    @stage(1) table cal_drop_t {
        actions = { cal_drop_a;}
        default_action = cal_drop_a;
    }

    action cal_drop_dif_a()
    {
        meta.drop_hash = meta.drop_hash |-| meta.drop_up;
    }
    @stage(4) table cal_drop_dif_t {
        actions = { cal_drop_dif_a;}
        default_action = cal_drop_dif_a;
    }
    action get_remap_index_a(bit<16> mask)
    {
        meta.re_index_1 = hdr.bridge.index_1 & mask;
        meta.re_index_2 = hdr.bridge.index_2 & mask;
        meta.re_index_3 = hdr.bridge.index_3 & mask;
    }
    @stage(3) table get_remap_index_t {
        actions = { get_remap_index_a;}
        default_action = get_remap_index_a(0xffff);
    }
    
action get_map_index_a(bit<16> mask)
    {
        hdr.bridge.index_1 = hdr.bridge.index_1 & mask;
        hdr.bridge.index_2 = hdr.bridge.index_2 & mask;
        hdr.bridge.index_3 = hdr.bridge.index_3 & mask;
    }
    @stage(5) table get_map_index_t {
        key = {meta.type:exact;}
        actions = { get_map_index_a;}
        default_action = get_map_index_a(0xffff);
    }

    action fermat_new_a()
    {
        hdr.ethernet.ether_type = (bit<16>)ether_type_t.FERMAT;
        ig_tm_md.ucast_egress_port = RECIRCULATE_PORT_1;
    }

    @stage(10) table fermat_new_t {
        key = {hdr.ethernet.ether_type:exact;}
        actions = { fermat_new_a; NoAction;}
        default_action = NoAction;
        size = 8;
    }



    action direction_in_a()
    {
        meta.in_flag = 1;
        meta.counter = 1;
        hdr.ipv4.res = meta.tstamp;
        //hdr.output.setValid();
    }
    @stage(1) table direction_in_t {
        key = { ig_intr_md.ingress_port : exact; }
        size = 32;
        actions = { direction_in_a; NoAction;}
        default_action = NoAction;
    }

    
// local timestamp + direction

//index init
    
    CAL_HASH(1)
    CAL_HASH(2)
    CAL_HASH(3)
    CAL_FILTER_HASH(1,14)
    CAL_FILTER_HASH(2,13)
    MEM_ZONE_MAP_INDEX(1)
    MEM_ZONE_MAP_INDEX(2)
    MEM_ZONE_MAP_INDEX(3)
    
    MEM_ZONE_REMAP_INDEX(1)
    MEM_ZONE_REMAP_INDEX(2)
    MEM_ZONE_REMAP_INDEX(3)
    MAP_IN_HASH(4096)
    MAP_FILTER_HASH(1,32768)
    MAP_FILTER_HASH(2,16384)
    // action index_def_init_a()
    // {  
    //     hdr.bridge.index_1 = hdr.fermat.index;
    //     hdr.bridge.index_2 = hdr.fermat.index;
    //     hdr.bridge.index_3 = hdr.fermat.index;  
    //     meta.index_1 = hdr.fermat.index;
    //     meta.index_2 = hdr.fermat.index;
    //     meta.index_3 = hdr.fermat.index;
        
    // } 
    // @stage(3) table index_def_init_t 
    // {           
    //     actions = { index_def_init_a; }
    //     default_action = index_def_init_a; 
    // }
//index init

//ID init
    bit<1> a;
    bit<1> b;
    bit<1> c;
    action cal_ID_123_a()
    {  
        hdr.bridge.ID_1 = hdr.ipv4.src_addr & MASK_ID;    
        hdr.bridge.ID_2 = hdr.ipv4.dst_addr & MASK_ID;  
        hdr.bridge.ID_3 = meta.ll & MASK_ID;
    } 
    @stage(2) table cal_ID_123_t 
    {           
        actions = { cal_ID_123_a; }
        default_action = cal_ID_123_a; 
    }

    action cal_ID_4_a(bit<32> val)
    {  
        hdr.bridge.ID_4 = hdr.bridge.ID_4 + val;              
    } 
    @stage(2) table cal_ID_4_t 
    {   key = {hdr.ipv4.src_addr[31:31] : exact; hdr.ipv4.dst_addr[31:31] : exact; meta.ll[31:31] : exact; }
        actions = { cal_ID_4_a; }
        default_action = cal_ID_4_a(0); 
    } 
    action cal_fp_a()
    {  
        hdr.bridge.ID_4[30:11] = hash_fp.get({1w0,hdr.ipv4.src_addr[30:0],1w0,hdr.ipv4.dst_addr[30:0],1w0,meta.ll[30:0],5w0,hdr.ipv4.src_addr[31:31],hdr.ipv4.dst_addr[31:31],meta.ll[31:31],hdr.ipv4.protocol});              
    } 
    @stage(1) table cal_fp_t 
    {   
        actions = { cal_fp_a; }
        default_action = cal_fp_a; 
    } 
//ID init

//dif init
    CAL_DIF(3)
//dif init






//fermat







    Fermat_Ingress(1,1,8,8192)
    Fermat_Ingress(1,2,8,8192)
    Fermat_Ingress(1,3,8,8192)
    Counter_Ingress(1,8,8192)
    Fermat_Ingress(1,4,11,8192)

    Fermat_Ingress(2,1,9,8192)
    Fermat_Ingress(2,2,9,8192)
    Fermat_Ingress(2,3,11,8192)
    Counter_Ingress(2,9,8192)
    Fermat_Ingress(2,4,9,8192)
    

    Fermat_Ingress(3,1,10,8192)
    Fermat_Ingress(3,2,11,8192)
    Fermat_Ingress(3,3,10,8192)
    Counter_Ingress(3,10,8192)
    Fermat_Ingress(3,4,10,8192)

    
    
    




//fermat

//filter
    Filter(1,8,2,65536,255)
    Filter(2,16,2,32768,65535)

    action clean_continue_1_a()
    {  
        hdr.clean_1.index = hdr.clean_1.index + 1;
        ig_tm_md.ucast_egress_port = RECIRCULATE_PORT_1;//recirculate??
        
    }
    action clean_stop_1_a()
    {   
        ig_tm_md.ucast_egress_port = OUTPUT_PORT;//output??
    }
    action clean_mirror_1_a()
    {   meta.clean_mirror.index = hdr.clean_1.index + 1;
        meta.clean_mirror.type = 0x2221;
        meta.clean_mirror.src = SWITCH_ID;
        ig_tm_md.ucast_egress_port = OUTPUT_PORT;//output??
        meta.session_id = 1;
        ig_dprsr_md.mirror_type = 1;
    }  
    @stage(6) table clean_continue_1_t 
    {   key = {hdr.clean_1.index:exact;}
        actions = { clean_continue_1_a; clean_stop_1_a; clean_mirror_1_a;}
        default_action = clean_continue_1_a; 
    }
    

    action clean_continue_2_a()
    {  
        hdr.clean_2.index = hdr.clean_2.index + 1;
        ig_tm_md.ucast_egress_port = RECIRCULATE_PORT_1;//recirculate??
    }
    action clean_stop_2_a()
    {  
        ig_tm_md.ucast_egress_port = OUTPUT_PORT;//output??
    }
    action clean_mirror_2_a()
    {   meta.clean_mirror.index = hdr.clean_2.index + 1;
        meta.clean_mirror.type = 0x2222;
        meta.clean_mirror.src = SWITCH_ID;
        ig_tm_md.ucast_egress_port = OUTPUT_PORT;//output??
        meta.session_id = 1;
        ig_dprsr_md.mirror_type = 1;
    }  
    @stage(6) table clean_continue_2_t 
    {   key = {hdr.clean_2.index:exact;}
        actions = { clean_continue_2_a; clean_stop_2_a; clean_mirror_2_a;}
        default_action = clean_continue_2_a; 
    }

    action filter_1_range_map_a(bit<2> type)
    {
        meta.type_flag_1 = type;
    }
    @stage(3) table filter_1_range_map_t {
        key = {meta.layer_1_filter_counter:range;}
        actions = { filter_1_range_map_a;}
        default_action = filter_1_range_map_a(0);
        size = 128;
    }

    action filter_2_range_map_a(bit<2> type)
    {
        meta.type_flag_2 = type;
    }
    @stage(3) table filter_2_range_map_t {
        key = {meta.layer_2_filter_counter:range;}
        actions = { filter_2_range_map_a;}
        default_action = filter_2_range_map_a(0);
        size = 128;
    }

    action type_def_a()
    {
        meta.type = min(meta.type_flag_1, meta.type_flag_2);
    }
    @stage(4) table type_def_t {
        actions = { type_def_a;}
        default_action = type_def_a;
    }
    action type_set_a()
    {
        meta.type = hdr.ipv4.type;
    }
    @stage(4) table type_set_t {
        actions = { type_set_a;}
        default_action = type_set_a;
    }

    

    // action mem_zone_set_index_a()
    // {
    //     hdr.bridge.index_1 = hdr.bridge.index_1 - meta.offset_1;
    //     hdr.bridge.index_2 = hdr.bridge.index_2 - meta.offset_2;
    //     hdr.bridge.index_3 = hdr.bridge.index_3 - meta.offset_3;
    //     // meta.re_index_1 = meta.re_index_1 - meta.re_offset_1;
    //     // meta.re_index_2 = meta.re_index_2 - meta.re_offset_2;
    //     // meta.re_index_3 = meta.re_index_3 - meta.re_offset_3;
    // }
    // @stage(6) table mem_zone_set_index_t {
    //     actions = { mem_zone_set_index_a;}
    //     default_action = mem_zone_set_index_a;
    // }

    // action mem_zone_cal_index_a()
    // {
    //     meta.re_offset_1 = meta.re_offset_1 - meta.offset_1;
    //     meta.re_offset_2 = meta.re_offset_2 - meta.offset_2;
    //     meta.re_offset_3 = meta.re_offset_3 - meta.offset_3;
    // }
    // @stage(8) table mem_zone_cal_index_t {
    //     actions = { mem_zone_cal_index_a;}
    //     default_action = mem_zone_cal_index_a;
    // }


    action mem_zone_reset_index_a()
    {
        hdr.bridge.index_1 = meta.re_index_1;
        hdr.bridge.index_2 = meta.re_index_2;
        hdr.bridge.index_3 = meta.re_index_3;
    }
    @stage(7) table mem_zone_reset_index_t {
        actions = { mem_zone_reset_index_a;}
        default_action = mem_zone_reset_index_a;
    }
    action clean_set_a()
    {
        hdr.output_clean_1.counter = meta.layer_1_filter_counter;
        hdr.output_clean_2.counter = meta.layer_2_filter_counter;
    }
    @stage(11) table clean_set_t {
        actions = { clean_set_a;}
        default_action = clean_set_a;
    }

//filter

//fer


    // action invalidate_output_a()
    // {
    //     hdr.output.setInvalid();
    // }
    // @stage(11) table invalidate_output_t {
    //     actions = { invalidate_output_a; }
    //     default_action = invalidate_output_a;
    // }
    
    

    action set_permission_1_a()
    {
        meta.layer_1_permission = 1;
    }
    @stage(8) table set_permission_1_t {
        actions = { set_permission_1_a; }
        default_action = set_permission_1_a;
    }
    action set_permission_2_a()
    {
        meta.layer_2_permission = 1;
    }
    @stage(9) table set_permission_2_t {
        actions = { set_permission_2_a; }
        default_action = set_permission_2_a;
    }
    action set_permission_3_a()
    {
        meta.layer_3_permission = 1;
    }
    @stage(10) table set_permission_3_t {
        actions = { set_permission_3_a; }
        default_action = set_permission_3_a;
    }


    action fermat_set_a()
    {
        hdr.bridge.ID_1 = 0;
        hdr.bridge.ID_2 = 0;
        hdr.bridge.ID_3 = 0;
        hdr.bridge.ID_4 = 0;
        ig_tm_md.ucast_egress_port = RECIRCULATE_PORT_1;
    }
    @stage(2) table fermat_set_t {
        actions = { fermat_set_a; }
        default_action = fermat_set_a;
    }
    /*action meta_copy_a()
    {  
        hdr.output.ID_1 = meta.copy.ID_1;
        hdr.output.ID_2 = meta.copy.ID_2;
        hdr.output.ID_3 = meta.copy.ID_3;
        hdr.output.ID_4 = meta.copy.ID_4;
        hdr.output.counter = meta.copy.counter;
    } 
    @stage(10) table meta_copy_t 
    {           
        actions = { meta_copy_a; }
        default_action = meta_copy_a; 
    }      */
    apply{
    //transmission block 0-1
        if (hdr.arp.isValid())
        {
            arp_host.apply();
        }
        else if (hdr.ipv4.isValid())
        {   
            cal_ecmp_t.apply();
            ipv4_host.apply();
            //ttl=0? acl?
        }
        else if (hdr.ethernet.ether_type == (bit<16>)ether_type_t.FERMAT_END)
        {
            ig_tm_md.ucast_egress_port = OUTPUT_PORT;
        }
        else if (meta.ctrl == 1)
        {
            ig_tm_md.ucast_egress_port = CPU_PORT;
        }
    //transmission block
     //filter init
    // local timestamp + direction 0
        get_ts_t.apply();
        //get_index_t.apply();
        if (hdr.udp.isValid())
        {
            direction_in_t.apply();
        }
    // local timestamp + direction

    //index init 0-1
    
        cal_hash_1_t.apply();
        cal_hash_2_t.apply();
        cal_hash_3_t.apply();
        cal_filter_hash_1_t.apply();
        cal_filter_hash_2_t.apply();
        get_drop_t.apply();
        cal_drop_t.apply();
        cal_drop_dif_t.apply();
        /*if (hdr.fermat.isValid())
        {
            //ig_tm_md.ucast_egress_port = 68;
            index_def_init_t.apply();
        }
        else
        {*/
            
        //}
        if (hdr.clean_1.isValid())
        {
            meta.filter_index_1 = hdr.clean_1.index;
        }
        else if (hdr.clean_2.isValid())
        {
            meta.filter_index_2 = hdr.clean_2.index;
        }
        else
        {
            map_filter_hash_1_t.apply();
            map_filter_hash_2_t.apply();
        }
        
        
    //index init

   
    //filter init

    //ID init 0-1
        // if (hdr.fermat.isValid()) 
        // {
        //     ID_def_init_t.apply();
        //     //bit<32> f = 1;
        // }
        // else
        // {
            cal_fp_t.apply();
            if (hdr.fermat.isValid())
            {
                fermat_set_t.apply();
            }
            else 
            {
                cal_ID_123_t.apply();
                cal_ID_4_t.apply();
            }
            get_remap_index_t.apply();
            
            mem_zone_remap_index_1_t.apply();
            mem_zone_remap_index_2_t.apply();
            mem_zone_remap_index_3_t.apply();
        //}
    //ID init 0-1

    //dif init 2
        // if (hdr.fermat.isValid())
        // {
        //     meta.index_1 = hdr.fermat.index;
        //     meta.index_2 = hdr.fermat.index;
        //     meta.index_3 = hdr.fermat.index;
        // }
        cal_DIF_t.apply();
    //dif init 1
    //workflow for inbound flow
        //filter_insert/clean + decouple large/small flow
        if (meta.in_flag == 1 || hdr.clean_1.isValid()) //?? clean header may be
        {
            insert_layer_1_filter_t.apply(); 
        }
        if (meta.in_flag == 1 || hdr.clean_2.isValid()) //?? clean header may be
        {
            insert_layer_2_filter_t.apply();  
        }
        
        clean_set_t.apply();
        //filter_insert + decouple large/small flow
        if (meta.in_flag == 1)
        {
            filter_1_range_map_t.apply();
            filter_2_range_map_t.apply();
        }
        if (meta.in_flag == 1)
        {
            type_def_t.apply();
        }
        else
        {
            type_set_t.apply();
        }
        get_map_index_t.apply();
        mem_zone_map_index_1_t.apply();
        mem_zone_map_index_2_t.apply();
        mem_zone_map_index_3_t.apply();
        //if (meta.type == 3)
        //{
            
            //mem_zone_reset_index_t.apply();
        //}
        //mem_zone_set_index_t.apply();
        if (meta.in_flag == 1)
        {
            if (meta.type != 1 || meta.drop_hash == 0)
            map_in_hash_t.apply();
            else
            hdr.ipv4.type = 0;
        }
        if (meta.type == 3)
        {
            mem_zone_reset_index_t.apply();
        }
        
        // //large_flow_reduce_t.apply();
        // // fermat insert
        if ((meta.in_flag == 1 &&hdr.ipv4.type != 0)|| hdr.fermat.isValid())
        {   
            
            insert_layer_1_part_1_t.apply();
            insert_layer_1_part_2_t.apply();
            insert_layer_1_part_3_t.apply();
            insert_layer_1_counter_t.apply();
            set_permission_1_t.apply();
            //insert_layer_1_counter_t.apply();
        }
        if ((meta.in_flag == 1 &&hdr.ipv4.type != 0)|| hdr.fermat.isValid())
        {
            insert_layer_2_part_1_t.apply();
            insert_layer_2_part_2_t.apply();
            insert_layer_2_counter_t.apply();
            insert_layer_2_part_4_t.apply();
            set_permission_2_t.apply();
            //insert_layer_2_counter_t.apply();
        }
        if ((meta.in_flag == 1 &&hdr.ipv4.type != 0)|| hdr.fermat.isValid())
        {
            insert_layer_3_part_1_t.apply();
            insert_layer_3_counter_t.apply();
            insert_layer_3_part_3_t.apply();
            insert_layer_3_part_4_t.apply();
            set_permission_3_t.apply();
            //insert_layer_3_counter_t.apply();
        }
        // if (meta.type == 3)
        // {
        //     mem_zone_cal_index_t.apply();
        // }
        if (meta.layer_1_permission == 1)
        {
            insert_layer_1_part_4_t.apply();
        }
        if (meta.layer_2_permission == 1)
        {
            insert_layer_2_part_3_t.apply();
        }
        if (meta.layer_3_permission == 1)
        {
            insert_layer_3_part_2_t.apply();
        }
        
        if (hdr.clean_1.isValid())
        {   
            clean_continue_1_t.apply();
        }
        else if (hdr.clean_2.isValid())
        {   
            clean_continue_2_t.apply();
        }
        fermat_new_t.apply();
if (hdr.ipv4.ecn ==1)
ig_dprsr_md.drop_ctl = 1;
        // fermat insert
    //workflow for inbound flow
    }
}






control IngressDeparser(packet_out pkt,
    /* User */
    inout my_ingress_headers_t                       hdr,
    in    my_ingress_metadata_t                      meta,
    /* Intrinsic */
    in    ingress_intrinsic_metadata_for_deparser_t  ig_dprsr_md)
{
    Checksum() ipv4_checksum;
    Mirror() mirror;
    apply {
        if (hdr.ipv4.isValid()) {
            hdr.ipv4.hdr_checksum = ipv4_checksum.update({
                hdr.ipv4.version,
                hdr.ipv4.ihl,
                hdr.ipv4.res,
                hdr.ipv4.type,
                hdr.ipv4.dscp,
                hdr.ipv4.ecn,
                hdr.ipv4.total_len,
                hdr.ipv4.identification,
                hdr.ipv4.flags,
                hdr.ipv4.frag_offset,
                hdr.ipv4.ttl,
                hdr.ipv4.protocol,
                hdr.ipv4.src_addr,
                hdr.ipv4.dst_addr
            });  
        }


        if (ig_dprsr_md.mirror_type == 1)
        mirror.emit<clean_mirror_h>(meta.session_id, meta.clean_mirror);
        
        
        pkt.emit(hdr);
    }
}
/*************************************************************************
 ****************  E G R E S S   P R O C E S S I N G   *******************
 *************************************************************************/

    /***********************  H E A D E R S  ************************/


struct my_egress_headers_t {

    ethernet_h         ethernet;
    fermat_h           fermat;
    arp_h              arp;
    vlan_tag_h[2]      vlan_tag;
    ipv4_h             ipv4;
    bridge_h           bridge;
    output_layer_1_h   output_layer_1;
    output_layer_2_h   output_layer_2;
    output_layer_3_h   output_layer_3;

}



    /********  G L O B A L   E G R E S S   M E T A D A T A  *********/

struct my_egress_metadata_t {
fermat_mirror_h fermat_mirror;
bit<1> validation;
bit<1> fermat_new_bit;
bit<1> out_flag;
bit<1> layer_1_permission;
bit<1> layer_2_permission;
bit<1> layer_3_permission;
bit<1> fermat_e;
bit<16> index_1;
bit<16> index_2;
bit<16> index_3;
bit<32> dif_1;
bit<32> dif_2;
bit<32> dif_3;
bit<32> dif_4;
bit<32> P;
bit<32> counter;
MirrorId_t session_id;
}

    /***********************  P A R S E R  **************************/

parser EgressParser(packet_in        pkt,
    /* User */
    out my_egress_headers_t          hdr,
    out my_egress_metadata_t         meta,
    /* Intrinsic */
    out egress_intrinsic_metadata_t  eg_intr_md)
{
    /* This is a mandatory state, required by Tofino Architecture */
    state start {
        pkt.extract(eg_intr_md);
        meta.P = PRIME;
        // pkt.advance(PORT_METADATA_SIZE);
        transition parse_ethernet;
    }

    state parse_ethernet{
        pkt.extract(hdr.ethernet);
        transition select((bit<16>)hdr.ethernet.ether_type) {
            (bit<16>)ether_type_t.TPID &&& 0xEFFF :  parse_vlan_tag;
            (bit<16>)ether_type_t.IPV4            :  parse_ipv4;
            (bit<16>)ether_type_t.ARP             :  parse_arp;
            (bit<16>)ether_type_t.FERMAT          :  parse_fermat;
            default :  accept;
        }       
    }

    state parse_fermat{
        pkt.extract(hdr.fermat);
        meta.index_1 = hdr.fermat.index;
        meta.index_2 = hdr.fermat.index;
        meta.index_3 = hdr.fermat.index;
        meta.counter = 0;
        hdr.output_layer_1.setValid();
        hdr.output_layer_2.setValid();
        hdr.output_layer_3.setValid();
        transition parse_bridge;
    }

    state parse_arp {
        pkt.extract(hdr.arp);
        transition  accept;
    }

    state parse_vlan_tag {
        pkt.extract(hdr.vlan_tag.next);
        transition select(hdr.vlan_tag.last.ether_type) {
            ether_type_t.TPID :  parse_vlan_tag;
            ether_type_t.IPV4 :  parse_ipv4;
            default: accept;
        }    
    }

    state parse_ipv4 {
        pkt.extract(hdr.ipv4);
        transition select(hdr.ipv4.protocol) {
            17 : parse_bridge;
            default : accept;
        }
    }    
    state parse_bridge {
        pkt.extract(hdr.bridge);
        transition  accept;
        
    }
}

    /***************** M A T C H - A C T I O N  *********************/

control Egress(
    /* User */
    inout my_egress_headers_t                          hdr,
    inout my_egress_metadata_t                         meta,
    /* Intrinsic */    
    in    egress_intrinsic_metadata_t                  eg_intr_md,
    in    egress_intrinsic_metadata_from_parser_t      eg_prsr_md,
    inout egress_intrinsic_metadata_for_deparser_t     eg_dprsr_md,
    inout egress_intrinsic_metadata_for_output_port_t  eg_oport_md)
{
    CAL_DIF(2)
    action direction_out_a()
    {
        meta.out_flag = 1;
        meta.counter = 1;
    }
    @stage(2) table direction_out_t {
        key = { eg_intr_md.egress_port : exact; }
        size = 32;
        actions = { direction_out_a; NoAction;}
        default_action = NoAction;
    }
    action invalidate_bridge_a()
    {
        hdr.bridge.setInvalid();
    }
    @stage(11) table invalidate_bridge_t {
        actions = { invalidate_bridge_a; }
        default_action = invalidate_bridge_a;
    }


    action invalidate_output_a()
    {
        hdr.output_layer_1.setInvalid();
        hdr.output_layer_2.setInvalid();
        hdr.output_layer_3.setInvalid();
    }
    @stage(9) table invalidate_output_t {
        actions = { invalidate_output_a; }
        default_action = invalidate_output_a;
    }

 
    action fermat_continue_a()
    {
        hdr.fermat.index = hdr.fermat.index + 1;
    }

    action fermat_mirror_a()
    {
        hdr.ethernet.ether_type = (bit<16>)ether_type_t.FERMAT_END;
        meta.fermat_mirror.type = (bit<16>)ether_type_t.FERMAT_NEW;
        meta.fermat_mirror.src = SWITCH_ID;
        meta.session_id = 2;
        meta.fermat_mirror.index = hdr.fermat.index + 1;
        eg_dprsr_md.mirror_type = 2;
    }
    action fermat_stop_a()
    {
        hdr.ethernet.ether_type = (bit<16>)ether_type_t.FERMAT_END;
    }
    @stage(11) table fermat_continue_t {
        key = {hdr.fermat.index:exact;}
        actions = { fermat_continue_a; fermat_mirror_a; fermat_stop_a;}
        default_action = fermat_continue_a;
    }

    
    MAP_OUT_HASH(3072)
    Fermat_Egress(1,1,4,6144)
    Fermat_Egress(1,2,4,6144)
    Fermat_Egress(1,3,4,6144)
    Fermat_Egress(1,4,7,6144)

    Fermat_Egress(2,1,5,6144)
    Fermat_Egress(2,2,5,6144)
    Fermat_Egress(2,3,7,6144)
    Fermat_Egress(2,4,5,6144)

    Fermat_Egress(3,1,6,6144)
    Fermat_Egress(3,2,7,6144)
    Fermat_Egress(3,3,6,6144)
    Fermat_Egress(3,4,6,6144)

    Counter_Egress(1,4,6144)
    Counter_Egress(2,5,6144)
    Counter_Egress(3,6,6144)


    action set_permission_1_a()
    {
        meta.layer_1_permission = 1;
    }
    @stage(4) table set_permission_1_t {
        actions = { set_permission_1_a; }
        default_action = set_permission_1_a;
    }
    action set_permission_2_a()
    {
        meta.layer_2_permission = 1;
    }
    @stage(5) table set_permission_2_t {
        actions = { set_permission_2_a; }
        default_action = set_permission_2_a;
    }
    action set_permission_3_a()
    {
        meta.layer_3_permission = 1;
    }
    @stage(6) table set_permission_3_t {
        actions = { set_permission_3_a; }
        default_action = set_permission_3_a;
    }


    action index_dec_a()
    {
        meta.index_1 = meta.index_1 - 1024;
        meta.index_2 = meta.index_2 - 1024;
        meta.index_3 = meta.index_3 - 1024;
    }
    @stage(3) table index_dec_t {
        actions = { index_dec_a; }
        default_action = index_dec_a;
    }
    apply {
        cal_DIF_t.apply();
        if (hdr.ipv4.protocol == 17)
        {   
            direction_out_t.apply();
            map_out_hash_t.apply();
        }
        else if (hdr.fermat.index[12:12] == 1)
        {
            index_dec_t.apply();
        }
        if ((meta.out_flag == 1 && hdr.ipv4.type !=0)|| hdr.fermat.isValid())
        {   
            
            insert_layer_1_part_1_t.apply();
            insert_layer_1_part_2_t.apply();
            insert_layer_1_part_3_t.apply();
            insert_layer_1_counter_t.apply();
            set_permission_1_t.apply();
            //insert_layer_1_counter_t.apply();
        }
        if ((meta.out_flag == 1 && hdr.ipv4.type !=0) || hdr.fermat.isValid())
        {
            insert_layer_2_part_1_t.apply();
            insert_layer_2_part_2_t.apply();
            insert_layer_2_counter_t.apply();
            insert_layer_2_part_4_t.apply();
            set_permission_2_t.apply();
            //insert_layer_2_counter_t.apply();
        }
        if ((meta.out_flag == 1 && hdr.ipv4.type !=0) || hdr.fermat.isValid())
        {
            insert_layer_3_part_1_t.apply();
            insert_layer_3_counter_t.apply();
            insert_layer_3_part_3_t.apply();
            insert_layer_3_part_4_t.apply();
            set_permission_3_t.apply();
            //insert_layer_3_counter_t.apply();
        }
        if (meta.layer_1_permission == 1)
        {
            insert_layer_1_part_4_t.apply();
        }
        if (meta.layer_2_permission == 1)
        {
            insert_layer_2_part_3_t.apply();
        }
        if (meta.layer_3_permission == 1)
        {
            insert_layer_3_part_2_t.apply();
        }
        if (hdr.fermat.index[11:10] == 3)
        {
            invalidate_output_t.apply();
        }
        fermat_continue_t.apply();
        invalidate_bridge_t.apply();
        //fermat_new_t.apply();
        


    }
}



























    /*********************  D E P A R S E R  ************************/

control EgressDeparser(packet_out pkt,
    /* User */
    inout my_egress_headers_t                       hdr,
    in    my_egress_metadata_t                      meta,
    /* Intrinsic */
    in    egress_intrinsic_metadata_for_deparser_t  eg_dprsr_md)
{
    Checksum() ipv4_checksum;
    Mirror() mirror;
    apply {
        if (hdr.ipv4.isValid()) {
            hdr.ipv4.hdr_checksum = ipv4_checksum.update({
                hdr.ipv4.version,
                hdr.ipv4.ihl,
                hdr.ipv4.res,
                hdr.ipv4.type,
                hdr.ipv4.dscp,
                hdr.ipv4.ecn,
                hdr.ipv4.total_len,
                hdr.ipv4.identification,
                hdr.ipv4.flags,
                hdr.ipv4.frag_offset,
                hdr.ipv4.ttl,
                hdr.ipv4.protocol,
                hdr.ipv4.src_addr,
                hdr.ipv4.dst_addr
            });  
        }
        if (eg_dprsr_md.mirror_type == 2)
        mirror.emit<fermat_mirror_h>(meta.session_id, meta.fermat_mirror);
        pkt.emit(hdr);
    }
}


/************ F I N A L   P A C K A G E ******************************/
Pipeline(
    IngressParser(),
    Ingress(),
    IngressDeparser(),
    EgressParser(),
    Egress(),
    EgressDeparser()
) pipe;


Switch(pipe) main;
