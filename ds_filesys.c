/* Requirimentos

OK formatar 
OK criar arquivo tamanho
OK apagar
OK listar
ler arquivo inicio fim
ordenar arquivo (usar hugepage)
concatenar arquivo1 arquivo2 (sem usar hugepage)

*/





#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>


#define BLOCK_SIZE 4096
#define TOTAL_BLOCKS (1024 * 1024 * 1024 / BLOCK_SIZE) // 262,144 blocks
#define INODES_COUNT 1024
#define INODE_DIRECT_BLOCKS 12
#define MAX_NAME_LEN 252
#define MAGIC 0xEF53


typedef struct {
    uint32_t magic;
    uint32_t total_blocks;
    uint32_t total_inodes;
    uint32_t free_blocks;
    uint32_t free_inodes;
    uint32_t block_size;
    uint32_t inode_table_start;
    uint32_t block_bitmap_start;
    uint32_t inode_bitmap_start;
    uint32_t data_start;
} superblock_t;

typedef struct {
    uint32_t size;
    uint32_t blocks;
    uint32_t direct[INODE_DIRECT_BLOCKS];
    uint32_t indirect;
    uint8_t is_dir;
} inode_t;


typedef struct {
    uint32_t inode_num;
    char name[MAX_NAME_LEN];
} dir_entry_t;


//inode bitmap: list of bits indicating which inodes are in use and which arent. suggested 1024 inodes.
//data bitmap: list of bits indicating which data blocks are in use and which arent. suggested 4096 data blocks.

//inode table: contains the actual inode structs
//data blocks: the contents of files. dir structs go here too.







void write_block(FILE *fs, uint32_t block_num, void *data) {
    fseek(fs, block_num * BLOCK_SIZE, SEEK_SET);
    fwrite(data, BLOCK_SIZE, 1, fs);
}

void read_block(FILE *fs, uint32_t block_num, void *data) {
    fseek(fs, block_num * BLOCK_SIZE, SEEK_SET);
    fread(data, BLOCK_SIZE, 1, fs);
}

void set_bit(uint8_t *bitmap, uint32_t bit) {
    bitmap[bit / 8] |= (1 << (bit % 8));
}

void unset_bit(uint8_t *bitmap, uint32_t bit) {
    bitmap[bit / 8] &= ~(1 << (bit % 8));
}

int is_bit_set(uint8_t *bitmap, uint32_t bit) {
    return (bitmap[bit / 8] >> (bit % 8)) & 1;
}

void format_vdisk(const char *filename) {
    FILE *fs = fopen(filename, "wb+");
    superblock_t sb = {
        .magic = MAGIC,
        .total_blocks = TOTAL_BLOCKS,
        .total_inodes = INODES_COUNT,
        .block_size = BLOCK_SIZE,
        .inode_table_start = 10,
        .block_bitmap_start = 1,
        .inode_bitmap_start = 9,
        .data_start = 42,
    };
    sb.free_blocks = TOTAL_BLOCKS - sb.data_start;
    sb.free_inodes = INODES_COUNT - 1; // Reserve inode 0

    // Write superblock
    write_block(fs, 0, &sb);

    // Initialize block bitmap (blocks 0-41 used)
    uint8_t block_bitmap[BLOCK_SIZE * 8] = {0};
    for (unsigned int i = 0; i < sb.data_start; i++) set_bit(block_bitmap, i);
    for (int i = 0; i < 8; i++)
        write_block(fs, sb.block_bitmap_start + i, block_bitmap + i * BLOCK_SIZE);

    // Initialize inode bitmap (inode 0 used for root directory)
    uint8_t inode_bitmap[BLOCK_SIZE] = {0};
    set_bit(inode_bitmap, 0);
    write_block(fs, sb.inode_bitmap_start, inode_bitmap);

    // Initialize root directory inode
    inode_t root_inode = { .is_dir = 1 };
    root_inode.direct[0] = sb.data_start; // First data block for root directory
    // Mark the block as used
    set_bit(block_bitmap, sb.data_start); 
    write_block(fs, sb.block_bitmap_start, block_bitmap);
    sb.free_blocks--;

    // Write root directory inode, which is inode 0.
    write_block(fs, sb.inode_table_start, &root_inode);

    // Initialize root directory block
    dir_entry_t root_dir[BLOCK_SIZE / sizeof(dir_entry_t)] = {0};
    //first element in root dir
    root_dir[0].inode_num = 0; // Root directory points to itself
    strncpy(root_dir[0].name, ".", MAX_NAME_LEN); // Current directory
    //second element in root dir, the parent dir is itself?
    root_dir[1].inode_num = 0; // Root directory points to itself
    strncpy(root_dir[1].name, "..", MAX_NAME_LEN); // Parent directory

    // Write root directory block
    write_block(fs, sb.data_start, root_dir);

    //update changes to superblock
    write_block(fs, 0, &sb);
    fclose(fs);
}



void read_inode(FILE *fs, uint32_t inode_num, inode_t *inode) {
    superblock_t sb;
    read_block(fs, 0, &sb); // Read the superblock
    // Calculate the block number and offset
    uint32_t block_num = sb.inode_table_start + (inode_num * sizeof(inode_t)) / BLOCK_SIZE;
    uint32_t offset = (inode_num * sizeof(inode_t)) % BLOCK_SIZE;
    // Read the block containing the inode
    uint8_t block[BLOCK_SIZE];
    read_block(fs, block_num, block);
    // Copy the inode from the correct position in the block
    memcpy(inode, block + offset, sizeof(inode_t));
}

void write_inode(FILE *fs, uint32_t inode_num, inode_t *inode) {
    superblock_t sb;
    read_block(fs, 0, &sb); // Read the superblock

    // Calculate the block number and offset
    uint32_t block_num = sb.inode_table_start + (inode_num * sizeof(inode_t)) / BLOCK_SIZE;
    uint32_t offset = (inode_num * sizeof(inode_t)) % BLOCK_SIZE;

    // Read the block containing the inode
    uint8_t block[BLOCK_SIZE];
    read_block(fs, block_num, block);

    // Copy the inode into the correct position in the block
    memcpy(block + offset, inode, sizeof(inode_t));

    // Write the updated block back to disk
    write_block(fs, block_num, block);
}



int add_dir_entry(FILE *fs, uint32_t dir_inode_num, const char *name, uint32_t file_inode_num) {
    superblock_t sb;
    read_block(fs, 0, &sb);

    inode_t dir_inode; //if dir_inode_num is 0, dir_inode is the root directory inode (not the dir struct!)
    read_block(fs, sb.inode_table_start + dir_inode_num, &dir_inode); 

    // Read the directory block
    dir_entry_t dir_entries[BLOCK_SIZE / sizeof(dir_entry_t)]; //fits as many dir entries as can fit inside a block (who knows)
    read_block(fs, dir_inode.direct[0], dir_entries);

    // Find an empty slot
    int start_index = 0;
    if (dir_inode_num == 0) { // Skip the first two entries (".", "..") in the root directory
        start_index = 2;
    }
    for (unsigned int i = start_index; i < BLOCK_SIZE / sizeof(dir_entry_t); i++) {
        if (dir_entries[i].inode_num == 0) {                    //empty DIRECT
            dir_entries[i].inode_num = file_inode_num;          //map inode to that directory
            strncpy(dir_entries[i].name, name, MAX_NAME_LEN);   //map filename to that inode
            write_block(fs, dir_inode.direct[0], dir_entries);  //write updated dir entries to disk
            return 0;
        }
    }

    return -1; // No space in directory
}

int remove_dir_entry(FILE *fs, uint32_t dir_inode_num, const char *name) {
    superblock_t sb;
    read_block(fs, 0, &sb);
    inode_t dir_inode;
    read_inode(fs, dir_inode_num, &dir_inode);

    // Read the directory block
    dir_entry_t dir_entries[BLOCK_SIZE / sizeof(dir_entry_t)];
    read_block(fs, dir_inode.direct[0], dir_entries);

    // Find the directory entry
    for (int i = 0; i < BLOCK_SIZE / sizeof(dir_entry_t); i++) {
        if (dir_entries[i].inode_num != 0 && strcmp(dir_entries[i].name, name) == 0) {
            uint32_t found_inode_num = dir_entries[i].inode_num;
            dir_entries[i].inode_num = 0;  // Mark the entry as free
            memset(dir_entries[i].name, 0, MAX_NAME_LEN);

            // Write the updated directory block back to disk
            write_block(fs, dir_inode.direct[0], dir_entries);
            return found_inode_num; // Return the inode number of the deleted file
        }
    }

    return -1; // File not found
}



void free_inode_in_bitmap(FILE *fs, uint32_t inode_num) {
    superblock_t sb;
    read_block(fs, 0, &sb);

    // Read the inode bitmap
    uint8_t inode_bitmap[BLOCK_SIZE];
    read_block(fs, sb.inode_bitmap_start, inode_bitmap);

    // Mark the inode as free
    inode_bitmap[inode_num / 8] &= ~(1 << (inode_num % 8));
    // Write the updated inode bitmap back to disk
    write_block(fs, sb.inode_bitmap_start, inode_bitmap);

    // Update the superblock
    sb.free_inodes++;
    write_block(fs, 0, &sb);
}

void free_inode_blocks(FILE* fs, inode_t* inode) {
    superblock_t sb;
    read_block(fs, 0, &sb);

    // Read the block bitmap
    uint8_t block_bitmap[BLOCK_SIZE * 8];
    for (int i = 0; i < 8; i++) {
        read_block(fs, sb.block_bitmap_start + i, block_bitmap + i * BLOCK_SIZE);
    }

    // Free direct blocks
    for (int i = 0; i < INODE_DIRECT_BLOCKS; i++) {
        if (inode->direct[i] != 0) {
            block_bitmap[inode->direct[i] / 8] &= ~(1 << (inode->direct[i] % 8));
            sb.free_blocks++;
        }
    }

    //we don't remove data from the data field, it can stay as is.

    // Write the updated block bitmap back to disk
    for (int i = 0; i < 8; i++) {
        write_block(fs, sb.block_bitmap_start + i, block_bitmap + i * BLOCK_SIZE);
    }

    // Write the updated superblock back to disk
    write_block(fs, 0, &sb);
}


//returns inode number if found, -1 if not found
int find_file(FILE *fs, superblock_t* sb, int dir_inode_num, const char* filename) {
    inode_t dir_inode;
    read_block(fs, sb->inode_table_start + dir_inode_num, &dir_inode);
    dir_entry_t dir_entries[BLOCK_SIZE / sizeof(dir_entry_t)];
    read_block(fs, dir_inode.direct[0], dir_entries);

    //find the inode num attached to the queried name
    int inode_num = -1;
    for (unsigned int i = 0; i < BLOCK_SIZE / sizeof(dir_entry_t); i++) {
        if (strcmp(filename, dir_entries[i].name) == 0) {
            inode_num = dir_entries[i].inode_num;
            break;
        }
    }
    if (inode_num == -1) printf("Couldn't find file '%s' \n", filename);
    return inode_num;
}


int create_file(FILE *fs, const char *name, size_t size) {
    superblock_t sb;
    read_block(fs, 0, &sb);

    // TODO: Check if filedirectory name to there? exists
    // [Code to check directory entries...]

    // Allocate inode
    uint8_t inode_bitmap[BLOCK_SIZE];
    read_block(fs, sb.inode_bitmap_start, inode_bitmap);
    int inode_num = -1;
    for (int i = 0; i < INODES_COUNT; i++) {  //find empty space to allocate inode
        if (!is_bit_set(inode_bitmap, i)) {   //if this inode index is not used,
            inode_num = i;                                              //remember index
            set_bit(inode_bitmap, i);                                   //set as used
            write_block(fs, sb.inode_bitmap_start, inode_bitmap);       //update the inode bitmap bits to disk
            sb.free_inodes--;                                           //update superblock to reflect one more used inode
            break;
        }
    }
    if (inode_num == -1) return -1; //couldn't find inode

    // Allocate data blocks
    inode_t inode = { .size = size };
    uint8_t block_bitmap[BLOCK_SIZE * 8];
    for (int i = 0; i < 8; i++)
        read_block(fs, sb.block_bitmap_start + i, block_bitmap + i * BLOCK_SIZE);

    int blocks_needed = (size + BLOCK_SIZE - 1) / BLOCK_SIZE; //"size + BLOCKSIZE - 1" basically does ceil(size_bytes / BLOCKSIZE)
    for (int i = 0; i < blocks_needed; i++) {
        int block_found = 0;
        for (int j = sb.data_start; j < TOTAL_BLOCKS; j++) {
            if (!is_bit_set(block_bitmap, j)) {
                inode.direct[i] = j;
                set_bit(block_bitmap, j);
                sb.free_blocks--;
                block_found = 1;
                break;
            }
        }
        if (!block_found) return -1;
    }
    //write changes to block bitmap and superblock to disk
    write_block(fs, sb.block_bitmap_start, block_bitmap);
    write_block(fs, 0, &sb);

    inode.blocks = blocks_needed;

    write_inode(fs, inode_num, &inode);

    // Write file data
    uint32_t ints_written = 0;
    for (int b = 0; b < blocks_needed; b++) {
        uint8_t data[BLOCK_SIZE];

        int* int_data = (int*) data; // Treat the block as an array of 32-bit integers
        int ints_per_block = BLOCK_SIZE / sizeof(int);
        
        // Fill the block with random 32-bit integers
        for (int j = 0; j < ints_per_block; j++) {
            if (ints_written >= size) break;
            int_data[j] = rand(); // Generate a random 32-bit integer
            ints_written += 1;
        }

        //writes that block to disk
        write_block(fs, inode.direct[b], data);
    }

    //add a directory entry    
    if (add_dir_entry(fs, 0, name, inode_num) == -1) {
        printf("Couldn't add file to directory\n");
        return -1;
    }

    fclose(fs);
    return 0;
}



int print_file(const char* disk_image, const char* name, int dir_inode_num, size_t start, size_t end) {
    FILE *fs = fopen(disk_image, "rb+");

    superblock_t sb;
    read_block(fs, 0, &sb);

    inode_t dir_inode;
    read_block(fs, sb.inode_table_start + dir_inode_num, &dir_inode);
    dir_entry_t dir_entries[BLOCK_SIZE / sizeof(dir_entry_t)];
    read_block(fs, dir_inode.direct[0], dir_entries);
  
    //find the inode num attached to the queried name
    int inode_num = find_file(fs, &sb, 0, name);
    if (inode_num == -1) {
        fclose(fs);
        return -1;   
    }

    //find the actual inode from that num
    inode_t inode;
    read_inode(fs, inode_num, &inode);

    // Read the file's data blocks
    if (end == -1 || end > inode.size) end = inode.size;
    if (start < 0 || start > inode.size) start = inode.size;
    uint32_t ints_read = 0;
    uint32_t ints_read_this_block = 0;
    for (unsigned int i = 0; i < inode.blocks; i++) {
        if (ints_read >= end) break; //custom end from command
        
        uint8_t data[BLOCK_SIZE];
        read_block(fs, inode.direct[i], data);

        int *int_data = (int *)data;
        int ints_per_block = BLOCK_SIZE / sizeof(int);

        for (int j = 0; j < ints_per_block; j++) {
            if (ints_read >= end) break; //custom end from command
            if (ints_read >= inode.size) break; //end on reading inode size

            if (ints_read >= start) {
                printf("%d ", int_data[j]);
            }
            ints_read += 1;
        }
        printf("\n");
    }

    fclose(fs);
    return 0;
}


int command_formatDrive(const char* disk_image) {
    format_vdisk(disk_image);
    printf("Disk image '%s' created and formatted.\n", disk_image);
    return 0;
}

int command_status(const char* disk_image) {
    FILE *fs = fopen(disk_image, "rb+");
    
    superblock_t sb;
    read_block(fs, 0, &sb);
    printf("Superblock:\n");
    printf("   Magic number: %d\n", sb.magic);
    printf("   Total Blocks: %d\n", sb.total_blocks);
    printf("   Free Blocks: %d\n", sb.free_blocks);
    printf("   Used Blocks: %d\n", sb.total_blocks - sb.free_blocks);
    printf("   Total Inodes: %d\n", sb.total_inodes);
    printf("   Free Inodes: %d\n", sb.free_inodes);
    printf("   Used Inodes: %d\n", sb.total_inodes - sb.free_inodes);
    printf("   Block Size: %d\n", sb.block_size);
    printf("   Inode Table Start: block %d\n", sb.inode_table_start);
    printf("   Block Bitmap Start: block %d\n", sb.block_bitmap_start);
    printf("   Inode Bitmap Start: block %d\n", sb.inode_bitmap_start);
    printf("   Data Start: block %d\n", sb.data_start);

    fclose(fs);
    return 0;
}

int command_createFile(const char* disk_image, const char* filename, const size_t size) {
    FILE *fs = fopen(disk_image, "rb+");

    if (create_file(fs, filename, size) == 0) {
        printf("File '%s' created successfully.\n", filename);
    } else {
        printf("Failed to create file '%s'.\n", filename);
        fclose(fs);
        return 1;
    }

    fclose(fs);
    return 0;
}

int command_listFiles(const char* disk_image, int dir_inode_num) {
    FILE *fs = fopen(disk_image, "rb+");
    superblock_t sb;
    read_block(fs, 0, &sb);

    inode_t dir_inode;
    read_block(fs, sb.inode_table_start + dir_inode_num, &dir_inode);

    dir_entry_t dir_entries[BLOCK_SIZE / sizeof(dir_entry_t)];
    read_block(fs, dir_inode.direct[0], dir_entries);

    printf("Files in directory:\n");
    for (unsigned int i = 0; i < BLOCK_SIZE / sizeof(dir_entry_t); i++) {
        //special roots
        if (dir_inode_num == 0 && i <= 1) {
            printf("  %s (inode: %u) [current directory is root]\n", dir_entries[i].name, dir_entries[i].inode_num);
        }

        if (dir_entries[i].inode_num != 0) { // Valid entry
            printf("  %s (inode: %u)\n", dir_entries[i].name, dir_entries[i].inode_num);
            inode_t inode;
            read_inode(fs, dir_entries[i].inode_num, &inode);
            printf("    size: %d, blocks: %d, first direct: %d\n", inode.size, inode.blocks, inode.direct[0]);
        }
    }

    fclose(fs);
    return 0;
}

int command_deleteFile(const char* disk_image, const char* name) {
    FILE *fs = fopen(disk_image, "rb+");

    //remove from root and get inode num
    int inode_num = remove_dir_entry(fs, 0, name);
    if (inode_num == -1) {
        printf("File '%s' not found.\n", name);
        fclose(fs);
        return -1;
    }

    inode_t inode;
    read_inode(fs, inode_num, &inode);

    free_inode_in_bitmap(fs, inode_num); //free from inode bitmap
    free_inode_blocks(fs, &inode);       //free blocks from block bitmap

    //write empty data in its place so no incorrect read can be made.
    // inode_t empty_inode = {0};
    // write_inode(fs, inode_num, &empty_inode);

    printf("File '%s' deleted successfully.\n", name);

    fclose(fs);
    return 0;
}


int command_joinFiles(const char* disk_image, const char* name1, const char* name2, const char* final_name) {
    int ints_per_block = BLOCK_SIZE / sizeof(int);
    FILE *fs = fopen(disk_image, "rb+");
    superblock_t sb;
    read_block(fs, 0, &sb);

    int file1_inode_num = find_file(fs, &sb, 0, name1);
    int file2_inode_num = find_file(fs, &sb, 0, name2);
    if (file1_inode_num == -1 || file2_inode_num == -1) {
        fclose(fs);
        return -1;
    }

    inode_t file1_inode;
    read_inode(fs, file1_inode_num, &file1_inode);
    inode_t file2_inode;
    read_inode(fs, file2_inode_num, &file2_inode);

    uint32_t final_size = file1_inode.size + file2_inode.size;
    int final_inode_num = create_file(fs, final_name, final_size); //create new file with random numbers
    if (final_inode_num == -1) { //if file name exists
        fclose(fs);
        return -1;
    }

    inode_t final_inode;
    read_inode(fs, final_inode_num, &final_inode);
    //fill new file with the contents of the first ones
    
    // Write file1 data into final
    uint32_t ints_written = 0;
    for (int b = 0; b < file1_inode.blocks; b++) {
        uint8_t final_data[BLOCK_SIZE];
        uint8_t file1_data[BLOCK_SIZE];
        read_block(fs, file1_inode.direct[b], &file1_data);

        int* final_data_ints = (int*) final_data; // Treat the block as an array of 32-bit integers
        int* file1_data_ints = (int*) file1_data;

        //TODO: fill the block with integers from file1
        for (int j = 0; j < ints_per_block; j++) {
            if (ints_written >= file1_inode.size) break;
            final_data_ints[j] = file1_data_ints[j];
            ints_written += 1;
        }

        //writes that block to disk
        write_block(fs, final_inode.direct[b], final_data);
    }
    uint32_t offset = ints_written % BLOCK_SIZE; // while (ints_written > BLOCK_SIZE) ints_written -= BLOCK_SIZE;
    // ints_written = 0;
    //write file2 data into final
    for (int b = file1_inode.blocks-1; b < final_inode.blocks; b++) {
        uint8_t final_data[BLOCK_SIZE];
        read_block(fs, final_inode.direct[b], &final_data);
        uint8_t file2_data[BLOCK_SIZE];
        read_block(fs, file2_inode.direct[b], &file2_data);

        int* final_data_ints = (int*) final_data;
        int* file2_data_ints = (int*) file2_data;

        //TODO: fill the block with integers from file1
        int wrapAround = 0;
        for (int j = 0; j < ints_per_block; j++) {
            if (ints_written >= final_inode.size) break;

            if (wrapAround == 0) {
                if (j + offset >= BLOCK_SIZE) {
                    wrapAround = 1;
                    //save current progress on current final block and move to the next
                    write_block(fs, final_inode.direct[b + 0], final_data);
                    //load new block from final to make progress on
                    read_block(fs, final_inode.direct[b + 1], final_data);
                }
            }

            final_data_ints[j + offset - wrapAround*BLOCK_SIZE] = file2_data_ints[j];
            ints_written += 1;
        }

        //writes that block to disk
        write_block(fs, final_inode.direct[b + wrapAround], final_data);
    }

    //TODO: delete old files
    printf("Finished joining files into '%s'.\n", final_name);
    return 0;
}



// Main function
int main(int argc, char *argv[]) {

    //hard override
    argc = 6;
    
    // argv[1] = "create";
    
    // argv[1] = "status";

    // argv[1] = "list";

    // argv[1] = "printfile"; argv[2] = "test5";

    // argv[1] = "deletefile"; argv[2] = "test12";
    
    // argv[1] = "createfile"; argv[2] = "test5"; argv[3] = "16";

    argv[1] = "createfile"; argv[2] = "test5"; argv[3] = "16";



    if (argc < 2) {
        printf("Usage: %s <command> [args...]\n", argv[0]);
        printf("Commands:\n");
        printf("\n  format\n    ↳ Create and format a new disk image named 'vdrive.img'\n");
        printf("\n  status\n    ↳ Prints superblock information\n");
        printf("\n  createfile <filename> <size>\n    ↳ Create a file with random data\n"); //TODO: set seed to system time
        printf("\n  printfile <filename> <opt:start_byte> <opt:end_byte>\n    ↳ Prints file contents as ints to console\n");
        printf("\n  deletefile <filename>\n    ↳ Deletes file from the directory structure\n");
        printf("\n  joinfiles <file1> <file2> <output_file>\n    ↳ Joins the contents of two files into a new third file\n");
        printf("\n  list\n    ↳ Lists all file names and inodes\n");
        return 1;
    }

    const char *command = argv[1];
    const char *disk_image = "vdrive.img";

    if (strcmp(command, "format") == 0) {
        return command_formatDrive(disk_image);
    } 
    
    else if (strcmp(command, "status") == 0) {
        command_status(disk_image);
        return -1;
    } 

    else if (strcmp(command, "createfile") == 0) {
        if (argc < 4) {
            printf("Usage: %s createfile <filename> <size_bytes>\n", argv[0]);
            return 1;
        }
        const char *filename = argv[2];
        size_t size = atoi(argv[3]);
        return command_createFile(disk_image, filename, size);
    } 

    else if (strcmp(command, "printfile") == 0) {
        if (argc < 3) {
            printf("Usage: %s printfile <filename>\n", argv[0]);
            return 1;
        }
        const char *filename = argv[2];

        size_t start = 0;
        if (argc >= 4) start = atoi(argv[3]); 
        size_t end = -1;
        if (argc >= 5) end = atoi(argv[4]); 
        
        // return print_file(disk_image, filename, 0);
        return print_file(disk_image, filename, 0, start, end);
    }

    else if (strcmp(command, "deletefile") == 0) {
        if (argc < 3) {
            printf("Usage: %s deletefile <filename>\n", argv[0]);
            return 1;
        }
        const char *filename = argv[2];
        return command_deleteFile(disk_image, filename);
    }

    else if (strcmp(command, "joinfiles") == 0) {
        if (argc < 5) {
            printf("Usage: %s joinfiles <file1> <file2> <output_file>\n", argv[0]);
            return 1;
        }
        const char *file1 = argv[2];
        const char *file2 = argv[3];
        const char *file3 = argv[4];
        return command_joinFiles(disk_image, file1, file2, file3);
    }

    else if (strcmp(command, "list") == 0) {
        return command_listFiles(disk_image, 0);
    }
    
    else {
        printf("Unknown command: %s\n", command);
        return 1;
    }

    return 0;
}
