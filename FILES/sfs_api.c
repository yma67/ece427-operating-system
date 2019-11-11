#include "sfs_api.h"

#define synch_bitmap(_opt) _opt##_blocks(NUM_BLOCKS - 1, 1, bitmap)

#define synch_superblock(_opt) {                                                \
    void* page_buf = (void *)calloc(sizeof(uint8_t), BLOCK_SIZE);               \
    if (!strcmp(#_opt, "write")) {                                              \
        memcpy(page_buf, &super_block, sizeof(super_block_t));                  \
        write_blocks(0, 1, page_buf);                                           \
        memset(page_buf, 0, sizeof(uint8_t) * BLOCK_SIZE);                      \
    } else if (!strcmp(#_opt, "read")) {                                        \
        read_blocks(0, 1, page_buf);                                            \
        memcpy(&super_block, page_buf, sizeof(super_block));                    \
    }                                                                           \
    free(page_buf);                                                             \
}

#define synch_inode(_opt) _opt##_blocks(1, NUM_INODE_BLOCKS, inode_cache)

#define synch_directory(_opt) {                                                \
    page_t* page_buf = (page_t *)calloc(sizeof(page_t), 1);                    \
    int is_finished = 0;                                                       \
    for (int i = 0; i < 12; i++) {                                             \
        if (inode_cache[0].pages[i] == PGPTR_NULL) {                           \
            is_finished = 1;                                                   \
            break;                                                             \
        }                                                                      \
        _opt##_blocks(1 + NUM_INODE_BLOCKS + inode_cache[0].pages[i],          \
                    1, directory_cache + i * (BLOCK_SIZE / sizeof(dirent_t))); \
    }                                                                          \
    if ((!is_finished) && inode_cache[0].index_page != PGPTR_NULL) {           \
        read_blocks(1 + NUM_INODE_BLOCKS + inode_cache[0].index_page,          \
                    1, page_buf);                                              \
        for (int i = 0; (unsigned long)i < (NUM_DATA_BLOCKS -                  \
                         12 * (BLOCK_SIZE / sizeof(dirent_t))) /               \
                         (BLOCK_SIZE / sizeof(dirent_t)); i++) {               \
            _opt##_blocks(1 + NUM_INODE_BLOCKS + page_buf->content.index[i],   \
                        1, directory_cache +                                   \
                           (12 + i) * (BLOCK_SIZE / sizeof(dirent_t)));        \
        }                                                                      \
    }                                                                          \
    free(page_buf);                                                            \
}

void mksfs(int flag) {
    if (flag) {
        init_fresh_disk(DISC_NAME, BLOCK_SIZE, NUM_BLOCKS);
        // super block
        super_block.magic_number = 0xACBD0005;
        super_block.page_size = BLOCK_SIZE;
        super_block.file_sys_size = NUM_BLOCKS;
        super_block.num_data_pages = NUM_DATA_BLOCKS;
        super_block.inode_root = 0;
        synch_superblock(write);
        // i node
        inode_cache[0].link_cnt = 1;
        inode_cache[0].uid = 0; 
        inode_cache[0].fsize = 0;
        uint32_t page_vec[] = {0         , PGPTR_NULL, PGPTR_NULL, PGPTR_NULL, 
                               PGPTR_NULL, PGPTR_NULL, PGPTR_NULL, PGPTR_NULL, 
                               PGPTR_NULL, PGPTR_NULL, PGPTR_NULL, PGPTR_NULL};
        memcpy(inode_cache[0].pages, page_vec, 12 * sizeof(uint32_t));
        inode_cache[0].index_page = PGPTR_NULL;
        synch_inode(write);
        // bitmap
        bitmap[0] = 1;
        synch_bitmap(write);
        // directory
        for (int i = 0; i < NUM_DATA_BLOCKS; i++)
            directory_cache[i].inode_index = INODE_NULL;
        synch_directory(write);
    } else {
        init_disk(DISC_NAME, BLOCK_SIZE, NUM_BLOCKS);
        synch_superblock(read);
        synch_inode(read);
        synch_directory(read);
        synch_bitmap(read);
    }
}


