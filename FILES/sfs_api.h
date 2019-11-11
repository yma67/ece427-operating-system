#ifndef _SFS_API
#define _SFS_API

#include <stdint.h>
#include "disk_emu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DISC_NAME "sherry"
#define BLOCK_SIZE 1024
#define NUM_BLOCKS 1090
#define NUM_INODE_BLOCKS 64
#define NUM_DATA_BLOCKS 1024
#define NAME_LIM 27

#define PGPTR_NULL NUM_DATA_BLOCKS 
#define INODE_NULL NUM_DATA_BLOCKS 

typedef uint32_t iindex_t;
typedef uint32_t pageptr_t;

// In disc data structures

// Directory Entry: 32 Bytes 
typedef struct _dirent_t {
    char name[NAME_LIM + 1];
    iindex_t inode_index;
} dirent_t;

// Super Block
typedef struct _super_block_t {
    uint32_t magic_number;
    uint32_t page_size;
    uint32_t file_sys_size;
    uint32_t num_data_pages;
    iindex_t inode_root;
} super_block_t;

// I node
typedef struct _inode_t {
    uint32_t link_cnt;
    uint32_t uid;
    uint32_t fsize;
    pageptr_t pages[12];
    pageptr_t index_page;
} inode_t;

// Page with Type definition
typedef struct _page_t {
    union _content_t {
        // 256 entries
        pageptr_t index[BLOCK_SIZE / sizeof(pageptr_t)];
        // 32 entries
        dirent_t directory[BLOCK_SIZE / sizeof(dirent_t)];
        // 1024 Bytes
        uint8_t data[BLOCK_SIZE / sizeof(uint8_t)];
        // 16 inodes
        inode_t inode[BLOCK_SIZE / sizeof(inode_t)];
        uint8_t is_free[BLOCK_SIZE / sizeof(uint8_t)];
    } content;
} page_t;

// In memory Data structures
typedef struct _fopen_entry_t {
    uint32_t inode_idx;
    uint32_t read_ptr;
    uint32_t write_ptr;
} fopen_entry_t;

fopen_entry_t file_open_table[NUM_DATA_BLOCKS];
inode_t inode_cache[NUM_DATA_BLOCKS];
dirent_t directory_cache[NUM_DATA_BLOCKS];
uint8_t bitmap[NUM_DATA_BLOCKS];

super_block_t super_block;

void mksfs(int flags);


#endif
