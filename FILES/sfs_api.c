#include "sfs_api.h"

#define min(_a, _b) (_a < _b) ? (_a) : (_b)
#define max(_a, _b) (_a > _b) ? (_b) : (_a)
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
    for (int i = 0; i < 12; i++) {                                             \
        if (inode_cache[0].pages[i].pageid == PGPTR_NULL.pageid)               \
            continue;                                                          \
        _opt##_blocks(1 + NUM_INODE_BLOCKS + inode_cache[0].pages[i].pageid,   \
                      1, directory_cache + i * (BLOCK_SIZE/sizeof(dirent_t))); \
    }                                                                          \
    if (inode_cache[0].index_page.pageid != PGPTR_NULL.pageid) {               \
        read_blocks(1 + NUM_INODE_BLOCKS + inode_cache[0].index_page.pageid,   \
                    1, page_buf);                                              \
        for (int i = 0; (unsigned long)i < (NUM_DATA_BLOCKS -                  \
                         12 * (BLOCK_SIZE / sizeof(dirent_t))) /               \
                         (BLOCK_SIZE / sizeof(dirent_t)); i++) {               \
            if (page_buf->content.index[i].pageid == PGPTR_NULL.pageid)        \
                continue;                                                      \
            _opt##_blocks(1 + NUM_INODE_BLOCKS + page_buf->content.index[i]    \
                          .pageid, 1, directory_cache +                        \
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
        pageptr_t page_vec[] = {{0, 0},     PGPTR_NULL, PGPTR_NULL, PGPTR_NULL, 
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
        // initiallize directories
        for (int i = 0; i < NUM_DATA_BLOCKS; i++)
            directory_cache[i].inode_index = INODE_NULL;
        synch_superblock(read);
        synch_inode(read);
        synch_directory(read);
        synch_bitmap(read);
    }
}

int sfs_open(char *name) {
    int is_fexist = 0, file_index = INODE_NULL;
    page_t* page_buf = (page_t *)calloc(sizeof(page_t), 1);
    for (int i = 0; i < NUM_DATA_BLOCKS; i++) {
        if (!strcmp(name, directory_cache[i].name)) {
            is_fexist = 1;
            file_index = i;
            break;
        }
    }
    // make new file
    if (!is_fexist) {
        // Find a good inode
        int empty_inode_idx = 0, empty_dir_idx = PGPTR_NULL.pageid;
        for (int i = 1; i < NUM_DATA_BLOCKS; i++) {
            if (inode_cache[i].link_cnt < 1) {
                empty_inode_idx = i;
                break;
            }
        }
        // Find a good directory entry
        for (int i = 0; i < NUM_DATA_BLOCKS; i++) {
            if (directory_cache[i].inode_index == INODE_NULL) {
                empty_dir_idx = i;
                break;
            }
        }
        // if no good inode, then max no. file reached
        if (empty_dir_idx == PGPTR_NULL.pageid || empty_inode_idx == 0) {
            perror("File Num overflow\n");
            return -1;
        }
        // new inode for file
        inode_cache[empty_inode_idx].link_cnt = 1;
        inode_cache[empty_inode_idx].uid = 0; 
        inode_cache[empty_inode_idx].fsize = 0;
        pageptr_t page_vec[] = {PGPTR_NULL, PGPTR_NULL, PGPTR_NULL, PGPTR_NULL, 
                                PGPTR_NULL, PGPTR_NULL, PGPTR_NULL, PGPTR_NULL, 
                                PGPTR_NULL, PGPTR_NULL, PGPTR_NULL, PGPTR_NULL};
        memcpy(inode_cache[empty_inode_idx].pages, page_vec, 12 * sizeof(uint32_t));
        inode_cache[empty_inode_idx].index_page = PGPTR_NULL;
        // Allocate directory enough directory block for the directory entry to be valid
        int ent_per_page = (BLOCK_SIZE / sizeof(dirent_t));
        // if the good directory block maps to some page pointed by 12 page pointers
        if (empty_dir_idx < 12 * ent_per_page) {
            pageptr_t dir_pageid = inode_cache[0].pages[empty_dir_idx / ent_per_page];
            // if corresponding pageid is NULL, allocate new page
            if (dir_pageid.pageid == PGPTR_NULL.pageid) {
                inode_cache[0].pages[empty_dir_idx / ent_per_page].pageid = dalloc();
                // return ERROR if disc full
                if (inode_cache[0].pages[empty_dir_idx / ent_per_page].pageid == PGPTR_NULL.pageid) 
                    return -1;
                // write EMPTY directory entry marker to this new allocated page
                for (int i = 0; (unsigned)i < (BLOCK_SIZE / sizeof(dirent_t)); i++) 
                    directory_cache[(empty_dir_idx / ent_per_page) * 
                                    (ent_per_page) + i].inode_index = INODE_NULL;
            }
        } else {
            // or the directory block maps to some page pointed in index page
            // if no such index page, allocate one
            if (inode_cache[0].index_page.pageid == PGPTR_NULL.pageid) {
                inode_cache[0].index_page.pageid = dalloc();
                if (inode_cache[0].index_page.pageid == PGPTR_NULL.pageid) 
                    return -1;
                // mark page pointer to null
                read_blocks(1 + NUM_INODE_BLOCKS + inode_cache[0].index_page.pageid, 
                            1, page_buf);
                for (int i = 0; (unsigned)i < (BLOCK_SIZE / sizeof(pageptr_t)); i++) 
                    page_buf->content.index[i] = PGPTR_NULL;
                // write initiallized index page to disc
                write_blocks(1 + NUM_INODE_BLOCKS + inode_cache[0].index_page.pageid, 
                             1, page_buf);
            }
            // read original index page from disc
            read_blocks(1 + NUM_INODE_BLOCKS + inode_cache[0].index_page.pageid, 
                        1, page_buf);
            // if corresponding page pointer is null, allocate new page
            if (page_buf->content.index[empty_dir_idx / ent_per_page].pageid == PGPTR_NULL.pageid) { 
                page_buf->content.index[empty_dir_idx / ent_per_page].pageid = dalloc();
                if (page_buf->content.index[empty_dir_idx / ent_per_page].pageid == PGPTR_NULL.pageid)
                    return -1;
                // mark page pointer to null
                for (int i = 0; (unsigned)i < (BLOCK_SIZE / sizeof(dirent_t)); i++)
                    directory_cache[(empty_dir_idx / ent_per_page) * 
                                    (ent_per_page) + i].inode_index = INODE_NULL;
            }
            // write updated index page to disc
            write_blocks(1 + NUM_INODE_BLOCKS + inode_cache[0].index_page.pageid, 1, page_buf);
        }
        // update directory cache and write to disc
        directory_cache[empty_dir_idx].inode_index = empty_inode_idx;
        strcpy(directory_cache[empty_inode_idx].name, name);
        synch_directory(write);
        synch_inode(write);
        synch_bitmap(write);
    } else {
        // if file opened, return its fd
        for (int i = 0; i < NUM_DATA_BLOCKS; i++) {
            if (!strcmp(file_open_table[i].fname, name)) {
                return i;
            }
        }
    }
    int empty_fopen_index = INODE_NULL;
    file_index = INODE_NULL;
    // find a slot in file descriptor table
    for (int i = 0; i < NUM_DATA_BLOCKS; i++) {
        if (file_open_table[i].inode_idx == INODE_NULL) {
            empty_fopen_index = i;
            break;
        }
    }
    // if full return
    if (empty_fopen_index == INODE_NULL) {
        perror("file overflow\n");
        return -1;
    }
    // find a slot in directory
    for (int i = 0; i < NUM_DATA_BLOCKS; i++) {
        if (!strcmp(directory_cache[i].name, name)) {
            file_index = i;
            break;
        }
    }
    // return on full
    if (file_index == INODE_NULL) {
        perror("Bad file\n");
        return -1;
    }
    // update file descriptor table
    file_open_table[empty_fopen_index].inode_idx = directory_cache[file_index].inode_index;
    file_open_table[empty_fopen_index].read_ptr = 0;
    strcpy(file_open_table[empty_fopen_index].fname, name);
    pageptr_t last_pos = {1024, 1024};
    if (inode_cache[directory_cache[file_index].inode_index].index_page.pageid != PGPTR_NULL.pageid) {
        read_blocks(1 + NUM_DATA_BLOCKS + 
                    inode_cache[directory_cache[file_index].inode_index]
                    .index_page.pageid, 1, page_buf);
        for (int i = (NUM_DATA_BLOCKS - 12 * (BLOCK_SIZE / sizeof(dirent_t))) / 
                     (BLOCK_SIZE / sizeof(dirent_t)) - 1; i > -1; i++) {
            if (page_buf->content.index[i].pageid != PGPTR_NULL.pageid) {
                last_pos = page_buf->content.index[i];
                break;
            }
        }
    }
    if (last_pos.pageid == PGPTR_NULL.pageid) {
        for (int i = 0; i < 12; i++) {
            if (inode_cache[directory_cache[file_index].inode_index].pages[i].pageid != PGPTR_NULL.pageid) {
                last_pos = inode_cache[directory_cache[file_index].inode_index].pages[i];
                break;
            }
        }
    }
    if (last_pos.pageid == PGPTR_NULL.pageid)
        file_open_table[empty_fopen_index].write_ptr = 0;
    else 
        file_open_table[empty_fopen_index].write_ptr = last_pos.end;
    free(page_buf);
    return empty_fopen_index;
}

int sfs_write(int fileID, char *buf, int length) {
    if (file_open_table[fileID].inode_idx == INODE_NULL) 
        return 0;
    uint32_t byte_written = 0;
    uint32_t write_ptr_prev = file_open_table[fileID].write_ptr;
    page_t* page_buf = (page_t *)calloc(sizeof(page_t *), 1);
    while (byte_written < (unsigned)length) {
        uint32_t pos_in_page = file_open_table[fileID].write_ptr % BLOCK_SIZE;
        uint32_t leftover_in_page = (BLOCK_SIZE - pos_in_page) % BLOCK_SIZE;
        uint32_t nth_page = file_open_table[fileID].write_ptr / BLOCK_SIZE;
        uint32_t b2w = min(leftover_in_page, length - byte_written);
        pageptr_t to_write;
        if (nth_page < 12) {
            if (inode_cache[file_open_table[fileID].inode_idx].pages[nth_page].pageid == PGPTR_NULL.pageid) {
                inode_cache[file_open_table[fileID].inode_idx].pages[nth_page].pageid = dalloc();
                if (inode_cache[file_open_table[fileID].inode_idx].pages[nth_page].pageid == PGPTR_NULL.pageid) 
                    break;
            }
            inode_cache[file_open_table[fileID].inode_idx].pages[nth_page].end = pos_in_page + b2w;
            to_write = inode_cache[file_open_table[fileID].inode_idx].pages[nth_page];
        } else {
            if (inode_cache[file_open_table[fileID].inode_idx].index_page.pageid == PGPTR_NULL.pageid) {
                inode_cache[file_open_table[fileID].inode_idx].index_page.pageid = dalloc();
                if (inode_cache[file_open_table[fileID].inode_idx].index_page.pageid == PGPTR_NULL.pageid) 
                    break;
                for (int i = 0; (unsigned)i < (BLOCK_SIZE / sizeof(pageptr_t)); i++) 
                    page_buf->content.index[i] = PGPTR_NULL;
                write_blocks(1 + NUM_DATA_BLOCKS + 
                             inode_cache[file_open_table[fileID].inode_idx].index_page.pageid, 1, page_buf);
            }
            read_blocks(1 + NUM_DATA_BLOCKS +
                        inode_cache[file_open_table[fileID].inode_idx].index_page.pageid, 1, page_buf);
            if (page_buf->content.index[nth_page - 12].pageid == PGPTR_NULL.pageid) {
                page_buf->content.index[nth_page - 12].pageid = dalloc();
                if (page_buf->content.index[nth_page - 12].pageid == PGPTR_NULL.pageid) 
                    break;
            }
            page_buf->content.index[nth_page - 12].end = pos_in_page + b2w;
            write_blocks(1 + NUM_DATA_BLOCKS +  
                         inode_cache[file_open_table[fileID].inode_idx].index_page.pageid, 1, page_buf);
            to_write = page_buf->content.index[nth_page - 12];
        }
        read_blocks(1 + NUM_DATA_BLOCKS + to_write.pageid, 1, page_buf);
        memcpy(page_buf + pos_in_page, buf, b2w);
        write_blocks(1 + NUM_DATA_BLOCKS + to_write.pageid, 1, page_buf);
        byte_written += b2w;
        file_open_table[fileID].write_ptr += b2w;
        uint32_t page_count = 0;
        if (inode_cache[file_open_table[fileID].inode_idx].index_page.pageid != PGPTR_NULL.pageid) {
            read_blocks(1 + NUM_DATA_BLOCKS + 
                        inode_cache[file_open_table[fileID].inode_idx]
                        .index_page.pageid, 1, page_buf);
            for (int i = (NUM_DATA_BLOCKS - 12 * (BLOCK_SIZE / sizeof(dirent_t))) / 
                         (BLOCK_SIZE / sizeof(dirent_t)) - 1; i > -1; i++) {
                if (page_buf->content.index[i].pageid != PGPTR_NULL.pageid) {
                    page_count += 1;
                }
            }
        }
        for (int i = 0; i < 12; i++) {
            if (inode_cache[file_open_table[fileID].inode_idx].pages[i].pageid != PGPTR_NULL.pageid) {
                page_count += 1;
            }
        }
        inode_cache[file_open_table[fileID].inode_idx].fsize = page_count * BLOCK_SIZE;
        synch_inode(write);
    }
    if (byte_written + write_ptr_prev)
    free(page_buf);
    return byte_written;
}

int sfs_fread(int fileID, char *buf, int length) {
    if (file_open_table[fileID].inode_idx == INODE_NULL)
        return 0;
    page_t* page_buf = (page_t *)calloc(sizeof(page_t), 1);
    pageptr_t last_pos = {1024, 1024};
    uint32_t flen = 0;
    if (inode_cache[file_open_table[fileID].inode_idx].index_page.pageid != PGPTR_NULL.pageid) {
        read_blocks(1 + NUM_DATA_BLOCKS + 
                    inode_cache[file_open_table[fileID].inode_idx].index_page.pageid, 1, page_buf);
        for (int i = (NUM_DATA_BLOCKS - 12 * (BLOCK_SIZE / sizeof(dirent_t))) / 
                     (BLOCK_SIZE / sizeof(dirent_t)) - 1; i > -1; i++) {
            if (page_buf->content.index[i].pageid != PGPTR_NULL.pageid) {
                last_pos = page_buf->content.index[i];
                flen = 12 + i;
                break;
            }
        }
    }
    if (last_pos.pageid == PGPTR_NULL.pageid) {
        for (int i = 0; i < 12; i++) {
            if (inode_cache[file_open_table[fileID].inode_idx].pages[i].pageid != PGPTR_NULL.pageid) {
                last_pos = inode_cache[file_open_table[fileID].inode_idx].pages[i];
                flen = i;
                break;
            }
        }
    }
    uint32_t byte_read = 0;
    uint32_t eof = last_pos.end + flen * BLOCK_SIZE;
    while (byte_read < (unsigned)length && file_open_table[fileID].read_ptr < eof) {
        uint32_t pos_in_page = file_open_table[fileID].read_ptr % BLOCK_SIZE;
        uint32_t leftover_in_page = (BLOCK_SIZE - pos_in_page) % BLOCK_SIZE;
        uint32_t nth_page = file_open_table[fileID].read_ptr / BLOCK_SIZE;
        uint32_t b2r = min(min(leftover_in_page, length - byte_read), eof - file_open_table[fileID].read_ptr);
        pageptr_t to_read;
        if (nth_page < 12) {
            if (inode_cache[file_open_table[fileID].inode_idx].pages[nth_page].pageid == PGPTR_NULL.pageid) {
                memset(buf + byte_read, 0, b2r);
                goto FINISH_READ;
            } 
            to_read = inode_cache[file_open_table[fileID].inode_idx].pages[nth_page];
        } else {
            if (inode_cache[file_open_table[fileID].inode_idx].index_page.pageid == PGPTR_NULL.pageid) {
                memset(buf + byte_read, 0, b2r);
                goto FINISH_READ;
            }
            read_blocks(1 + NUM_DATA_BLOCKS +
                        inode_cache[file_open_table[fileID].inode_idx].index_page.pageid, 1, page_buf);
            if (page_buf->content.index[nth_page - 12].pageid == PGPTR_NULL.pageid) {
                memset(buf + byte_read, 0, b2r);
                goto FINISH_READ;
            }
            to_read = page_buf->content.index[nth_page - 12];
        }
        read_blocks(1 + NUM_DATA_BLOCKS + to_read.pageid, 1, page_buf);
        memcpy(buf + byte_read, page_buf + pos_in_page, b2r);
FINISH_READ:
        byte_read += b2r;
        file_open_table[fileID].read_ptr += b2r;
    }
   free(page_buf);
   return byte_read;
}
