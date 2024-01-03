//#include <stdatomic.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>

#define ICACHE_SET_SIZE 64
#define ICACHE_N_WAYS 4

#define DIRTY_MISS 0b10
#define CLEAN_MISS 0b01
#define HIT 0b00

/***************************************************************/
/* Main memory.                                                */
/***************************************************************/

#define MEM_DATA_START  0x10000000
#define MEM_DATA_SIZE   0x00100000
#define MEM_TEXT_START  0x00400000
#define MEM_TEXT_SIZE   0x00100000
#define MEM_STACK_START 0x7ff00000
#define MEM_STACK_SIZE  0x00100000
#define MEM_KDATA_START 0x90000000
#define MEM_KDATA_SIZE  0x00100000
#define MEM_KTEXT_START 0x80000000
#define MEM_KTEXT_SIZE  0x00100000

typedef struct {
    uint32_t start, size;
    uint8_t *mem;
} mem_region_t;

/* memory will be dynamically allocated at initialization */
mem_region_t MEM_REGIONS[] = {
    { MEM_TEXT_START, MEM_TEXT_SIZE, NULL },
    { MEM_DATA_START, MEM_DATA_SIZE, NULL },
    { MEM_STACK_START, MEM_STACK_SIZE, NULL },
    { MEM_KDATA_START, MEM_KDATA_SIZE, NULL },
    { MEM_KTEXT_START, MEM_KTEXT_SIZE, NULL }
};

#define MEM_NREGIONS (sizeof(MEM_REGIONS)/sizeof(mem_region_t))

/***************************************************************/
/*                                                             */
/* Procedure : init_memory                                     */
/*                                                             */
/* Purpose   : Allocate and zero memoryy                       */
/*                                                             */
/***************************************************************/
void init_memory() {                                           
    int i;
    for (i = 0; i < MEM_NREGIONS; i++) {
        MEM_REGIONS[i].mem = malloc(MEM_REGIONS[i].size);
        memset(MEM_REGIONS[i].mem, 0, MEM_REGIONS[i].size);
    }
}

// /***************************************************************/
// /*                                                             */
// /* Procedure: mem_read_32                                      */
// /*                                                             */
// /* Purpose: Read a 32-bit word from memory                     */
// /*                                                             */
// /***************************************************************/
// uint32_t mem_read_32(uint32_t address)
// {
//     int i;
//     for (i = 0; i < MEM_NREGIONS; i++) {
//         if (address >= MEM_REGIONS[i].start &&
//                 address < (MEM_REGIONS[i].start + MEM_REGIONS[i].size)) {
//             uint32_t offset = address - MEM_REGIONS[i].start;

//             return
//                 (MEM_REGIONS[i].mem[offset+3] << 24) |
//                 (MEM_REGIONS[i].mem[offset+2] << 16) |
//                 (MEM_REGIONS[i].mem[offset+1] <<  8) |
//                 (MEM_REGIONS[i].mem[offset+0] <<  0);
//         }
//     }

//     return 0;
// }

// /***************************************************************/
// /*                                                             */
// /* Procedure: mem_write_32                                     */
// /*                                                             */
// /* Purpose: Write a 32-bit word to memory                      */
// /*                                                             */
// /***************************************************************/
// void mem_write_32(uint32_t address, uint32_t value)
// {
//     int i;
//     for (i = 0; i < MEM_NREGIONS; i++) {
//         if (address >= MEM_REGIONS[i].start &&
//                 address < (MEM_REGIONS[i].start + MEM_REGIONS[i].size)) {
//             uint32_t offset = address - MEM_REGIONS[i].start;

//             MEM_REGIONS[i].mem[offset+3] = (value >> 24) & 0xFF;
//             MEM_REGIONS[i].mem[offset+2] = (value >> 16) & 0xFF;
//             MEM_REGIONS[i].mem[offset+1] = (value >>  8) & 0xFF;
//             MEM_REGIONS[i].mem[offset+0] = (value >>  0) & 0xFF;
//             return;
//         }
//     }
// }


typedef struct {
    uint8_t valid;
    uint8_t dirty;
    uint32_t tag;
    uint8_t lru_sum;
    uint8_t mem[32];
} way_elem_t;

typedef struct {
    way_elem_t mem_elems[ICACHE_N_WAYS];
} set_elem_t;

typedef struct {
    set_elem_t sets[ICACHE_SET_SIZE];
    uint8_t out_data;
} cache_mem_t;


void init_cache(cache_mem_t* cache_inst){
    uint8_t i = 0;
    for(i = 0; i  < ICACHE_SET_SIZE; i++) {
        //cache_inst->sets[i].mem_elems = malloc(ICACHE_N_WAYS*sizeof(way_elem_t));
        memset(cache_inst->sets[i].mem_elems, 0, ICACHE_N_WAYS*sizeof(way_elem_t));
    }
}

void mem_read_block(uint8_t* block_mem, uint32_t addr) {
    uint32_t block_start = (0xFFFFFFFE0 & addr);
    printf("block_start: %x \n", block_start);

    int i;
    for (i = 0; i < MEM_NREGIONS; i++) {
        if (block_start >= MEM_REGIONS[i].start &&
                block_start < (MEM_REGIONS[i].start + MEM_REGIONS[i].size)) {
            uint32_t offset = block_start - MEM_REGIONS[i].start;

            for(uint8_t j = 0; j < 32; j++){
                printf("reading %x \n", (offset + j));
                block_mem[j] = MEM_REGIONS[i].mem[offset+j];
            }

            return;
        }
    }
    printf("address not found! \n");
}

void mem_write_block(uint8_t* block_mem, uint32_t addr) {
    uint32_t block_start = (0xFFFFFFFE0 & addr);
    printf("block_start: %x \n", block_start);

    int i;
    for (i = 0; i < MEM_NREGIONS; i++) {
        if (block_start >= MEM_REGIONS[i].start &&
                block_start < (MEM_REGIONS[i].start + MEM_REGIONS[i].size)) {
            uint32_t offset = block_start - MEM_REGIONS[i].start;

            for(uint8_t j = 0; j < 32; j++){
                printf("writing %x \n", (offset + j));
                MEM_REGIONS[i].mem[offset+j] = block_mem[j];
            }

            return;
        }
    }
    printf("address not found! \n");
}

uint8_t read_cache(cache_mem_t* cache_inst, uint32_t addr) {
    uint8_t status = 3;
    uint32_t set_no = (addr & 0x7E0) >> 5;
    printf("set_no: %d \n", set_no);
    uint8_t block_offset = (addr & 0x1f);
    uint32_t tag = (addr & 0xfffff800) >> 11;
    set_elem_t* set_i = &(cache_inst->sets[set_no]);
    
    uint8_t i = 0;
    // HIT 
    for(i = 0; i < 4; i++) {
        // if valid and tag match 
        if ( (set_i->mem_elems[i].valid == 1) && (set_i->mem_elems[i].tag == tag)) {
            cache_inst->out_data = set_i->mem_elems[i].mem[block_offset];
            status = HIT;
            return status;
        }
    }

    // MISS
    for(uint8_t i = 0; i < 4; i++) {
        if(set_i->mem_elems[i].lru_sum == 0){
            printf("replacing way %d \n", i);
            uint32_t block_addr = ((set_i->mem_elems[i].tag << 11) | (set_no << 5));
            // IF VALID and DIRTY, WRITEBACK TO MEMORY
            if((set_i->mem_elems[i].valid == 1) && (set_i->mem_elems[i].dirty == 1)) {
                printf("VALID and DIRTY block!, writing back to address %x \n", block_addr);
                mem_write_block(set_i->mem_elems[i].mem, block_addr);
                status = DIRTY_MISS;
            } else if((set_i->mem_elems[i].valid == 1) && (set_i->mem_elems[i].dirty == 0)) {
                printf("VALID and CLEAN block!, will not write back to address %x \n", block_addr);
                status = CLEAN_MISS;
            } else {
                printf("INVALID block!, will not write back to %x \n", block_addr);
                status = CLEAN_MISS;
            }
            
            mem_read_block(set_i->mem_elems[i].mem, addr);
            cache_inst->out_data = set_i->mem_elems[i].mem[block_offset];
            set_i->mem_elems[i].dirty = 0;
            set_i->mem_elems[i].valid = 1;
            set_i->mem_elems[i].tag = tag;
            for(uint8_t n = 0; n < 4; n++){
                if(set_i->mem_elems[n].lru_sum > 0){
                    set_i->mem_elems[n].lru_sum -= 1;
                } 
            } 
            set_i->mem_elems[i].lru_sum = 3;

            break;
        }
    }
    return status;

    return 0;
}

uint8_t write_cache(cache_mem_t* cache_inst, uint32_t addr, uint8_t val) {
    uint8_t status = 3;

    uint32_t set_no = (addr & 0x7E0) >> 5;
    printf("set_no: %d \n", set_no);
    uint8_t block_offset = (addr & 0x1f);
    uint32_t tag = (addr & 0xfffff800) >> 11;
    set_elem_t* set_i = &(cache_inst->sets[set_no]);
    // HIT
    for(uint8_t i = 0; i < 4; i++) {
        if ( (set_i->mem_elems[i].valid == 1) && (set_i->mem_elems[i].tag == tag)){
            uint8_t lru_n = set_i->mem_elems[i].lru_sum;
            set_i->mem_elems[i].mem[block_offset] = val;
            for(uint8_t n = 0; n < 4; n++){
                if(set_i->mem_elems[n].lru_sum > lru_n){
                    set_i->mem_elems[n].lru_sum -= 1;
                } 
            } 
            set_i->mem_elems[i].lru_sum = 3;
            set_i->mem_elems[i].dirty = 1;

            status = HIT;
            return status;
        }
    }
    
    // MISS
    for(uint8_t i = 0; i < 4; i++) {
        if(set_i->mem_elems[i].lru_sum == 0){
            printf("replacing way %d \n", i);
            uint32_t block_addr = ((set_i->mem_elems[i].tag << 11) | (set_no << 5));
            // IF VALID and DIRTY, WRITEBACK TO MEMORY
            if((set_i->mem_elems[i].valid == 1) && (set_i->mem_elems[i].dirty == 1)) {
                printf("VALID and DIRTY block!, writing back to address %x \n", block_addr);
                mem_write_block(set_i->mem_elems[i].mem, block_addr);
                status = DIRTY_MISS;
            } else if((set_i->mem_elems[i].valid == 1) && (set_i->mem_elems[i].dirty == 0)) {
                printf("VALID and CLEAN block!, will not write back to address %x \n", block_addr);
                status = CLEAN_MISS;
            } else {
                printf("INVALID block!, will not write back to %x \n", block_addr);
                status = CLEAN_MISS;
            }
            
            mem_read_block(set_i->mem_elems[i].mem, addr);
            set_i->mem_elems[i].mem[block_offset] = val;
            set_i->mem_elems[i].dirty = 1;
            set_i->mem_elems[i].valid = 1;
            set_i->mem_elems[i].tag = tag;
            for(uint8_t n = 0; n < 4; n++){
                if(set_i->mem_elems[n].lru_sum > 0){
                    set_i->mem_elems[n].lru_sum -= 1;
                } 
            } 
            set_i->mem_elems[i].lru_sum = 3;

            break;
        }
    }
    return status;
}

int main() {
    printf("%d, %d, %d \n", sizeof(way_elem_t), sizeof(set_elem_t), sizeof(cache_mem_t));

    init_memory();

    // printf("mem:%d", MEM_REGIONS[0].mem[0]);
    // uint32_t temp = mem_read_32(0x1234);

    cache_mem_t* cache = malloc(sizeof(cache_mem_t));
    init_cache(cache);

    // mem_read_block(cache->sets[0].mem_elems[0].mem, 0x00412345);

    uint32_t PC = 0x00412345;
    write_cache(cache, PC, 0xf0);
    PC = 0x00412346;
    write_cache(cache, PC, 0xab);
    PC = 0x00422345;
    write_cache(cache, PC, 0xf1);
    PC = 0x00422345;
    write_cache(cache, PC, 0xac);
    PC = 0x00432345;
    write_cache(cache, PC, 0xf2);
    PC = 0x00432345;
    write_cache(cache, PC, 0xad);
    PC = 0x00442345;
    write_cache(cache, PC, 0xf3);
    PC = 0x00442345;
    write_cache(cache, PC, 0xae);
    PC = 0x00452345;
    write_cache(cache, PC, 0xf4);
    PC = 0x00452345;
    write_cache(cache, PC, 0xaf);
    PC = 0x00462345;
    write_cache(cache, PC, 0xf5);
    PC = 0x00462345;
    write_cache(cache, PC, 0xab);

    uint8_t status = read_cache(cache, 0x412346);
    printf("data from cache: 0x%04x with status %d\n", cache->out_data, status);

    // uint32_t data = mem_read_32(0x412346);
    // printf("\n\n\ndata: 0x%04x \n", data);
    // PC = 0xdead;
    // write_cache(cache, PC, 0x45);



} 
