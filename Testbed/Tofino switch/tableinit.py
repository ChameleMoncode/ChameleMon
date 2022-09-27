from ipaddress import ip_address
table = bfrt.mirror.cfg
table.add_with_normal(sid=1,session_enable=True,ucast_egress_port=196 ,ucast_egress_port_valid=True,direction="INGRESS",max_pkt_len=64)
table.add_with_normal(sid=2,session_enable=True,ucast_egress_port=196 ,ucast_egress_port_valid=True,direction="EGRESS",max_pkt_len=128)

ingress = bfrt.Hierarchical_Fermat.pipe.Ingress
egress = bfrt.Hierarchical_Fermat.pipe.Egress
path="/mnt/onl/data/Hierarchical_Fermat/table/"

'''arp_host'''
arp_host_conf=open(path+"arp_host_conf.txt")

table=ingress.arp_host
table.clear();
for t in arp_host_conf:
    d=t.split()
    table.add_with_unicast_send(proto_dst_addr=ip_address(d[0]), port=int(d[1]))

'''ipv4_host'''
ipv4_host_conf=open(path+"ipv4_host_conf.txt")

table=ingress.ipv4_host
table.clear();
for t in ipv4_host_conf:
    d=t.split()
    table.add_with_unicast_send(dst_addr=ip_address(d[0]), ecmp=int(d[1]), port=int(d[2]))

'''direction_in_t'''
direction_in_conf=open(path+"direction_in_conf.txt")

table=ingress.direction_in_t
table.clear();
for t in direction_in_conf:
    d=t.split()
    table.add_with_direction_in_a(ingress_port=int(d[0]))

'''direction_out_t'''
direction_out_conf=open(path+"direction_out_conf.txt")

table=egress.direction_out_t
table.clear();
for t in direction_out_conf:
    d=t.split()
    table.add_with_direction_out_a(egress_port=int(d[0]))

table=ingress.cal_ID_4_t
table.clear()
for a in range(2):
    for b in range(2):
        for c in range(2):
            table.add_with_cal_ID_4_a(src_addr_31_31_=a, dst_addr_31_31_=b, ll_31_31_=c, val=(a*4+b*2+c)*256)



table=ingress.map_in_hash_t
table.clear()
table.add_with_map_in_hash_a(tstamp=1)

table=ingress.map_filter_hash_1_t
table.clear()
table.add_with_map_filter_hash_1_a(tstamp=1)
table=ingress.map_filter_hash_2_t
table.clear()
table.add_with_map_filter_hash_2_a(tstamp=1)

table=egress.map_out_hash_t
table.clear()
table.add_with_map_out_hash_a(res=1)

'''range_conf_1'''
range_conf_1=open(path+"range_conf_1.txt")
table=ingress.filter_1_range_map_t
table.clear();
for t in range_conf_1:
    d=t.split();
    for i in range(len(d)-1):
        table.add_with_filter_1_range_map_a(layer_1_filter_counter_start = int(d[i]), layer_1_filter_counter_end = int(d[i+1])-1,type = i+1)
        
'''range_conf_2'''
range_conf_2=open(path+"range_conf_2.txt")
table=ingress.filter_2_range_map_t
table.clear();
for t in range_conf_2:
    d=t.split();
    for i in range(len(d)-1):
        table.add_with_filter_2_range_map_a(layer_2_filter_counter_start = int(d[i]), layer_2_filter_counter_end = int(d[i+1])-1,type = i+1)


'''zone_base_conf'''
zone_base_conf=open(path+"zone_base_conf.txt")
zone_size=[]
zone_base=[]
for t in zone_base_conf:
    zone=t.split()
    for i in range(len(zone)):
        zone_base.append(int(zone[i]))

'''zone_size_conf'''
zone_size_conf=open(path+"zone_size_conf.txt")
for t in zone_size_conf:
    zone=t.split()
    for i in range(len(zone)):
        zone_size.append(int(zone[i]))
type_len = 3

mask_list = []
'''mask_conf'''
ingress.get_map_index_t.clear()
ingress.get_remap_index_t.clear()
mask_conf=open(path+"mask_conf.txt")
for t in mask_conf:
    masks=t.split()
    for i in range(len(masks)):
        mask_list.append(int(masks[i]))
        ingress.get_map_index_t.add_with_get_map_index_a(type=i+1, mask = int(masks[i]))
        if i == 1:
            ingress.get_remap_index_t.set_default_with_get_remap_index_a(mask = int(masks[i]))



ingress.mem_zone_map_index_1_t.clear()
ingress.mem_zone_map_index_2_t.clear()
ingress.mem_zone_map_index_3_t.clear()
ingress.mem_zone_remap_index_1_t.clear()
ingress.mem_zone_remap_index_2_t.clear()
ingress.mem_zone_remap_index_3_t.clear()
#mask TBD

for i in range(type_len):
    up = 65535 & mask_list[i]
    res = up % zone_size[i]
    mul = (up-res)//zone_size[i]
    ost = 0
    for j in range(mul+1):
        if (zone_base[i] - j*zone_size[i]>=0):
            ost = zone_base[i] - j*zone_size[i]
        else:
            ost = 65536 - j*zone_size[i] + zone_base[i]
        table=ingress.mem_zone_map_index_1_t
        
        table.add_with_mem_zone_map_index_1_a( type = i+1,index_1_start =j*zone_size[i], index_1_end =min((j+1)*zone_size[i]-1,up), offset = ost)
       
        
        table=ingress.mem_zone_map_index_2_t
        
        table.add_with_mem_zone_map_index_2_a(type = i+1,index_2_start =j*zone_size[i], index_2_end =min((j+1)*zone_size[i]-1,up), offset = ost)
        

        table=ingress.mem_zone_map_index_3_t
    
        table.add_with_mem_zone_map_index_3_a(type = i+1,index_3_start =j*zone_size[i], index_3_end =min((j+1)*zone_size[i]-1,up), offset = ost)
        

        if i == 1:
            table=ingress.mem_zone_remap_index_1_t
            table.add_with_mem_zone_remap_index_1_a(re_index_1_start =j*zone_size[i], re_index_1_end =min((j+1)*zone_size[i]-1,up), offset = ost)
           
            
            table=ingress.mem_zone_remap_index_2_t
            
            table.add_with_mem_zone_remap_index_2_a(re_index_2_start =j*zone_size[i], re_index_2_end =min((j+1)*zone_size[i]-1,up), offset = ost)
            

            table=ingress.mem_zone_remap_index_3_t
        
            table.add_with_mem_zone_remap_index_3_a(re_index_3_start =j*zone_size[i], re_index_3_end =min((j+1)*zone_size[i]-1,up), offset = ost)
            
table=ingress.fermat_new_t
table.clear()
table.add_with_fermat_new_a(ether_type=0x3335)

table=egress.fermat_continue_t
table.clear()

for i in range(256):
    if i%32 != 31 :
        table.add_with_fermat_mirror_a(index = (i+1)*16-1)
        table.add_with_fermat_mirror_a(index = (i+1)*16-1+4096)
    else:
        table.add_with_fermat_stop_a(index = (i+1)*16-1)
        table.add_with_fermat_stop_a(index = (i+1)*16-1+4096)


table=ingress.clean_continue_1_t
table.clear()
for i in range(32):
    if i%4==3:
        table.add_with_clean_stop_1_a(index=(i+1)*1024-1)
        table.add_with_clean_stop_1_a(index=(i+1)*1024-1+32768)
    else:
        table.add_with_clean_mirror_1_a(index=(i+1)*1024+1)
        table.add_with_clean_mirror_1_a(index=(i+1)*1024-1+32768)
table=ingress.clean_continue_2_t
table.clear()
for i in range(32):
    if i%4==3:
        table.add_with_clean_stop_2_a(index=(i+1)*512-1)
        table.add_with_clean_stop_2_a(index=(i+1)*512-1+16384)
    else:
        table.add_with_clean_mirror_2_a(index=(i+1)*512-1)
        table.add_with_clean_mirror_2_a(index=(i+1)*512-1+16384)




