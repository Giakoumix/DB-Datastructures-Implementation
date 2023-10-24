
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bf.h"
#include "ht_table.h"
#include "ht_utils.h"

int hash_int(void *ht_info, long int id) {
    HT_info *info = ht_info;
    return id % info->num_buckets;
}

HT_info make_ht_info(int file_desc, int num_of_buckets, Hash_Func hash_func) {
    HT_info info;

    info.file_desc = file_desc;
    info.file_type = 'h';
    info.block_counter = 10;
    info.records_per_block = (BF_BLOCK_SIZE- sizeof(HT_block_info))/sizeof(Record);
    info.num_buckets = num_of_buckets;
    info.ht = malloc(info.num_buckets * sizeof(*info.ht));
    info.hash_func = hash_func;

    for (int i = 0 ; i < info.num_buckets ; i++) {
        info.ht[i] = malloc(2 * sizeof(int));
        info.ht[i][0] = i;
        info.ht[i][1] = i+1;
    }

    return info;
}

HT_block_info make_block_info(int records, int block_id) {
    HT_block_info info;

    info.num_of_records = records;
    info.block_id = block_id;
    info.next = 0;

    return info;
}

BF_ErrorCode block_dirty_destroy(BF_Block *block) {
    BF_Block_SetDirty(block);
    BF_UnpinBlock(block);
    BF_Block_Destroy(&block);
}