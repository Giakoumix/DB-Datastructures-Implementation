
#pragma once

#include "hp_file.h"

HP_info make_hp_info(int file_desc, void *first_block);

HP_block_info make_block_info(int records);

BF_ErrorCode block_dirty_destroy(BF_Block *block);