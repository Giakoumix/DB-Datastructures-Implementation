
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bf.h"
#include "hp_file.h"
#include "utils.h"

HP_info make_hp_info(int file_desc, void *first_block) {
    HP_info info;

    info.file_desc = file_desc;
    info.file_type = 'g';
    info.records_per_block = (BF_BLOCK_SIZE-sizeof(HP_block_info))/sizeof(Record);
    info.last_block_id = 1;
    info.first_block = first_block;

    return info;
}

HP_block_info make_block_info(int records) {
    HP_block_info info;

    info.num_of_records = records;
    info.next = 0;

    return info;
}

BF_ErrorCode block_dirty_destroy(BF_Block *block) {
    BF_Block_SetDirty(block);
    BF_UnpinBlock(block);
    BF_Block_Destroy(&block);
}