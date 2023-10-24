
#include <limits.h>

#include "hash_statistics.h"

int HashStatistics(char *file_name) {
    // Τα ζητουμενα της συναρτησης
    int num_of_blocks = 0;
    double min_records = INT_MAX, average_records, max_records = INT_MIN;
    double average_blocks = 0;
    int overflow_buckets = 0;
    int *overflow_blocks_per_bucket;

    int buckets;

    int file_desc;
    if (BF_OpenFile(file_name, &file_desc) != BF_OK) {
        printf("Already open\n");
    }

    BF_Block *block;
    BF_Block_Init(&block);

    BF_GetBlock(file_desc, 0, block);
    char *data = BF_Block_GetData(block);

    if (*data == 's') {
        SHT_info *info = (SHT_info *) data;
        SHT_block_info *block_info = malloc(sizeof(*block_info));

        overflow_blocks_per_bucket = malloc(info->buckets * sizeof(int));     

        int all_records = 0;
        buckets = info->buckets;

        for (int i = 0 ; i < info->buckets ; i++) {
            // Παιρνουμε το ith block 
            BF_GetBlock(file_desc, info->ht[i][1], block);
            void *block_data = BF_Block_GetData(block);
            int offset = info->blocks_per_index*sizeof(Pair);
            
            // Παιρνουμε το block_info του
            memcpy(block_info, block_data + offset, sizeof(*block_info));
    
            num_of_blocks++;

            int block_records = block_info->num_of_blocks;
            overflow_blocks_per_bucket[i] = 0;
            
            // Block υπερχειλισης
            if (block_info->next)
                overflow_buckets++;

            while (block_info->next) {
                BF_GetBlock(file_desc, block_info->next, block);
                block_data = BF_Block_GetData(block);

                memcpy(block_info, block_data + offset, sizeof(*block_info));
                
                block_records += block_info->num_of_blocks;
                overflow_blocks_per_bucket[i]++;
                num_of_blocks++;
            }
            
            all_records += block_records;

            // Eλαχιστο πληθος records
            if (block_records < min_records)
                min_records = block_records;
            // Μεγιστο πληθος records
            if (block_records > max_records)
                max_records = block_records;
        
        }

        average_blocks = (double) num_of_blocks / info->buckets;
        average_records = (double) all_records/ info->buckets;

        free(block_info);
    }
    else if (*data = 'h') {
        HT_info *info = (HT_info *) data;
        HT_block_info *block_info = malloc(sizeof(*block_info));

        overflow_blocks_per_bucket = malloc(info->num_buckets * sizeof(int));     

        int all_records = 0;
        buckets = info->num_buckets;

        for (int i = 0 ; i < info->num_buckets ; i++) {
            // Παιρνουμε το ith block 
            BF_GetBlock(file_desc, info->ht[i][1], block);
            void *block_data = BF_Block_GetData(block);
            int offset = info->records_per_block*sizeof(Record);
            
            // Παιρνουμε το block_info του
            memcpy(block_info, block_data + offset, sizeof(*block_info));
    
            num_of_blocks++;

            int block_id;
            int block_records = block_info->num_of_records;
            overflow_blocks_per_bucket[i] = 0;
            
            // Block υπερχειλισης
            if (block_info->next)
                overflow_buckets++;

            while (block_info->next) {
                BF_GetBlock(file_desc, block_info->next, block);
                block_data = BF_Block_GetData(block);

                memcpy(block_info, block_data + offset, sizeof(*block_info));
                
                block_records += block_info->num_of_records;
                overflow_blocks_per_bucket[i]++;
                num_of_blocks++;
            }
            
            all_records += block_records;

            // Eλαχιστο πληθος records
            if (block_records < min_records)
                min_records = block_records;
            // Μεγιστο πληθος records
            if (block_records > max_records)
                max_records = block_records;
        
        }

        average_blocks = (double) num_of_blocks / info->num_buckets;
        average_records = (double) all_records/ info->num_buckets;

        free(block_info); 
    }
    else 
        return -1; 

    printf("Το αρχειο εχει %d blocks\n", num_of_blocks);
    printf("Το ελαχιστο πληθος records ειναι %.3f\n", min_records);
    printf("Το μεγιστο πληθος records ειναι %.3f\n", max_records);
    printf("Το μεσο πληθος records ειναι %.3f\n", average_records);
    printf("Ο μεσος αριθμος blocks που εχει καθε bucket ειναι %.3f\n", average_blocks);
    printf("To πληθος των buckets που εχουν block υπερχειλισης ειναι %d\n", overflow_buckets);

    int counter = 0;
    for (int i = 0 ; i < buckets ; i++) {
        if (overflow_blocks_per_bucket[i])
            printf("Το %d bucket ειχε %d\n", i, overflow_blocks_per_bucket[i]);
        counter += overflow_blocks_per_bucket[i];
    }
    printf("Συνολο %d απο block υπερχειλισης\n", counter);

    BF_Block_Destroy(&block);
    free(overflow_blocks_per_bucket);

    return 0;
}