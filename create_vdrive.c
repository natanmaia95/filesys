








#define DEVICE "/dev/loop1000"
#define BLOCK_SIZE 4096

struct ext2_super_block { //tirado do kernel do linux, online
    __u32 s_inodes_count;
    __u32 s_blocks_count;
    //etc
}


struct ext2_super_block super_block;


void create_file() {
    int fd = open(DEVICE, O_RDWR);
    
    read_superblock(fd);
    close(fd);
}


void read_superblock(int fd) {
    //superblock é o bloco 1 nao 0, entao a gente move um bloco pra frente
    lseek(fd, BLOCK_SIZE, SEEK_SET);

    //read retorna numero de bytes. se nao conseguiu ler o bloco, o numero de bytes lidos vai ser diferente
    if (read(fd, &super_block, sizeof(struct ext2_super_block)) != sizeof(struct ext2_super_block)) {
        printf("erro lendo superbloco");
        exit(EXIT_FAILURE);
    }
    printf("lido superbloco: blocks count = %d\n", super_block.s_blocks_count);
}



void main() {
    create_file();
}