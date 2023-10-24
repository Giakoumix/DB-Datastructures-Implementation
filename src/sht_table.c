#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bf.h"
#include "sht_table.h"
#include "ht_table.h"
#include "ht_utils.h"
#include "sht_utils.h"
#include "record.h"

#define CALL_OR_DIE(call)     \
  {                           \
    BF_ErrorCode code = call; \
    if (code != BF_OK) {      \
      BF_PrintError(code);    \
      exit(code);             \
    }                         \
  }



int SHT_CreateSecondaryIndex(char *sfileName,  int buckets, char* fileName){
	int fd;
    if (BF_CreateFile(sfileName) != BF_OK)
        return -1;
    if (BF_OpenFile(sfileName, &fd) != BF_OK)
        return -1;
    BF_Block *block;
    BF_Block_Init(&block);

	BF_AllocateBlock(fd, block);
    void *data = BF_Block_GetData(block);
	SHT_info *info = data;
	info[0] = make_sht_info(fd, buckets, fileName);

	BF_Block_SetDirty(block);

	for (int i = 0 ; i < info->buckets ; i++) {
        BF_AllocateBlock(fd, block);
        void *block_data = BF_Block_GetData(block);
    
        block_data += (info->blocks_per_index)*sizeof(Pair);
        SHT_block_info block_info = make_sht_block_info(0, i+1);
        memcpy(block_data, &block_info, sizeof(block_info));
        
        BF_Block_SetDirty(block);
        BF_UnpinBlock(block);
    }

	BF_Block_Destroy(&block);
    BF_CloseFile(fd);
	
	return 0;
}

SHT_info* SHT_OpenSecondaryIndex(char *indexName){
	int file_desc;
    if (BF_OpenFile(indexName, &file_desc) != BF_OK)
        return NULL;

	BF_Block *block; 
    BF_Block_Init(&block);

    BF_GetBlock(file_desc, 0, block);
    void *data = BF_Block_GetData(block);
    
    SHT_info *info = data;
    BF_Block_Destroy(&block);

	if (info->file_type != 's')
		return NULL;

	return info;
}


int SHT_CloseSecondaryIndex( SHT_info* SHT_info ){
	BF_Block *block;
    BF_Block_Init(&block);

	BF_GetBlock(SHT_info->file_desc, 0, block);
    void *data = BF_Block_GetData(block);
    memcpy(data, SHT_info, sizeof(*SHT_info));

    block_dirty_destroy(block);

	if (BF_CloseFile(SHT_info->file_desc) != BF_OK)
        return -1;

    return 0;
}

int SHT_SecondaryInsertEntry(SHT_info* sht_info, Record record, int block_id){
	BF_Block *block;
    BF_Block_Init(&block);

	// Hash το record
	int hash = hash_string(sht_info, record.name);
	int index_block_id = sht_info->ht[hash][1];

	// Παιρνουμε το block_info
	BF_GetBlock(sht_info->file_desc, index_block_id, block);
	void *data = BF_Block_GetData(block);
	void *pointer;

	SHT_block_info *block_info = malloc(sizeof(*block_info));
	int offset1 = (sht_info->blocks_per_index)*sizeof(Pair);
	pointer = data + offset1;

	memcpy(block_info, pointer, sizeof(*block_info));

	// Δημιουργουμε το Pair
	Pair pair;
	strcpy(pair.name,record.name);
	pair.pointer = block_id;

	// Αν χωραει στο block 
	if (block_info->num_of_blocks < sht_info->blocks_per_index) {
		int offset2 = block_info->num_of_blocks*sizeof(Pair);
		pointer = data + offset2;

		memcpy(pointer, &pair, sizeof(pair));

		block_info->num_of_blocks++;
		pointer = data + offset1;

		memcpy(pointer, block_info, sizeof(*block_info));
	}
	else {
		// Το βαζω στο νεο allocated block
		BF_Block *new_block;
        BF_Block_Init(&new_block);

		if (BF_AllocateBlock(sht_info->file_desc, new_block) != BF_OK)
            printf("wrong allocation");

		void *new_data = BF_Block_GetData(new_block);
        
        int new_block_id;
        BF_GetBlockCounter(sht_info->file_desc, &new_block_id);
        new_block_id--;

		SHT_block_info new_block_info = make_sht_block_info(1, new_block_id);
		pointer = new_data;

		memcpy(pointer, &pair, sizeof(pair));

		// Συνδεω το καινουριο block με το παλιο 

		sht_info->ht[hash][1] = new_block_id;
		new_block_info.next = block_info->index_block_id;

		pointer = new_data + offset1;
		memcpy(pointer, &new_block_info, sizeof(new_block_info));
	
		// Αυξανω τον block counter
		sht_info->block_counter++;

		// Αποδεσμευση και Unpin
        block_dirty_destroy(new_block);
	}

	free(block_info);
    block_dirty_destroy(block);

	return 0;
}

int SHT_SecondaryGetAllEntries(HT_info* ht_info, SHT_info* sht_info, char* name){
	BF_Block *block;
    BF_Block_Init(&block);

	Pair *pair = malloc(sizeof(*pair));
	Record *r = malloc(sizeof(*r));
	SHT_block_info *index_block_info = malloc(sizeof(*index_block_info));
	HT_block_info *block_info = malloc(sizeof(*block_info));

	int block_counter = 0;
	int offset1 = sht_info->blocks_per_index*sizeof(*pair);

	// Hash για index_block_id
	int index_hash = hash_string(sht_info, name);
	int index_block_id = sht_info->ht[index_hash][1];

	// Για το index_block με id == hash και τα block υπερχειλισης του
	do {
		BF_GetBlock(sht_info->file_desc, index_block_id, block);

		void *data = BF_Block_GetData(block);
		void *pointer = data + offset1;
		memcpy(index_block_info, pointer, sizeof(*index_block_info));

		for (int i = 0 ; i < index_block_info->num_of_blocks ; i++) {
			memcpy(pair, data+i*sizeof(*pair), sizeof(*pair));

			if (!strcmp(pair->name, name)) {
				BF_GetBlock(ht_info->file_desc, pair->pointer, block);
				void *block_data = BF_Block_GetData(block);

				int offset = ht_info->records_per_block*sizeof(Record);
				void *pointer = block_data + offset;
        		memcpy(block_info, pointer, sizeof(*block_info));

				for (int j = 0 ; j < ht_info->records_per_block ; j++) {
            		memcpy(r, block_data+j*sizeof(*r), sizeof(*r));
					if (!strcmp(r->name, name)) {
                		printRecord(*r);
            		}  
				}

			}
		}

		index_block_id = index_block_info->next;
	} while(index_block_info->next);

	free(pair);
	free(r);
	free(index_block_info);
	free(block_info);
	BF_Block_Destroy(&block);

	return 0;
}



