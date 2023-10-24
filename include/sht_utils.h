
#pragma once

#include "sht_table.h"

SHT_info make_sht_info(int file_desc, int num_of_buckets, char *ht_name);

SHT_block_info make_sht_block_info(int records, int block_id);

Pair make_pair(int block_id, char *name);

int hash_string(void *sht_info, char *name);