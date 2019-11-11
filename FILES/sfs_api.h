#ifndef _SFS_API
#define _SFS_API

#include <stdint.h>
#include "disk_emu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DISC_NAME "sherry"
#define BLOCK_SIZE 1024
#define NUM_BLOCKS 1076
#define NUM_INODE_BLOCKS 74
#define NUM_DATA_BLOCKS 1024
#define NAME_LIM 16
#define EXT_LIM 3

#define PGPTR_NULL NUM_DATA_BLOCKS 

typedef uint32_t iindex_t;
typedef uint32_t pageptr_t;

// In disc data structures
// Type of a page
typedef enum _page_type {
    DIRECTORY,
    DATA,
    INDEX 
} page_type;

// Data Page
typedef struct _data_t {
    uint8_t bytes[BLOCK_SIZE - 4];
} data_t;

// Directory Entry 
typedef struct _dirent_t {
    char name[NAME_LIM + 1];
    char ext[EXT_LIM + 1];
    iindex_t inode_index;
} dirent_t;

// Directory Table
typedef struct _dir_t {
    dirent_t dlist[(BLOCK_SIZE - 4) / sizeof(dirent_t)];  
} dir_t;

// Index Page
typedef struct _index_t {
    pageptr_t page_pointer[(BLOCK_SIZE - 4) / sizeof(uint32_t)];
} index_t;

// Page with Type definition
typedef struct _page_t {
    page_type type;
    union _content_t {
        index_t index_page;
        dir_t directory_page;
        data_t data_page;
        uint8_t _padding[BLOCK_SIZE - sizeof(page_type)];
    } content;
} page_t;

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
    uint32_t mode;
    uint32_t link_cnt;
    uint32_t uid;
    uint32_t gid;
    uint32_t fsize;
    pageptr_t pages[12];
    pageptr_t index_page;
} inode_t;

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
