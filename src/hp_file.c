#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bf.h"
#include "hp_file.h"
#include "record.h"
#include "utils.h"

typedef void * Pointer;

#define CALL_BF(call)       \
{                           \
  BF_ErrorCode code = call; \
  if (code != BF_OK) {         \
    BF_PrintError(code);    \
    return HP_ERROR;        \
  }                         \
}



int HP_CreateFile(char *fileName) {
    int fd;
    if (BF_CreateFile(fileName) != BF_OK)
        return -1;

    if (BF_OpenFile(fileName, &fd) != BF_OK)
        return -1;

    BF_Block *block;
    BF_Block_Init(&block);
    
    BF_AllocateBlock(fd, block);
    void *data = BF_Block_GetData(block);

    HP_info *info = data;
    info[0] = make_hp_info(fd, data);

    BF_Block_SetDirty(block);
    BF_UnpinBlock(block);
    
    BF_AllocateBlock(fd, block);
    
    void *d = BF_Block_GetData(block);
    d += info->records_per_block*sizeof(Record);

    HP_block_info block_info = make_block_info(0);

    memcpy(d, &block_info, sizeof(HP_block_info));

    block_dirty_destroy(block);

    BF_CloseFile(fd);
    
    return 0;
}

HP_info* HP_OpenFile(char *fileName) {
    int file_desc;
    if (BF_OpenFile(fileName, &file_desc) != BF_OK)
        return NULL;

    BF_Block *block; 
    BF_Block_Init(&block);
    
    BF_GetBlock(file_desc, 0, block);
    void *data = BF_Block_GetData(block);

    HP_info *info = data;
    BF_Block_Destroy(&block);
    
    return &info[0];
}


int HP_CloseFile( HP_info* hp_info ) {
    if (BF_CloseFile(hp_info->file_desc) != BF_OK)
        return -1;
    return 0;
}

int HP_InsertEntry(HP_info* hp_info, Record record) {

    BF_Block *block;
    BF_Block_Init(&block);

    BF_GetBlock(hp_info->file_desc, hp_info->last_block_id, block);
    void *data = BF_Block_GetData(block);
    void *pointer;

    HP_block_info *block_info = malloc(sizeof(*block_info));
    long offset1 = (hp_info->records_per_block-1)*sizeof(Record);
    pointer = data + offset1;
    
    memcpy(block_info, pointer, sizeof(*block_info));
    
    // Αν χωράει στο block 
    if (block_info->num_of_records < hp_info->records_per_block) {
        int offset2 = block_info->num_of_records*sizeof(record);
        pointer = data + offset2;

        memcpy(pointer, &record, sizeof(record));
        
        block_info->num_of_records++;
        pointer = data + offset1;

        memcpy(pointer, block_info, sizeof(*block_info));
        // Dirty
    }
    else {
        // Το βαζω στο νεο allocated block
        BF_Block *new_block;
        BF_Block_Init(&new_block);
        
        if (BF_AllocateBlock(hp_info->file_desc, new_block) != BF_OK)
            printf("wrong allocation");
        
        void *new_data = BF_Block_GetData(new_block);
        
        HP_block_info new_block_info = make_block_info(1);
        pointer = new_data + 0;
        
        memcpy(pointer, &record, sizeof(record));
        
        pointer = new_data+offset1;
        memcpy(pointer, &new_block_info, sizeof(new_block_info));
        
        // pointer = data+offset1;
        // memcpy(pointer, block_info, sizeof(block_info));
        
        BF_Block_SetDirty(new_block);

        hp_info->last_block_id++;
        BF_GetBlock(hp_info->file_desc, 0, new_block);
        void *fb_data = BF_Block_GetData(block);
        memcpy(fb_data, hp_info, sizeof(*hp_info));

        block_dirty_destroy(new_block);
    }

    free(block_info);
    block_dirty_destroy(block);

    return hp_info->last_block_id;
}

int HP_GetAllEntries(HP_info* hp_info, int value) {

    BF_Block *block;
    BF_Block_Init(&block);

    Record *r = malloc(sizeof(*r));
    HP_block_info *block_info = malloc(sizeof(*block_info));

    int block_counter = 0;
    int offset = (hp_info->records_per_block-1)*sizeof(Record);

    for (int i = 1 ; i < hp_info->last_block_id ; i++) {
        BF_GetBlock(hp_info->file_desc, i, block);
        block_counter++;

        void *data = BF_Block_GetData(block);
        void *pointer = data + offset;
        memcpy(block_info, pointer, sizeof(*block_info));

        for (int j = 0 ; j < block_info->num_of_records ; j++) {
            memcpy(r, data+j*sizeof(*r), sizeof(*r));
            
            if (r->id == value) {
                printRecord(*r);
                break;
            }            
        }
    }
    
    BF_Block_Destroy(&block);

    free(block_info);
    free(r);

    return block_counter;
}

