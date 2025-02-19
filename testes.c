#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <linux/types.h>
#include <sys/stat.h>

#define DEVICE "/dev/loop19" // Substitua pelo seu dispositivo de teste
#define BLOCK_SIZE 1024

struct ext2_super_block {
    __u32 s_inodes_count;         /* Inodes count */
    __u32 s_blocks_count;         /* Blocks count */
    __u32 s_r_blocks_count;       /* Reserved blocks count */
    __u32 s_free_blocks_count;    /* Free blocks count */
    __u32 s_free_inodes_count;    /* Free inodes count */
    __u32 s_first_data_block;     /* First Data Block */
    __u32 s_log_block_size;       /* Block size */
    __s32 s_log_frag_size;        /* Fragment size */
    __u32 s_blocks_per_group;     /* # Blocks per group */
    __u32 s_frags_per_group;      /* # Fragments per group */
    __u32 s_inodes_per_group;     /* # Inodes per group */
    __u32 s_mtime;                /* Mount time */
    __u32 s_wtime;                /* Write time */
    __u16 s_mnt_count;            /* Mount count */
    __s16 s_max_mnt_count;        /* Maximal mount count */
    __u16 s_magic;                /* Magic signature */
    __u16 s_state;                /* File system state */
    __u16 s_errors;               /* Behaviour when detecting errors */
    __u16 s_minor_rev_level;      /* minor revision level */
    __u32 s_lastcheck;            /* time of last check */
    __u32 s_checkinterval;        /* max. time between checks */
    __u32 s_creator_os;           /* OS */
    __u32 s_rev_level;            /* Revision level */
    __u16 s_def_resuid;           /* Default uid for reserved blocks */
    __u16 s_def_resgid;           /* Default gid for reserved blocks */
    __u32 s_first_ino;            /* First non-reserved inode */
    __u16 s_inode_size;           /* size of inode structure */
    __u16 s_block_group_nr;       /* block group # of this superblock */
    __u32 s_feature_compat;       /* compatible feature set */
    __u32 s_feature_incompat;     /* incompatible feature set */
    __u32 s_feature_ro_compat;    /* readonly-compatible feature set */
    __u8 s_uuid[16];              /* 128-bit uuid for volume */
    char s_volume_name[16];       /* volume name */
    char s_last_mounted[64];      /* directory where last mounted */
    __u32 s_algorithm_usage_bitmap; /* For compression */
    __u8 s_prealloc_blocks;       /* Nr of blocks to try to preallocate*/
    __u8 s_prealloc_dir_blocks;   /* Nr to preallocate for dirs */
    __u16 s_reserved_gdt_blocks;  /* Per group table for online growth */
    __u8 s_journal_uuid[16];      /* uuid of journal superblock */
    __u32 s_journal_inum;         /* inode number of journal file */
    __u32 s_journal_dev;          /* device number of journal file */
    __u32 s_last_orphan;          /* start of list of inodes to delete */
    __u32 s_hash_seed[4];         /* HTREE hash seed */
    __u8 s_def_hash_version;      /* Default hash version to use */
    __u8 s_jnl_backup_type;       /* Default type of journal backup */
    __u16 s_desc_size;            /* Group desc. size: INCOMPAT_64BIT */
    __u32 s_default_mount_opts;
    __u32 s_first_meta_bg;        /* First metablock group */
    __u32 s_mkfs_time;            /* When the filesystem was created */
    __u32 s_jnl_blocks[17];       /* Backup of the journal inode */
    __u32 s_blocks_count_hi;      /* Blocks count high 32bits */
    __u32 s_r_blocks_count_hi;    /* Reserved blocks count high 32 bits*/
    __u32 s_free_blocks_hi;       /* Free blocks count */
    __u16 s_min_extra_isize;      /* All inodes have at least # bytes */
    __u16 s_want_extra_isize;     /* New inodes should reserve # bytes */
    __u32 s_flags;                /* Miscellaneous flags */
    __u16 s_raid_stride;          /* RAID stride */
    __u16 s_mmp_interval;         /* # seconds to wait in MMP checking */
    __u64 s_mmp_block;            /* Block for multi-mount protection */
    __u32 s_raid_stripe_width;    /* blocks on all data disks (N*stride)*/
    __u32 s_reserved[163];        /* Padding to the end of the block */
};

struct ext2_group_desc {
    __u32 bg_block_bitmap;        /* Blocks bitmap block */
    __u32 bg_inode_bitmap;        /* Inodes bitmap block */
    __u32 bg_inode_table;         /* Inodes table block */
    __u16 bg_free_blocks_count;   /* Free blocks count */
    __u16 bg_free_inodes_count;   /* Free inodes count */
    __u16 bg_used_dirs_count;     /* Directories count */
    __u16 bg_flags;
    __u32 bg_reserved[2];
    __u16 bg_itable_unused;       /* Unused inodes count */
    __u16 bg_checksum;            /* crc16(s_uuid+grouo_num+group_desc)*/
};

// Variáveis globais
struct ext2_super_block super;
struct ext2_group_desc group;

// Função para ler o superbloco
void read_superblock(int fd) {
    lseek(fd, BLOCK_SIZE, SEEK_SET);
    if (read(fd, &super, sizeof(struct ext2_super_block)) != sizeof(struct ext2_super_block)) {
        perror("Erro ao ler o superbloco");
        exit(EXIT_FAILURE);
    }
    printf("blocks_count = %d\n", super.s_blocks_count);
    printf("block size = %d\n", super.s_log_block_size);
    printf("Qtd inodes = %d\n", super.s_inodes_count);
    printf("grupo do super bloco = %d\n", super.s_block_group_nr);
    
    
    time_t hora = super.s_mtime;
    char strhora[100];
    strftime(strhora, 100, "%d/%m/%Y %H:%M:%S", localtime(&hora));
    printf("%s\n", strhora); 
}

void read_group_desc(int fd) {
    lseek(fd, 2*4096, SEEK_SET);
    if (read(fd, &group, sizeof(struct ext2_group_desc)) != sizeof(struct ext2_group_desc)) {
        perror("Erro ao ler o grupo de descritores");
        exit(EXIT_FAILURE);
    }
    printf("Qtd de inodes usados = %d\n", super.s_inodes_count - 
            group.bg_free_inodes_count);
    printf("Qtd de inodes livres = %d\n", group.bg_itable_unused);
    printf("Qtd de blocos livres = %d\n", group.bg_free_blocks_count);
    printf("block_bitmap = %d\n", group.bg_block_bitmap);
    printf("inode_bitmap = %d\n", group.bg_inode_bitmap);
    printf("inode_table = %d\n", group.bg_inode_table);
    printf("used dirs = %d\n", group.bg_used_dirs_count);
}

// Função principal para criar um arquivo
void create_file() {
    int fd = open(DEVICE, O_RDWR);
    if (fd < 0) {
        perror("Erro ao abrir o dispositivo");
        exit(EXIT_FAILURE);
    }

    read_superblock(fd);
    read_group_desc(fd);
 
    close(fd);
}

int main() {
    create_file();
    return 0;
}