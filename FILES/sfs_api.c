#include "sfs_api.h"

void mksfs(int flag) {
    void* page_buf = (void *)calloc(sizeof(uint8_t), BLOCK_SIZE);
    if (flag) {
        // TODO: Write Super Block
        init_fresh_disk(DISC_NAME, BLOCK_SIZE, NUM_BLOCKS);
        super_block_t _super_block = {
            .magic_number = 0xACBD0005, 
            .page_size = BLOCK_SIZE, 
            .file_sys_size = NUM_BLOCKS, 
            .num_data_pages = NUM_DATA_BLOCKS, 
            .inode_root = 0
        };
        super_block = _super_block;
        memcpy(page_buf, &super_block, sizeof(super_block_t));
        write_blocks(0, 1, page_buf);
        memset(page_buf, 0, sizeof(uint8_t) * BLOCK_SIZE);
        // TODO: Write root inode
        inode_cache[0].mode = 04000; 
        inode_cache[0].link_cnt = 1;
        inode_cache[0].uid = 0; 
        inode_cache[0].gid = 0;
        inode_cache[0].fsize = 0;
        uint32_t page_vec[] = {0         , PGPTR_NULL, PGPTR_NULL, PGPTR_NULL, 
                               PGPTR_NULL, PGPTR_NULL, PGPTR_NULL, PGPTR_NULL, 
                               PGPTR_NULL, PGPTR_NULL, PGPTR_NULL, PGPTR_NULL};
        memcpy(inode_cache[0].pages, page_vec, 12 * sizeof(uint32_t));
        inode_cache[0].index_page = PGPTR_NULL;
        memcpy(page_buf, &(inode_cache[0]), sizeof(inode_t));
        write_blocks(1, 0, page_buf);
        memset(page_buf, 0, sizeof(uint8_t) * BLOCK_SIZE); 
        // TODO: Write Bitmap
        bitmap[0] = 1;
        memcpy(page_buf, bitmap, sizeof(uint8_t) * NUM_DATA_BLOCKS);
        write_blocks(NUM_BLOCKS - 1, 1, page_buf);
        memset(page_buf, 0, sizeof(uint8_t) * BLOCK_SIZE); 
        // TODO: Write First Directory Page
        ((page_t *)page_buf)->type = DIRECTORY;
        write_blocks(1 + NUM_INODE_BLOCKS, 1, page_buf);
    } else {
        init_disk(DISC_NAME, BLOCK_SIZE, NUM_BLOCKS);
        // Super Block
        if (read_blocks(0, 1, page_buf) != 1) {
            perror("failed to read in super block\n");
            exit(1);
        }
        memcpy(&super_block, page_buf, sizeof(super_block));
        memset(page_buf, 0, sizeof(uint8_t) * BLOCK_SIZE);
        // I node
        for (int i = 0; i < NUM_INODE_BLOCKS; i++) {
            if (read_blocks(1 + i, 1, page_buf) != 1) {
                perror("failed to read in i nodes\n");
                exit(1);
            }
            memcpy(inode_cache + i * (BLOCK_SIZE / sizeof(inode_t)), 
                   page_buf, (BLOCK_SIZE / sizeof(inode_t)) * sizeof(inode_t));
            memset(page_buf, 0, sizeof(uint8_t) * BLOCK_SIZE);
        }
        // Directories
        for (int i = 0; i < 12; i++) {
            if (read_blocks(1 + NUM_INODE_BLOCKS + inode_cache[0].pages[i], 
                            1, page_buf) != 1) {
                perror("failed to read in directories\n");
                exit(1);
            }
            if (((page_t *)page_buf)->type != DIRECTORY) {
                perror("Bad Page Type\n");
                exit(1);
            }
            memcpy(directory_cache + i * ((BLOCK_SIZE - 4) / sizeof(dirent_t)), 
                   &(((page_t *)page_buf)->content), 
                   ((BLOCK_SIZE - 4) / sizeof(dirent_t)) * sizeof(dirent_t));
            memset(page_buf, 0, sizeof(uint8_t) * BLOCK_SIZE);
        }
        // Bit Map
        if (read_blocks(NUM_BLOCKS - 1, 1, page_buf) != 1) {
            perror("Failed to Read\n");
            exit(1);
        }
        memcpy(bitmap, page_buf, sizeof(uint8_t) * BLOCK_SIZE);
    }
    free(page_buf);
}
