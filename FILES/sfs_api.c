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
                          1, directory_cache +                                 \
                          (12 + i) * (BLOCK_SIZE / sizeof(dirent_t)));         \
        }                                                                      \
    }                                                                          \
    free(page_buf);                                                            \
}

#define dalloc() ({                                                            \
    synch_bitmap(read);                                                        \
    uint32_t first_free = 0;                                                   \
    for (int i = 1; i < NUM_DATA_BLOCKS; i++) {                                \
        if (bitmap[first_free] == 0) {                                         \
            first_free = i;                                                    \
            break;                                                             \
        }                                                                      \
    }                                                                          \
    if (first_free == 0) {                                                     \
        perror("Disc Full\n");                                                 \
        INODE_NULL;                                                            \
    }                                                                          \
    bitmap[first_free] = 1;                                                    \
    synch_bitmap(write);                                                       \
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
        for (int i = 0; i < NUM_DATA_BLOCKS; i++)
            directory_cache[i].inode_index = INODE_NULL;
        synch_superblock(read);
        synch_inode(read);
        synch_idx_inode_cache(read);
        synch_directory(read);
        synch_bitmap(read);
    }
}

int sfs_open(char *name) {
    int is_fexist = 0, file_index = INODE_NULL;
    for (int i = 0; i < NUM_DATA_BLOCKS; i++) {
        if (!strcmp(name, directory_cache[i].name)) {
            is_fexist = 1;
            file_index = i;
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
            return -1;
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
                if (inode_cache[0].pages[empty_dir_idx / ent_per_page] == PGPTR_NULL) 
                    return -1;
                for (int i = 0; (unsigned)i < (BLOCK_SIZE / sizeof(dirent_t)); i++) 
                    directory_cache[(empty_dir_idx / ent_per_page) * 
                                    (ent_per_page) + i].inode_index = INODE_NULL;
            }
        } else {
            if (inode_cache[0].index_page == PGPTR_NULL) {
                inode_cache[0].index_page = dalloc();
                if (inode_cache[0].index_page == PGPTR_NULL) 
                    return -1;
                for (int i = 0; (unsigned)i < (BLOCK_SIZE / sizeof(pageptr_t)); i++) 
                    idx_node_cache[i] = PGPTR_NULL;
            }
            if (idx_node_cache[empty_dir_idx / ent_per_page] == PGPTR_NULL) { 
                idx_node_cache[empty_dir_idx / ent_per_page] = dalloc();
                if (idx_node_cache[empty_dir_idx / ent_per_page] == PGPTR_NULL)
                    return -1;
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
    } else {
        for (int i = 0; i < NUM_DATA_BLOCKS; i++) {
            if (!strcmp(file_open_table[i].fname, name)) {
                return i;
            }
        }
    }
    int empty_fopen_index = INODE_NULL;
    file_index = INODE_NULL;
    for (int i = 0; i < NUM_DATA_BLOCKS; i++) {
        if (file_open_table[i].inode_idx == INODE_NULL) {
            empty_fopen_index = i;
            break;
        }
    }
    if (empty_fopen_index == INODE_NULL) {
        perror("file overflow\n");
        return -1;
    }
    for (int i = 0; i < NUM_DATA_BLOCKS; i++) {
        if (!strcmp(directory_cache[i].name, name)) {
            file_index = i;
            break;
        }
    }
    if (file_index == INODE_NULL) {
        perror("Bad file\n");
        return -1;
    }
    file_open_table[empty_fopen_index].inode_idx = directory_cache[file_index].inode_index;
    file_open_table[empty_fopen_index].read_ptr = 0;
    file_open_table[empty_fopen_index].write_ptr = inode_cache[directory_cache[file_index].inode_index].fsize;
    strcpy(file_open_table[empty_fopen_index].fname, name);
    return empty_fopen_index;
}

int sfs_write(int fileID, char *buf, int length) {
    uint32_t byte_written = 0;
    page_t* page_buf = (page_t *)calloc(sizeof(page_t *), 1);
    while (byte_written < (unsigned)length) {
        uint32_t pos_in_page = file_open_table[fileID].write_ptr % BLOCK_SIZE;
        uint32_t leftover_in_page = (BLOCK_SIZE - pos_in_page) % BLOCK_SIZE;
        uint32_t nth_page = file_open_table[fileID].write_ptr / BLOCK_SIZE;
        pageptr_t to_write;
        if (nth_page < 12) {
            if (inode_cache[file_open_table[fileID].inode_idx].pages[nth_page] == PGPTR_NULL) {
                inode_cache[file_open_table[fileID].inode_idx].pages[nth_page] = dalloc();
                if (inode_cache[file_open_table[fileID].inode_idx].pages[nth_page] == PGPTR_NULL) {
                    return byte_written;
                }
            }
            to_write = inode_cache[file_open_table[fileID].inode_idx].pages[nth_page];
        } else {
            if (inode_cache[file_open_table[fileID].inode_idx].index_page == PGPTR_NULL) {
                inode_cache[file_open_table[fileID].inode_idx].index_page = dalloc();
                if (inode_cache[file_open_table[fileID].inode_idx].index_page == PGPTR_NULL) {
                    return byte_written;
                }
                for (int i = 0; (unsigned)i < (BLOCK_SIZE / sizeof(pageptr_t)); i++) 
                    page_buf->content.index[i] = PGPTR_NULL;
                write_blocks(1 + NUM_DATA_BLOCKS + 
                             inode_cache[file_open_table[fileID].inode_idx]
                             .index_page, 1, page_buf);
            }
            read_blocks(1 + NUM_DATA_BLOCKS +
                        inode_cache[file_open_table[fileID].inode_idx].index_page,
                        1, page_buf);
            if (page_buf->content.index[nth_page - 12] == PGPTR_NULL) {
                page_buf->content.index[nth_page - 12] = dalloc();
                if (page_buf->content.index[nth_page - 12] == PGPTR_NULL) 
                    return byte_written;
            }
            write_blocks(1 + NUM_DATA_BLOCKS +  
                         inode_cache[file_open_table[fileID].inode_idx].index_page,
                         1, page_buf);
            to_write = page_buf->content.index[nth_page - 12];
        }
        read_blocks(1 + NUM_DATA_BLOCKS + to_write, 1, page_buf);
        memcpy(page_buf + pos_in_page, buf, leftover_in_page);
        write_blocks(1 + NUM_DATA_BLOCKS + to_write, 1, page_buf);
        byte_written += leftover_in_page;
    }
    return byte_written;
}
