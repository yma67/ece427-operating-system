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

#define dalloc() ({                                                            \
    uint32_t first_free = 0;                                                   \
    for (int i = 1; i < NUM_DATA_BLOCKS; i++) {                                \
        if (bitmap[first_free] == 0) {                                         \
            first_free = i;                                                    \
            break;                                                             \
        }                                                                      \
    }                                                                          \
    if (first_free == 0) {                                                     \
        perror("Disc Full\n");                                                 \
        exit(1);                                                               \
    }                                                                          \
    bitmap[first_free] = 1;                                                    \
    first_free;                                                                \
})

#define synch_idx_inode_cache(_opt) {                                          \
    if (inode_cache[0].index_page != PGPTR_NULL) {                             \
        _opt##_blocks(1 + NUM_DATA_BLOCKS + inode_cache[0].index_page,         \
                      1, idx_node_cache);                                      \
    }                                                                          \
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
        // File Open Table
        for (int i = 0; i < NUM_DATA_BLOCKS; i++)
            file_open_table[i].inode_idx = INODE_NULL;
    } else {
        init_disk(DISC_NAME, BLOCK_SIZE, NUM_BLOCKS);
        // File Open Table 
        for (int i = 0; i < NUM_DATA_BLOCKS; i++)
            file_open_table[i].inode_idx = INODE_NULL;
        synch_superblock(read);
        synch_inode(read);
        synch_idx_inode_cache(read);
        synch_directory(read);
        synch_bitmap(read);
    }
}

int sfs_open(char *name) {
    int is_fexist = 0;
    for (int i = 0; i < NUM_DATA_BLOCKS; i++) {
        if (!strcmp(name, directory_cache[i].name)) {
            is_fexist = 1;
            break;
        }
    }
    // make new file
    if (!is_fexist) {
        int empty_inode_idx = 0, empty_dir_idx = PGPTR_NULL;
        for (int i = 1; i < NUM_DATA_BLOCKS; i++) {
            if (inode_cache[i].link_cnt < 1) {
                empty_inode_idx = i;
                break;
            }
        }
        for (int i = 0; i < NUM_DATA_BLOCKS; i++) {
            if (directory_cache[i].inode_index == INODE_NULL) {
                empty_dir_idx = i;
                break;
            }
        }
        if (empty_dir_idx == PGPTR_NULL) {
            perror("File Num overflow\n");
            exit(1);
        }
        inode_cache[empty_inode_idx].link_cnt = 1;
        inode_cache[empty_inode_idx].uid = 0; 
        inode_cache[empty_inode_idx].fsize = 0;
        uint32_t page_vec[] = {PGPTR_NULL, PGPTR_NULL, PGPTR_NULL, PGPTR_NULL, 
                               PGPTR_NULL, PGPTR_NULL, PGPTR_NULL, PGPTR_NULL, 
                               PGPTR_NULL, PGPTR_NULL, PGPTR_NULL, PGPTR_NULL};
        memcpy(inode_cache[empty_inode_idx].pages, page_vec, 12 * sizeof(uint32_t));
        inode_cache[empty_inode_idx].index_page = PGPTR_NULL;
        int ent_per_page = (BLOCK_SIZE / sizeof(dirent_t));
        if (empty_dir_idx < 12 * ent_per_page) {
            pageptr_t dir_pageid = inode_cache[0].pages[empty_dir_idx / ent_per_page];
            if (dir_pageid == PGPTR_NULL) {
                inode_cache[0].pages[empty_dir_idx / ent_per_page] = dalloc();
                for (int i = 0; (unsigned)i < (BLOCK_SIZE / sizeof(dirent_t)); i++) 
                    directory_cache[(empty_dir_idx / ent_per_page) * 
                                    (ent_per_page) + i].inode_index = INODE_NULL;
            }
        } else {
            if (inode_cache[0].index_page == PGPTR_NULL) {
                inode_cache[0].index_page = dalloc();
                for (int i = 0; (unsigned)i < (BLOCK_SIZE / sizeof(pageptr_t)); i++) 
                    idx_node_cache[i] = PGPTR_NULL;
            }
            if (idx_node_cache[empty_dir_idx / ent_per_page] == PGPTR_NULL) { 
                idx_node_cache[empty_dir_idx / ent_per_page] = dalloc();
                for (int i = 0; (unsigned)i < (BLOCK_SIZE / sizeof(dirent_t)); i++)
                    directory_cache[(empty_dir_idx / ent_per_page) * 
                                    (ent_per_page) + i].inode_index = INODE_NULL;
            }
        }
        directory_cache[empty_dir_idx].inode_index = empty_inode_idx;
        strcpy(directory_cache[empty_inode_idx].name, name);
        synch_idx_inode_cache(write);
        synch_directory(write);
        synch_inode(write);
        synch_bitmap(write);
    }
    int empty_fopen_index = INODE_NULL, file_index = INODE_NULL;
    for (int i = 0; i < NUM_DATA_BLOCKS; i++) {
        if (file_open_table[i].inode_idx == INODE_NULL) {
            empty_fopen_index = i;
            break;
        }
    }
    if (empty_fopen_index == INODE_NULL) {
        perror("file overflow\n");
        exit(1);
    }
    for (int i = 0; i < NUM_DATA_BLOCKS; i++) {
        if (!strcmp(directory_cache[i].name, name)) {
            file_index = i;
            break;
        }
    }
    if (file_index == INODE_NULL) {
        perror("Bad file\n");
        exit(0);
    }
    file_open_table[empty_fopen_index].inode_idx = directory_cache[file_index].inode_index;
    file_open_table[empty_fopen_index].read_ptr = 0;
    file_open_table[empty_fopen_index].write_ptr = inode_cache[directory_cache[file_index].inode_index].fsize;
    return 0;
}
