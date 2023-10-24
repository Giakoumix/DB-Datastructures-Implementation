
#pragma once

#include "ht_table.h"

HT_info make_ht_info(int file_desc, int num_of_buckets, Hash_Func hash_func);

HT_block_info make_block_info(int records, int block_id);

BF_ErrorCode block_dirty_destroy(BF_Block *block); 

int hash_int(void *ht_info, long int id);