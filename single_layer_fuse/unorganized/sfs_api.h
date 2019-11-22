/**
 * Single layer FUSE
 * Author: Yuxiang Ma
 *
 * NOTE
 * This file is edited under a directory mounted by 
 * a previous version of Single Layer FUSE!
 *
 */

#ifndef SFS_API_H
#define SFS_API_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "disk_emu.h"

/**
 * Assumption / constrains
 * 1. size of super block is less than the page size
 * 2. page size is divisible by size of i node
 * 3. data page have no extra bits
 * 4. bitmap fits within one page 
 */

#define DISC_NAME "sherry"
#define BLOCK_SIZE 1024
#define NUM_DATA_BLOCKS 4096
#define MAXFILENAME 20
#define FILELEN 28

/**
 * Common Data structures
 */

// Inode index
typedef uint32_t iindex_t;

// Page pointer + end marker
typedef struct _pageptr_t {
    uint16_t end;
    uint16_t pageid;
} pageptr_t;


/**
 * On disc data structures
 */

// Directory Entry: 32 Bytes 
typedef struct _dirent_t {
    char name[FILELEN];
    iindex_t inode_index;
} dirent_t;

// Super Block
typedef struct _super_block_t {
    char disc_name[FILELEN];
    uint32_t page_size;
    uint32_t file_sys_size;
    uint32_t num_data_pages;
    uint32_t num_inode_pages;
    iindex_t inode_root;
} super_block_t;

// I node
typedef struct _inode_t {
    uint32_t link_cnt;
    uint32_t eof;
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
        // bytemap
        uint8_t is_free[BLOCK_SIZE / sizeof(uint8_t)];
        // Place holder 1k24 bytes
        uint8_t _place_holder[BLOCK_SIZE / sizeof(uint8_t)];
    } content;
} page_t;


/**
 * Constant
 */

static const iindex_t INODE_NULL;
static const pageptr_t PGPTR_NULL;


/**
 * SFS api
 */

extern void mksfs(int flags);
extern int sfs_fopen(char *name);
extern int sfs_fwrite(int fileID, const char *buf, int length);
extern int sfs_fread(int fileID, char *buf, int length);
extern int sfs_fclose(int fileID);
extern int sfs_frseek(int fileID,int loc);
extern int sfs_fwseek(int fileID,int loc);
extern int sfs_remove(char *file);
extern int sfs_getfilesize(const char *path);
extern int sfs_getnextfilename(char *fname);


/**
 * To adopt tester
 */

static inline int sfs_GetFileSize(const char *path) {
    return sfs_getfilesize(path);
}

static inline int sfs_get_next_filename(char *path) {
    return sfs_getnextfilename(path);
}

#endif
