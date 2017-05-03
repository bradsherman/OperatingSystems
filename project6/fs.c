
#include "fs.h"
#include "disk.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

#define FS_MAGIC           0xf0f03410
#define INODES_PER_BLOCK   128
#define POINTERS_PER_INODE 5
#define POINTERS_PER_BLOCK 1024

int * FREE_BLOCK_MAP;
int MOUNTED = 0;

struct fs_superblock {
    int magic;
    int nblocks;
    int ninodeblocks;
    int ninodes;
};

struct fs_inode {
    int isvalid;
    int size;
    int direct[POINTERS_PER_INODE];
    int indirect;
};

union fs_block {
    struct fs_superblock super;
    struct fs_inode inode[INODES_PER_BLOCK];
    int pointers[POINTERS_PER_BLOCK];
    char data[DISK_BLOCK_SIZE];
};

void inode_load( int inumber, struct fs_inode *inode) {
    int inode_block = inumber / INODES_PER_BLOCK + 1;
    int inode_num = inumber % INODES_PER_BLOCK;
    union fs_block x;
    disk_read(inode_block, x.data);
    memcpy(inode, &x.inode[inode_num], sizeof(struct fs_inode));
}

void inode_save( int inumber, struct fs_inode *inode) {
    int inode_block = inumber / INODES_PER_BLOCK + 1;
    int inode_num = inumber % INODES_PER_BLOCK;
    union fs_block x;
    disk_read(inode_block, x.data);
    memcpy(&x.inode[inode_num], inode, sizeof(struct fs_inode));
    disk_write(inode_block, x.data);
}

int find_free_block() {
    union fs_block sblock;
    disk_read(0,sblock.data);
    int i;
    for(i = 1; i < sblock.super.nblocks; i++) {
        if(FREE_BLOCK_MAP[i] == 0) {
            FREE_BLOCK_MAP[i] = 1;
            return i;
        }
    }
    return -1;
}

int find_free_indirect_block(struct fs_inode *inode) {
    int block_num = find_free_block();
    if (block_num == -1) {
        return - 1;
    }
    union fs_block block;
    int i;
    inode->indirect = block_num;
    disk_read(block_num, block.data);
    for (i = 0; i < POINTERS_PER_BLOCK; i++) {
        block.pointers[i] = 0;
    }
    disk_write(block_num, block.data);
    return block_num;
}

int read_inode_data(int block_num, int offset, int length, char *data) {
    union fs_block block;
    int return_length;
    int data_length = DISK_BLOCK_SIZE - offset;
    int len = length < data_length ? length : data_length;
    if (strlen(data) + len == 16384) {
        len--;
    }
    disk_read(block_num, block.data);
    strncat(data, block.data + offset, len);
    return_length = length < data_length ? 0 : length - data_length;
    return return_length;
}

int write_inode_data(int block_num, int offset, int length, const char **data) {
    union fs_block block;
    int return_length;
    int data_length = DISK_BLOCK_SIZE - offset;
    int len = length < data_length ? length : data_length;
    disk_read(block_num, block.data);
    strncpy(block.data + offset, *data, len);
    *data = *data + len;
    disk_write(block_num, block.data);
    return_length = length < data_length ? 0 : length - data_length;
    return return_length;
}


int fs_format()
{
    // check to see if disk is mounted
    if(MOUNTED) {
        printf("disk already mounted\n");
        return 0;
    }

    // if not mounted, clear the disk, reset inode table
    union fs_block block;
    disk_read(0,block.data);
    // loop through all previous inodes and invalidate them
    int i;
    for(i = 1; i <= block.super.ninodeblocks; i++) {
        union fs_block inode;
        disk_read(i,inode.data);
        // loop through each inode in this block
        int j;
        for(j = 0; j < INODES_PER_BLOCK; j++) {
            inode.inode[j].isvalid = 0;
        }
        disk_write(i,inode.data);
    }

    // otherwise setup new superblock
    union fs_block sblock;
    sblock.super.magic = FS_MAGIC;
    int nblocks = disk_size();
    sblock.super.nblocks = nblocks;
    int ninodes = nblocks/10;
    if(ninodes == 0) ninodes = 1;
    sblock.super.ninodeblocks = ninodes;
    sblock.super.ninodes = INODES_PER_BLOCK;
    disk_write(0,sblock.data);

    return 1;
}

void fs_debug()
{
    union fs_block block;

    disk_read(0,block.data);

    printf("superblock:\n");
    printf("    %d blocks\n",block.super.nblocks);
    printf("    %d inode blocks\n",block.super.ninodeblocks);
    printf("    %d inodes\n",block.super.ninodes);

    // loop through all inode blocks
    int i;
    for(i = 1; i <= block.super.ninodeblocks; i++) {
        union fs_block inode;
        disk_read(i,inode.data);
        // loop through each inode in this block
        int j;
        for(j = 0; j < INODES_PER_BLOCK; j++) {
            // only print info is the inode is valid
            if(inode.inode[j].isvalid == 1) {
                int inumber = INODES_PER_BLOCK*(i-1) + j;
                printf("inode %d\n", inumber);
                printf("    size: %d bytes\n", inode.inode[j].size);
                printf("    direct blocks:");
                // loop through direct pointers and print block if valid
                int k;
                for(k = 0; k < POINTERS_PER_INODE; k++) {
                    if(inode.inode[j].direct[k] != 0) {
                        printf(" %d", inode.inode[j].direct[k]);
                    }
                }
                printf("\n");
                // get indirect block
                if(inode.inode[j].indirect != 0) {
                    printf("    indirect block: %d\n", inode.inode[j].indirect);
                    union fs_block indirect;
                    disk_read(inode.inode[j].indirect, indirect.data);
                    // loop through indirect data blocks
                    printf("    indirect data blocks:");
                    int x;
                    for(x = 0; x < POINTERS_PER_BLOCK; x++) {
                        if(indirect.pointers[x] != 0) {
                            printf(" %d", indirect.pointers[x]);
                        }
                    }
                    printf("\n");
                }
            }
        }
    }
}

int fs_mount()
{
    union fs_block sblock;
    disk_read(0,sblock.data);
    /* size_t m = sblock.super.magic; */
    if(sblock.super.magic != FS_MAGIC) {
        printf("disk not formatted\n");
        return 0;
    }
    if(FREE_BLOCK_MAP) free(FREE_BLOCK_MAP);
    FREE_BLOCK_MAP = calloc(sblock.super.nblocks, sizeof(int));
    // super block is not valid
    FREE_BLOCK_MAP[0] = 1;
    // loop through all inodes and build free block bitmap
    int i;
    for(i = 1; i <= sblock.super.ninodeblocks; i++) {
        union fs_block inode;
        disk_read(i,inode.data);
        // inode block is not valid
        FREE_BLOCK_MAP[i] = 1;
        // loop through each inode in this block
        int j;
        for(j = 0; j < INODES_PER_BLOCK; j++) {
            if(inode.inode[j].isvalid == 1) {
                // loop through direct pointers and update map if valid
                int k;
                for(k = 0; k < POINTERS_PER_INODE; k++) {
                    if(inode.inode[j].direct[k] != 0) {
                        FREE_BLOCK_MAP[inode.inode[j].direct[k]] = 1;
                    }
                }
                // get indirect block
                if(inode.inode[j].indirect != 0) {
                    union fs_block indirect;
                    FREE_BLOCK_MAP[inode.inode[j].indirect] = 1;
                    disk_read(inode.inode[j].indirect, indirect.data);
                    // loop through indirect data blocks and update map
                    int x;
                    for(x = 0; x < POINTERS_PER_BLOCK; x++) {
                        if(indirect.pointers[x] != 0) {
                            FREE_BLOCK_MAP[indirect.pointers[x]] = 1;
                        }
                    }
                }
            }
        }
    }
    MOUNTED = 1;
    return 1;
}

int fs_create()
{
    if(!MOUNTED) {
        printf("filesystem not mounted\n");
        return 0;
    }
    union fs_block super;
    disk_read(0,super.data);
    int i;
    // scan for available inode
    for(i = 1; i < super.super.ninodes; i++) {
        struct fs_inode x;
        inode_load(i, &x);
        if(x.isvalid == 0) {
            x.isvalid = 1;
            x.size = 0;
            int j;
            for(j = 0; j < POINTERS_PER_INODE; j++) {
                x.direct[j] = 0;
            }
            x.indirect = 0;
            inode_save(i, &x);
            return i;
        }
    }
    return -1;
}

int fs_delete( int inumber )
{
    if(!MOUNTED) {
        printf("filesystem not mounted\n");
        return 0;
    }
    struct fs_inode x;
    inode_load(inumber, &x);
    x.isvalid = 0;
    x.size = 0;
    int i;
    for(i = 0; i < POINTERS_PER_INODE; i++) {
        FREE_BLOCK_MAP[x.direct[i]] = 0;
    }
    union fs_block f;
    disk_read(x.indirect, f.data);
    for(i = 0; i < POINTERS_PER_BLOCK; i++) {
        if(f.pointers[i] > 0) {
            FREE_BLOCK_MAP[f.pointers[i]] = 0;
        }
    }
    disk_write(x.indirect, f.data);
    x.indirect = 0;
    inode_save(inumber, &x);
    return 1;
}

int fs_getsize( int inumber )
{
    if(!MOUNTED) {
        printf("filesystem not mounted\n");
        return -1;
    }
    struct fs_inode x;
    inode_load(inumber, &x);
    if(x.isvalid == 0) {
        printf("not a valid inode\n");
        return -1;
    }
    return x.size;
}

int fs_read(int inumber, char *data, int length, int offset ) {
    // reset string in order to avoid writing to end of a string already in memory
    data[0] = '\0';
    if (!MOUNTED) {
        printf("filesystem not mounted\n");
        return 0;
    }
    struct fs_inode x;
    inode_load(inumber, &x);
    if (offset >= x.size) {
        return 0;
    }
    int actual_length = offset + length > x.size ? x.size - offset : length;
    int length_to_read = actual_length;
    int block_pointer = offset / DISK_BLOCK_SIZE;
    int block_offset = offset % DISK_BLOCK_SIZE;
    int block_num;
    while (length_to_read > 0) {
        if (block_pointer > POINTERS_PER_BLOCK + POINTERS_PER_INODE) {
            break;
        }
        if (block_pointer >= POINTERS_PER_INODE) {
            // calculate block num for indirect pointer
            int indirect_block = x.indirect;
            union fs_block block;
            disk_read(indirect_block, block.data);
            block_num = block.pointers[block_pointer - POINTERS_PER_INODE];
        } else {
            block_num = x.direct[block_pointer];
        }
        length_to_read = read_inode_data(block_num, block_offset, length_to_read, data);
        block_offset = 0;
        block_pointer++;
    }
    return actual_length - length_to_read;
}

int fs_write( int inumber, const char *data, int length, int offset ) {
    if(!MOUNTED) {
        printf("filesystem not mounted\n");
        return -1;
    }

    struct fs_inode inode;
    inode_load(inumber, &inode);
    if(inode.isvalid != 1) {
        printf("invalid inode\n");
        return -1;
    }
    int block_pointer = offset / DISK_BLOCK_SIZE;
    int block_offset = offset % DISK_BLOCK_SIZE;
    int bytesLeft = length;
    int block_num;
    int indirect_block;
    union fs_block block;
    while(bytesLeft > 0) {
        if (block_pointer > POINTERS_PER_BLOCK + POINTERS_PER_INODE) {
            break;
        }
        if (block_pointer >= POINTERS_PER_INODE) {
            // calculate block num for indirect pointer
            indirect_block = inode.indirect == 0 ? find_free_indirect_block(&inode) : inode.indirect;
            if (indirect_block == -1) {
                printf("no free blocks left\n");
                return 0;
            }
            disk_read(indirect_block, block.data);
            block_num = block.pointers[block_pointer - POINTERS_PER_INODE];
            if (block_num == 0) {
                block_num = find_free_block();
                block.pointers[block_pointer- POINTERS_PER_INODE] = block_num == -1 ? 0 : block_num;
                disk_write(indirect_block, block.data);
            }
        } else {
            block_num = inode.direct[block_pointer];
            if (block_num == 0) {
                block_num = find_free_block();
                inode.direct[block_pointer] = block_num == -1 ? 0 : block_num;
            }
        }
        if (block_num == -1) {
            printf("no free blocks left\n");
            return 0;
        }

        bytesLeft = write_inode_data(block_num, block_offset, bytesLeft, &data);
        block_offset = 0;
        block_pointer++;
    }
    inode.size += length - bytesLeft;
    inode_save(inumber, &inode);
    return length - bytesLeft;
}

