
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bf.h"
#include "sht_utils.h"
#include "ht_table.h"

SHT_info make_sht_info(int file_desc, int num_of_buckets, char *ht_name) {
    SHT_info info;

    info.file_desc = file_desc;
    info.file_type = 's';
    info.block_counter = 10;
    info.blocks_per_index = (BF_BLOCK_SIZE -sizeof(SHT_block_info))/sizeof(Pair);
    info.buckets = num_of_buckets;
    info.ht = malloc(info.buckets * sizeof(*info.ht));

    for (int i = 0 ; i < info.buckets ; i++) {
        info.ht[i] = malloc(2 * sizeof(int));
        info.ht[i][0] = i;
        info.ht[i][1] = i+1;
    }

    return info;
}

SHT_block_info make_sht_block_info(int blocks, int index_block_id) {
    SHT_block_info info;

    info.num_of_blocks = blocks;
    info.index_block_id = index_block_id;
    info.next = 0;
}

int hash_string(void *sht_info, char *name) {
    SHT_info *info = sht_info;
    int hash = 0;
    
    for (int i = 0 ; i < 7; i++) 
        hash += name[i];

    return hash%info->buckets; 
}