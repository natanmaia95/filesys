#include <linux/types.h>

struct superblock {
    uint32_t magic;             // Identifier (e.g., 0xABCD1234)
    uint32_t total_blocks;      // Total blocks in the file system
    uint32_t block_size;        // Bytes per block (e.g., 4096)
    uint32_t inode_count;       // Total inodes
    uint32_t free_inodes;       // Free inodes remaining
    uint32_t free_blocks;       // Free data blocks remaining
    uint32_t inode_bitmap_block; // Starting block of inode bitmap
    uint32_t data_bitmap_block;  // Starting block of data bitmap
    uint32_t inode_table_block;  // Starting block of inode table
    uint32_t data_blocks_block;  // Starting block of data region
    uint32_t created_time;      // File system creation timestamp
    // ... (other fields like mount/write times)
};

struct inode {
    uint16_t type;              // File type (e.g., regular, directory)
    uint16_t permissions;       // Read/write/execute permissions
    uint32_t size;              // File size in bytes
    uint32_t created_time;
    uint32_t modified_time;
    uint32_t accessed_time;
    uint32_t direct_blocks[12]; // Direct block pointers (48 KB if block=4K)
    uint32_t single_indirect;   // Indirect block for larger files (4 MB if block=4K)
    // ... (optionally extend with double/triple indirect blocks)
};

#define DEVICE "/dev/loop1000"
#define IDENTIFIER 0xACBD0251;
#define BLOCK_SIZE = 4096;

struct superblock {
    __u32 identifier; //for the device to udnerstand what kind of file system this is
    __u32 total_blocks;
    __u32 block_size;
    __u32 inode_count;
    __u32 free_inodes;
    __u32 free_blocks;
    __u32 inode_bitmap_block; //starting block of inode bitmap
    __u32 data_bitmap_block;
    __u32 inode_table_block;
    __u32 data_blocks_block;
    // __u32 created_time;
};

//inode bitmap: list of bits indicating which inodes are in use and which arent. suggested 1024 inodes.
//data bitmap: list of bits indicating which data blocks are in use and which arent. suggested 4096 data blocks.
//inode table: contains the actual inode structs
//data blocks: the contents of files. dir structs go here too.

struct inode {
    __u16 type;
    __u16 permissions;
    __u32 size; //file size in bytes
    __u32 created_time;
    __u32 modified_time;
    __u32 accessed_time;
    __u32 direct_blocks[8];
    __u32 indirect_block;
};





struct superblock SUPERBLOCK;


