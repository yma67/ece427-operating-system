/**
 * Single Layer FUSE
 * Author: Yuxiang Ma
 *
 * NOTE 
 * This file is edited under directory mounted by a 
 * previous version of single layer fuse!
 *
 */

#include "sfs_api.h"

/**
 * In memory data structure
 */

// File descriptor table entry
typedef struct _fopen_entry_t {
    char fname[FILELEN];
    uint32_t inode_idx;
    uint32_t read_ptr;
    uint32_t write_ptr;
} fopen_entry_t;

// File descriptor table
static fopen_entry_t file_open_table[NUM_DATA_BLOCKS];


/** 
 * Cache
 */

// All I-Node cache
static inode_t inode_cache[NUM_DATA_BLOCKS];
// All Directory Entry cache
static dirent_t directory_cache[NUM_DATA_BLOCKS];
// Bytemap cache
// sequential read is cheap, therefore no worries
static uint8_t bitmap[NUM_DATA_BLOCKS];
// Super Block cache
static super_block_t super_block = {
    .disc_name = "sherry",
    .page_size = BLOCK_SIZE,
    .file_sys_size = 1 + NUM_DATA_BLOCKS * sizeof(inode_t) / BLOCK_SIZE +
                     NUM_DATA_BLOCKS + NUM_DATA_BLOCKS / BLOCK_SIZE,
    .num_data_pages = NUM_DATA_BLOCKS,
    .num_inode_pages = NUM_DATA_BLOCKS * sizeof(inode_t) / BLOCK_SIZE,
    .inode_root = 0
};


/**
 * State variables
 */

// Directory Walker
static uint32_t num_de_files = 0;
static uint32_t cur_nth_file = 0;
static uint32_t current_file = 0;
// Page Buffer
static page_t page_buf;

// constant definition
static const iindex_t INODE_NULL = NUM_DATA_BLOCKS;
static const pageptr_t PGPTR_NULL = {
    .end = 0,
    .pageid = NUM_DATA_BLOCKS
};


/** 
 * Utility macro
 */

#define MIN(_a, _b) (_a < _b) ? (_a) : (_b)
#define MAX(_a, _b) (_a > _b) ? (_a) : (_b)


/**
 * Cache synchronization helper
 */

// invoke avec helper(read) or helper(write)
// inode cache sync: 1 random read + ? sequencial read
#define SYNCH_INODE(_opt) _opt##_blocks(1, super_block.num_inode_pages,           \
                                        inode_cache)
// superblock cache sync: 1 random read
#define SYNCH_SUPERBLOCK(_opt) _opt##_blocks(0, 1, &super_block)
// bitmap cache sync: 1 random read followed by 3 sequencial reads
#define SYNCH_BITMAP(_opt) _opt##_blocks(super_block.file_sys_size -              \
                                         super_block.num_data_pages /             \
                                         super_block.page_size,                   \
                                         super_block.num_data_pages /             \
                                         super_block.page_size, bitmap)                       
// directory sync: multiple random reads
#define SYNCH_DIRECTORY(_opt) {                                                   \
    memset(&page_buf, 0, sizeof(page_t));                                         \
    for (int i = 0; i < 12; i++) {                                                \
        if (inode_cache[0].pages[i].pageid == PGPTR_NULL.pageid)                  \
            continue;                                                             \
        _opt##_blocks(1 + super_block.num_inode_pages +                           \
                      inode_cache[0].pages[i].pageid,                             \
                      1, &directory_cache[i * (BLOCK_SIZE / sizeof(dirent_t))]);  \
    }                                                                             \
    if (inode_cache[0].index_page.pageid != PGPTR_NULL.pageid) {                  \
        read_blocks(1 + super_block.num_inode_pages +                             \
                    inode_cache[0].index_page.pageid, 1, &page_buf);              \
        for (int i = 0; (unsigned)i < (BLOCK_SIZE / sizeof(pageptr_t)); i++) {    \
            if (page_buf.content.index[i].pageid == PGPTR_NULL.pageid)            \
                continue;                                                         \
            _opt##_blocks(1 + super_block.num_inode_pages +                       \
                          page_buf.content.index[i].pageid, 1,                    \
                          &directory_cache[(12 + i) *                             \
                          (BLOCK_SIZE / sizeof(dirent_t))]);                      \
        }                                                                         \
    }                                                                             \
}

static inline uint16_t dalloc() { 
    SYNCH_BITMAP(read); 
    uint32_t first_free = 0;
    for (int i = 1; (unsigned)i < super_block.num_data_pages; i++) {
        if (bitmap[i] == 0) {
            first_free = i;
            break;
        }
    }
    if (first_free == 0) { 
        perror("Disc Full\n"); 
        return INODE_NULL; 
    } 
    bitmap[first_free] = 1; 
    SYNCH_BITMAP(write); 
    return first_free;
}

static inline uint32_t calc_fsize(int fd) {
    uint32_t byte_count = 0;
    if (inode_cache[file_open_table[fd].inode_idx].index_page.pageid != 
        PGPTR_NULL.pageid) {
        read_blocks(1 + super_block.num_inode_pages + 
                    inode_cache[file_open_table[fd].inode_idx]
                    .index_page.pageid, 1, &page_buf);
        for (int i = (BLOCK_SIZE) / sizeof(pageptr_t) - 1; i > -1; i--) 
            if (page_buf.content.index[i].pageid != PGPTR_NULL.pageid) 
                byte_count += page_buf.content.index[i].end;
    }
    for (int i = 0; i < 12; i++) 
        if (inode_cache[file_open_table[fd].inode_idx].pages[i].pageid !=
            PGPTR_NULL.pageid) 
            byte_count += inode_cache[file_open_table[fd].inode_idx]
                                      .pages[i].end;
    return byte_count;
}

void mksfs(int flag) {
    if (flag) {
        init_fresh_disk(super_block.disc_name, 
                        super_block.page_size, super_block.file_sys_size);
        // super block
        SYNCH_SUPERBLOCK(write);
        // i node
        inode_cache[0].link_cnt = 1;
        inode_cache[0].eof = 0; 
        inode_cache[0].fsize = 0;
        pageptr_t page_vec[] = {PGPTR_NULL, PGPTR_NULL, PGPTR_NULL, PGPTR_NULL, 
                                PGPTR_NULL, PGPTR_NULL, PGPTR_NULL, PGPTR_NULL, 
                                PGPTR_NULL, PGPTR_NULL, PGPTR_NULL, PGPTR_NULL};
        memcpy(inode_cache[0].pages, page_vec, 12 * sizeof(uint32_t));
        inode_cache[0].index_page = PGPTR_NULL;
        SYNCH_INODE(write);
        // bitmap
        SYNCH_BITMAP(write);
        // directory
        for (int i = 0; (unsigned)i < super_block.num_data_pages; i++)
            directory_cache[i].inode_index = INODE_NULL;
        SYNCH_DIRECTORY(write);
        // File Open Table
        for (int i = 0; (unsigned)i < super_block.num_data_pages; i++)
            file_open_table[i].inode_idx = INODE_NULL;
    } else {
        init_disk(super_block.disc_name, super_block.page_size, 
                  super_block.file_sys_size);
        // File Open Table 
        for (int i = 0; (unsigned)i < super_block.num_data_pages; i++)
            file_open_table[i].inode_idx = INODE_NULL;
        // initiallize directories
        for (int i = 0; (unsigned)i < super_block.num_data_pages; i++)
            directory_cache[i].inode_index = INODE_NULL;
        SYNCH_SUPERBLOCK(read);
        SYNCH_INODE(read);
        SYNCH_DIRECTORY(read);
        SYNCH_BITMAP(read);
    }
}

int sfs_fopen(char* name) {
    int is_fexist = 0, file_index = INODE_NULL;
    memset(&page_buf, 0, sizeof(page_t));
    for (int i = 0; (unsigned)i < super_block.num_data_pages; i++) {
        if (!strcmp(name, directory_cache[i].name)) {
            is_fexist = 1;
            file_index = i;
            break;
        }
    }
    // make new file
    if (!is_fexist) {
        if (strlen(name) > MAXFILENAME) {
            return -1;
        }
        // Find a good inode
        int empty_inode_idx = 0, empty_dir_idx = INODE_NULL;
        for (int i = 1; (unsigned)i < super_block.num_data_pages; i++) {
            if (inode_cache[i].link_cnt < 1) {
                empty_inode_idx = i;
                break;
            }
        }
        // Find a good directory entry
        for (int i = 0; (unsigned)i < super_block.num_data_pages; i++) {
            if (directory_cache[i].inode_index == INODE_NULL) {
                empty_dir_idx = i;
                break;
            }
        }
        // if no good inode, then max no. file reached
        if (empty_dir_idx == INODE_NULL || empty_inode_idx == 0) {
            perror("File Num overflow\n");
            return -1;
        }
        // new inode for file
        inode_cache[empty_inode_idx].link_cnt = 1;
        inode_cache[empty_inode_idx].eof = 0; 
        inode_cache[empty_inode_idx].fsize = 0;
        pageptr_t page_vec[] = {PGPTR_NULL, PGPTR_NULL, PGPTR_NULL, PGPTR_NULL, 
                                PGPTR_NULL, PGPTR_NULL, PGPTR_NULL, PGPTR_NULL, 
                                PGPTR_NULL, PGPTR_NULL, PGPTR_NULL, PGPTR_NULL};
        memcpy(inode_cache[empty_inode_idx].pages, page_vec, 12 * sizeof(uint32_t));
        inode_cache[empty_inode_idx].index_page = PGPTR_NULL;
        // Allocate directory enough directory block for the directory entry 
        // to be valid
        int ent_per_page = (BLOCK_SIZE / sizeof(dirent_t));
        // if the good directory block maps to some page pointed by 12 page pointers
        if (empty_dir_idx < 12 * ent_per_page) {
            pageptr_t dir_pageid = inode_cache[0].pages[empty_dir_idx / 
                                                        ent_per_page];
            // if corresponding pageid is NULL, allocate new page
            if (dir_pageid.pageid == PGPTR_NULL.pageid) {
                inode_cache[0].pages[empty_dir_idx / 
                                     ent_per_page].pageid = dalloc();
                // return ERROR if disc full
                if (inode_cache[0].pages[empty_dir_idx / ent_per_page]
                    .pageid == PGPTR_NULL.pageid) 
                    return -1;
                // write EMPTY directory entry marker to this new allocated page
                for (int i = 0; (unsigned)i < (BLOCK_SIZE/sizeof(dirent_t)); i++) 
                    directory_cache[(empty_dir_idx / ent_per_page) * 
                                    (ent_per_page) + i].inode_index = INODE_NULL;
            }
        } else {
            // or the directory block maps to some page pointed in index page
            // if no such index page, allocate one
            if (inode_cache[0].index_page.pageid == PGPTR_NULL.pageid) {
                inode_cache[0].index_page.pageid = dalloc();
                if (inode_cache[0].index_page.pageid == PGPTR_NULL.pageid) {
                    return -1;
                }
                // mark page pointer to null
                read_blocks(1 + super_block.num_inode_pages + 
                            inode_cache[0].index_page.pageid, 1, &page_buf);
                for (int i = 0; (unsigned)i < (BLOCK_SIZE/sizeof(pageptr_t)); i++)
                    page_buf.content.index[i] = PGPTR_NULL;
                // write initiallized index page to disc
                write_blocks(1 + super_block.num_inode_pages + 
                             inode_cache[0].index_page.pageid, 1, &page_buf);
            }
            // read original index page from disc
            read_blocks(1 + super_block.num_inode_pages + 
                        inode_cache[0].index_page.pageid, 1, &page_buf);
            // if corresponding page pointer is null, allocate new page
            if (page_buf.content.index[empty_dir_idx / ent_per_page].pageid == 
                PGPTR_NULL.pageid) { 
                page_buf.content.index[empty_dir_idx / 
                                       ent_per_page].pageid = dalloc();
                if (page_buf.content.index[empty_dir_idx / ent_per_page].pageid ==
                    PGPTR_NULL.pageid) 
                    return -1;
                // mark page pointer to null
                for (int i = 0; (unsigned)i < (BLOCK_SIZE/sizeof(dirent_t)); i++)
                    directory_cache[(empty_dir_idx / ent_per_page) * 
                                    (ent_per_page) + i].inode_index = INODE_NULL;
            }
            // write updated index page to disc
            write_blocks(1 + super_block.num_inode_pages + 
                         inode_cache[0].index_page.pageid, 1, &page_buf);
        }
        // update directory cache and write to disc
        num_de_files += 1;
        directory_cache[empty_dir_idx].inode_index = empty_inode_idx;
        strcpy(directory_cache[empty_dir_idx].name, name);
        SYNCH_DIRECTORY(write);
        SYNCH_INODE(write);
        SYNCH_BITMAP(write);
    } else {
        // if file opened, return its fd
        for (int i = 0; (unsigned)i < super_block.num_data_pages; i++) {
            if (!strcmp(file_open_table[i].fname, name)) {
                return i;
            }
        }
    }
    int empty_fopen_index = INODE_NULL;
    file_index = INODE_NULL;
    // find a slot in file descriptor table
    for (int i = 0; (unsigned)i < super_block.num_data_pages; i++) {
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
    for (int i = 0; (unsigned)i < super_block.num_data_pages; i++) {
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
    file_open_table[empty_fopen_index].inode_idx = directory_cache[file_index]
                                                   .inode_index;
    file_open_table[empty_fopen_index].read_ptr = 0;
    strcpy(file_open_table[empty_fopen_index].fname, name);
    file_open_table[empty_fopen_index].write_ptr = 
                    inode_cache[file_open_table[empty_fopen_index]
                                .inode_idx].eof; 
    return empty_fopen_index;
}

int sfs_fwrite(int fileID, const char* buf, int length) {
    if (file_open_table[fileID].inode_idx == INODE_NULL) 
        return 0;
    memset(&page_buf, 0, sizeof(page_t));
    uint32_t byte_written = 0;
    int rc = 0;
    while (byte_written < (unsigned)length && 
           file_open_table[fileID].write_ptr < 
           BLOCK_SIZE * (12 + BLOCK_SIZE / sizeof(pageptr_t))) {
        uint32_t pos_in_page = file_open_table[fileID].write_ptr % BLOCK_SIZE;
        uint32_t leftover_in_page = (BLOCK_SIZE - pos_in_page);
        uint32_t nth_page = file_open_table[fileID].write_ptr / BLOCK_SIZE;
        uint32_t b2w = MIN(leftover_in_page, length - byte_written);
        pageptr_t to_write;
        if (nth_page < 12) {
            if (inode_cache[file_open_table[fileID].inode_idx]
                .pages[nth_page].pageid == PGPTR_NULL.pageid) {
                inode_cache[file_open_table[fileID].inode_idx]
                            .pages[nth_page].pageid = dalloc();
                if (inode_cache[file_open_table[fileID].inode_idx]
                    .pages[nth_page].pageid == PGPTR_NULL.pageid) 
                    break;
            }
            inode_cache[file_open_table[fileID].inode_idx]
                        .pages[nth_page].end = MAX(pos_in_page + b2w, 
                        inode_cache[file_open_table[fileID].inode_idx]
                        .pages[nth_page].end);
            to_write = inode_cache[file_open_table[fileID].inode_idx]
                                   .pages[nth_page];
        } else {
            if (inode_cache[file_open_table[fileID].inode_idx]
                .index_page.pageid == PGPTR_NULL.pageid) {
                inode_cache[file_open_table[fileID].inode_idx]
                            .index_page.pageid = dalloc();
                if (inode_cache[file_open_table[fileID].inode_idx]
                    .index_page.pageid == PGPTR_NULL.pageid) 
                    break;
                for (int i = 0; (unsigned)i < (BLOCK_SIZE/sizeof(pageptr_t)); i++)
                    page_buf.content.index[i] = PGPTR_NULL;
                rc = write_blocks(1 + super_block.num_inode_pages + 
                                  inode_cache[file_open_table[fileID].inode_idx]
                                  .index_page.pageid, 1, &page_buf);
                if (rc != 1) 
                    break;
            }
            rc = read_blocks(1 + super_block.num_inode_pages +
                             inode_cache[file_open_table[fileID].inode_idx]
                             .index_page.pageid, 1, &page_buf);
            if (rc != 1) 
                break;
            if (page_buf.content.index[nth_page - 12].pageid == 
                PGPTR_NULL.pageid) {
                page_buf.content.index[nth_page - 12].pageid = dalloc();
                if (page_buf.content.index[nth_page - 12].pageid == 
                    PGPTR_NULL.pageid) 
                    break;
            }
            page_buf.content.index[nth_page - 12].end = MAX(pos_in_page + b2w, 
                                                            page_buf.content
                                                            .index[nth_page - 12]
                                                            .end);
            rc = write_blocks(1 + super_block.num_inode_pages +  
                              inode_cache[file_open_table[fileID].inode_idx]
                              .index_page.pageid, 1, &page_buf);
            if (rc != 1) 
                break;
            to_write = page_buf.content.index[nth_page - 12];
        }
        rc = read_blocks(1 + super_block.num_inode_pages + to_write.pageid, 1, &page_buf);
        if (rc != 1) 
            break;
        memcpy(&(page_buf.content.data[pos_in_page]), buf + byte_written, b2w);
        rc = write_blocks(1 + super_block.num_inode_pages + to_write.pageid, 1, &page_buf);
        if (rc != 1) 
            break;
        byte_written += b2w;
        file_open_table[fileID].write_ptr += b2w;
    }
    inode_cache[file_open_table[fileID].inode_idx].fsize = calc_fsize(fileID); 
    inode_cache[file_open_table[fileID].inode_idx].eof = MAX(file_open_table[fileID]
                                                             .write_ptr, inode_cache[
                                                             file_open_table[fileID]
                                                             .inode_idx].eof);
    SYNCH_INODE(write);
    return byte_written;
}

int sfs_fread(int fileID, char* buf, int length) {
    if (file_open_table[fileID].inode_idx == INODE_NULL)
        return 0;
    memset(&page_buf, 0, sizeof(page_t));
    uint32_t byte_read = 0, rcount = 0;
    while (byte_read < (unsigned)length && file_open_table[fileID].read_ptr < 
           inode_cache[file_open_table[fileID].inode_idx].eof) {
        uint32_t pos_in_page = file_open_table[fileID].read_ptr % BLOCK_SIZE;
        uint32_t leftover_in_page = (BLOCK_SIZE - pos_in_page);
        uint32_t nth_page = file_open_table[fileID].read_ptr / BLOCK_SIZE;
        uint32_t b2r = MIN(MIN(leftover_in_page, length - byte_read), 
                           inode_cache[file_open_table[fileID].inode_idx].eof - 
                           file_open_table[fileID].read_ptr);
        pageptr_t to_read;
        if (nth_page < 12) {
            if (inode_cache[file_open_table[fileID].inode_idx].pages[nth_page]
                .pageid == PGPTR_NULL.pageid) {
                memset(buf + byte_read, 0, b2r * sizeof(char));
                goto FINISH_READ;
            } 
            to_read = inode_cache[file_open_table[fileID].inode_idx]
                                  .pages[nth_page];
        } else {
            if (inode_cache[file_open_table[fileID].inode_idx]
                .index_page.pageid == PGPTR_NULL.pageid) {
                memset(buf + byte_read, 0, b2r * sizeof(char));
                goto FINISH_READ;
            }
            read_blocks(1 + super_block.num_inode_pages +
                        inode_cache[file_open_table[fileID].inode_idx].index_page
                        .pageid, 1, &page_buf);
            if (page_buf.content.index[nth_page - 12].pageid == 
                PGPTR_NULL.pageid) {
                memset(buf + byte_read, 0, b2r * sizeof(char));
                goto FINISH_READ;
            }
            to_read = page_buf.content.index[nth_page - 12];
        }
        read_blocks(1 + super_block.num_inode_pages + to_read.pageid, 1, &page_buf);
        memcpy(buf + byte_read, (&page_buf.content.data[pos_in_page]), b2r);
FINISH_READ:
        byte_read += b2r;
        file_open_table[fileID].read_ptr += b2r;
        rcount += 1;
    }
    return byte_read;
}

int sfs_fclose(int fileID) {
    if (file_open_table[fileID].inode_idx != INODE_NULL) {
        memset(&file_open_table[fileID], 0, sizeof(fopen_entry_t));
        file_open_table[fileID].inode_idx = INODE_NULL;
        return 0;
    }
    return -1;
}

int sfs_frseek(int fileID, int loc) {
    if (file_open_table[fileID].inode_idx != INODE_NULL) {
        if (loc < 0 || (unsigned)loc > 
            inode_cache[file_open_table[fileID].inode_idx].eof)
            return -1;
        file_open_table[fileID].read_ptr = (unsigned)(loc); 
        return 0;
    }
    return -1;
}

int sfs_fwseek(int fileID, int loc) {
    if (file_open_table[fileID].inode_idx != INODE_NULL && 
        0 <= loc && (unsigned)loc <= 
        BLOCK_SIZE * (12 + BLOCK_SIZE / sizeof(pageptr_t))) {
        file_open_table[fileID].write_ptr = (unsigned)(loc); 
        return 0;
    }
    return -1;
}

int sfs_remove(char* file) {
    for (int i = 0; (unsigned)i < super_block.num_data_pages; i++) {
        if (!strcmp(directory_cache[i].name, file)) {
            for (int j = 0; (unsigned)j < super_block.num_data_pages; j++) 
                if (!strcmp(file, file_open_table[j].fname)) 
                    sfs_fclose(j);
            memset(&inode_cache[directory_cache[i].inode_index], 0, 
                   sizeof(inode_t));
            memset(&directory_cache[i], 0, sizeof(dirent_t));
            directory_cache[i].inode_index = INODE_NULL;
            num_de_files -= 1; 
            SYNCH_DIRECTORY(write);
            SYNCH_INODE(write);
            return 0;
        }
    }
    return -1;
}

int sfs_getnextfilename(char* fname) {
    if (cur_nth_file >= num_de_files) {
        cur_nth_file = 0;
        return 0;
    }
    for (int i = 0; (unsigned)i < super_block.num_data_pages; i++) {
        if (directory_cache[(current_file + i) % 
            super_block.num_data_pages].inode_index != INODE_NULL) {
            strcpy(fname, directory_cache[(current_file + i) % 
                   super_block.num_data_pages].name);
            current_file = (current_file + i + 1) % 
                           super_block.num_data_pages;
            cur_nth_file += 1;
            return 1;
        }
    }
    return 0;
}

int sfs_getfilesize(const char* path) {
    for (int i = 0; (unsigned)i < super_block.num_data_pages; i++) 
        if (!strcmp(path, directory_cache[i].name)) 
            return inode_cache[directory_cache[i].inode_index].fsize;
    return -1;
}
