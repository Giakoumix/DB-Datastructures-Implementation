#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bf.h"
#include "ht_table.h"
#include "record.h"
#include "ht_utils.h"

#define CALL_OR_DIE(call)     \
  {                           \
    BF_ErrorCode code = call; \
    if (code != BF_OK) {      \
      BF_PrintError(code);    \
      exit(code);             \
    }                         \
  }


int HT_CreateFile(char *fileName,  int buckets){
    int fd;
    if (BF_CreateFile(fileName) != BF_OK)
        return -1;
    if (BF_OpenFile(fileName, &fd) != BF_OK)
        return -1;
    BF_Block *block;
    BF_Block_Init(&block);
    
    BF_AllocateBlock(fd, block);
    void *data = BF_Block_GetData(block);
    HT_info *info = data;
    info[0] = make_ht_info(fd, buckets, hash_int);
    
    BF_Block_SetDirty(block);
    
    for (int i = 0 ; i < info->num_buckets ; i++) {
        BF_AllocateBlock(fd, block);
        void *block_data = BF_Block_GetData(block);

        block_data += (info->records_per_block)*sizeof(Record);
        HT_block_info block_info = make_block_info(0, i+1);
        memcpy(block_data, &block_info, sizeof(block_info));
        
        BF_Block_SetDirty(block);
        BF_UnpinBlock(block);
    }

    BF_CloseFile(fd);
    BF_Block_Destroy(&block);

    return 0;
}

HT_info* HT_OpenFile(char *fileName){
    int file_desc;
    if (BF_OpenFile(fileName, &file_desc) != BF_OK)
        return NULL;

    BF_Block *block; 
    BF_Block_Init(&block);

    BF_GetBlock(file_desc, 0, block);
    void *data = BF_Block_GetData(block);
    
    HT_info *info = data;
    BF_Block_Destroy(&block);

    if (info->file_type != 'h')
        return NULL;
    
    return info;
}


int HT_CloseFile( HT_info* HT_info ) {
    // Το HT_info εγγραφεται μια φορα στον δισκο 
    // κατα το κλεισιμο του αρχειου διοτι η συνεχης
    // εγγραφη του κατα τα inserts ειναι κοστοβορα για τον δισκο
    BF_Block *block;
    BF_Block_Init(&block);

    BF_GetBlock(HT_info->file_desc, 0, block);
    void *data = BF_Block_GetData(block);
    memcpy(data, HT_info, sizeof(*HT_info));
    
    block_dirty_destroy(block);

    if (BF_CloseFile(HT_info->file_desc) != BF_OK)
        return -1;
    return 0;
}

int HT_InsertEntry(HT_info* ht_info, Record record){
    
    BF_Block *block;
    BF_Block_Init(&block);

    // Hash το record
    int block_inserted;
    int hash = ht_info->hash_func(ht_info, record.id);
    int block_id = ht_info->ht[hash][1];

    // Παιρνουμε το block_info
    BF_GetBlock(ht_info->file_desc, block_id, block);
    void *data = BF_Block_GetData(block);
    void *pointer;

    HT_block_info *block_info = malloc(sizeof(*block_info));
    int offset1 = (ht_info->records_per_block)*sizeof(Record);
    pointer = data + offset1;
    
    memcpy(block_info, pointer, sizeof(*block_info));
    
    // Αν χωραει στο block 
    if (block_info->num_of_records < ht_info->records_per_block) {
        int offset2 = block_info->num_of_records*sizeof(record);
        pointer = data + offset2;

        memcpy(pointer, &record, sizeof(record));
    
        block_info->num_of_records++;
        pointer = data + offset1;

        memcpy(pointer, block_info, sizeof(*block_info));
    
        block_inserted = block_info->block_id;
    }
    else {
        // Το βαζω στο νεο allocated block
        BF_Block *new_block;
        BF_Block_Init(&new_block);

        if (BF_AllocateBlock(ht_info->file_desc, new_block) != BF_OK)
            printf("wrong allocation");

        void *new_data = BF_Block_GetData(new_block);
        
        int new_block_id;
        BF_GetBlockCounter(ht_info->file_desc, &new_block_id);
        new_block_id--;

        HT_block_info new_block_info = make_block_info(1, new_block_id);
        pointer = new_data;

        memcpy(pointer, &record, sizeof(record));

        // Συνδεω το καινουριο block με το παλιο 

        ht_info->ht[hash][1] = new_block_id;
        new_block_info.next = block_info->block_id;

        pointer = new_data + offset1;
        memcpy(pointer, &new_block_info, sizeof(new_block_info));
        block_inserted = new_block_id;

        // Αυξανω τον block counter
        ht_info->block_counter++;

        // Αποδεσμευση και Unpin
        block_dirty_destroy(new_block);
    }

    free(block_info);
    block_dirty_destroy(block);

    return block_inserted;
}

int HT_GetAllEntries(HT_info* ht_info, void *value ){
    BF_Block *block;
    BF_Block_Init(&block);

    Record *r = malloc(sizeof(*r));
    HT_block_info *block_info = malloc(sizeof(*block_info));

    int block_counter = 0;
    int offset = ht_info->records_per_block*sizeof(*r);

    // Hash για block_id
    int *id = value;
    int hash = ht_info->hash_func(ht_info, *id);
    int block_id = ht_info->ht[hash][1];
    
    int continue_search = 1;
    do {
        BF_GetBlock(ht_info->file_desc, block_id, block);
        block_counter++;

        void *data = BF_Block_GetData(block);
        void *pointer = data + offset;
        memcpy(block_info, pointer, sizeof(*block_info));

        for (int j = 0 ; j < block_info->num_of_records ; j++) {
            memcpy(r, data+j*sizeof(*r), sizeof(*r));

            if (r->id == *id) {
                printRecord(*r);
                continue_search = 0;
                break;
            }            
        }

        block_id = block_info->next;
    } while (block_info->next && continue_search);

    BF_Block_Destroy(&block);

    free(block_info);
    free(r);

    return block_counter;
}




