/***************************************************************/
/*                                                             */
/*   MIPS-32 Instruction Level Simulator                       */
/*                                                             */
/*   Computer Architecture - Professor Onur Mutlu              */
/*                                                             */
/***************************************************************/

/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
/*          DO NOT MODIFY THIS FILE!                            */
/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "shell.h"
#include "pipe.h"

/***************************************************************/
/* Statistics.                                                 */
/***************************************************************/

uint32_t stat_cycles = 0, stat_inst_retire = 0, stat_inst_fetch = 0;
uint32_t stat_squash = 0;

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

#define ICACHE_SET_SIZE 64
#define ICACHE_N_WAYS 4

#define DIRTY_MISS 0b10
#define CLEAN_MISS 0b01
#define HIT 0b00

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

int RUN_BIT = TRUE;

/***************************************************************/
/*                                                             */
/* Procedure: mem_read_32                                      */
/*                                                             */
/* Purpose: Read a 32-bit word from memory                     */
/*                                                             */
/***************************************************************/
uint32_t mem_read_32(uint32_t address)
{
    int i;
    for (i = 0; i < MEM_NREGIONS; i++) {
        if (address >= MEM_REGIONS[i].start &&
                address < (MEM_REGIONS[i].start + MEM_REGIONS[i].size)) {
            uint32_t offset = address - MEM_REGIONS[i].start;

            return
                (MEM_REGIONS[i].mem[offset+3] << 24) |
                (MEM_REGIONS[i].mem[offset+2] << 16) |
                (MEM_REGIONS[i].mem[offset+1] <<  8) |
                (MEM_REGIONS[i].mem[offset+0] <<  0);
        }
    }

    return 0;
}

/***************************************************************/
/*                                                             */
/* Procedure: mem_write_32                                     */
/*                                                             */
/* Purpose: Write a 32-bit word to memory                      */
/*                                                             */
/***************************************************************/
void mem_write_32(uint32_t address, uint32_t value)
{
    int i;
    for (i = 0; i < MEM_NREGIONS; i++) {
        if (address >= MEM_REGIONS[i].start &&
                address < (MEM_REGIONS[i].start + MEM_REGIONS[i].size)) {
            uint32_t offset = address - MEM_REGIONS[i].start;

            MEM_REGIONS[i].mem[offset+3] = (value >> 24) & 0xFF;
            MEM_REGIONS[i].mem[offset+2] = (value >> 16) & 0xFF;
            MEM_REGIONS[i].mem[offset+1] = (value >>  8) & 0xFF;
            MEM_REGIONS[i].mem[offset+0] = (value >>  0) & 0xFF;
            return;
        }
    }
}

/***************************************************************/
/*                                                             */
/* Procedure : help                                            */
/*                                                             */
/* Purpose   : Print out a list of commands                    */
/*                                                             */
/***************************************************************/
void help() {                                                    
  printf("----------------MIPS ISIM Help-----------------------\n");
  printf("go                     -  run program to completion         \n");
  printf("run n                  -  execute program for n instructions\n");
  printf("rdump                  -  dump architectural registers      \n");
  printf("mdump low high         -  dump memory from low to high      \n");
  printf("input reg_no reg_value - set GPR reg_no to reg_value  \n");
  printf("?                      -  display this help menu            \n");
  printf("quit                   -  exit the program                  \n\n");
}

/***************************************************************/
/*                                                             */
/* Procedure : cycle                                           */
/*                                                             */
/* Purpose   : Execute a cycle                                 */
/*                                                             */
/***************************************************************/
void cycle() {                                                
  pipe_cycle();

  stat_cycles++;
}

/***************************************************************/
/*                                                             */
/* Procedure : run n                                           */
/*                                                             */
/* Purpose   : Simulate the MIPS for n cycles                 */
/*                                                             */
/***************************************************************/
void run(int num_cycles) {                                      
  int i;

  if (RUN_BIT == FALSE) {
    printf("Can't simulate, Simulator is halted\n\n");
    return;
  }

  printf("Simulating for %d cycles...\n\n", num_cycles);
  for (i = 0; i < num_cycles; i++) {
    if (RUN_BIT == FALSE) {
	    printf("Simulator halted\n\n");
	    break;
    }
    cycle();
  }
}

/***************************************************************/
/*                                                             */
/* Procedure : go                                              */
/*                                                             */
/* Purpose   : Simulate the MIPS until HALTed                 */
/*                                                             */
/***************************************************************/
void go() {                                                     
  if (RUN_BIT == FALSE) {
    printf("Can't simulate, Simulator is halted\n\n");
    return;
  }

  printf("Simulating...\n\n");
  while (RUN_BIT)
    cycle();
  printf("Simulator halted\n\n");
}

/***************************************************************/ 
/*                                                             */
/* Procedure : rdump                                           */
/*                                                             */
/* Purpose   : Dump architectural registers and other stats    */
/*                                                             */
/***************************************************************/
void rdump() {
    int i;

    printf("PC: 0x%08x\n", pipe.PC);

    for (i = 0; i < 32; i++) {
        printf("R%d: 0x%08x\n", i, pipe.REGS[i]);
    }

    printf("HI: 0x%08x\n", pipe.HI);
    printf("LO: 0x%08x\n", pipe.LO);
    printf("Cycles: %u\n", stat_cycles);
    printf("FetchedInstr: %u\n", stat_inst_fetch);
    printf("RetiredInstr: %u\n", stat_inst_retire);
    printf("IPC: %0.3f\n", ((float) stat_inst_retire) / stat_cycles);
    printf("Flushes: %u\n", stat_squash);
}

/***************************************************************/ 
/*                                                             */
/* Procedure : mdump                                           */
/*                                                             */
/* Purpose   : Dump a word-aligned region of memory to the     */
/*             output file.                                    */
/*                                                             */
/***************************************************************/
void mdump(int start, int stop) {          
  int address;

  printf("\nMemory content [0x%08x..0x%08x] :\n", start, stop);
  printf("-------------------------------------\n");
  for (address = start; address <= stop; address += 4)
    printf("  0x%08x (%d) : 0x%08x\n", address, address, mem_read_32(address));
  printf("\n");
}

/***************************************************************/
/*                                                             */
/* Procedure : get_command                                     */
/*                                                             */
/* Purpose   : Read a command from standard input.             */  
/*                                                             */
/***************************************************************/
void get_command() {
  char buffer[20];
  int start, stop, cycles;
  int register_no, register_value;

  printf("MIPS-SIM> ");

  if (scanf("%s", buffer) == EOF)
      exit(0);

  printf("\n");

  switch(buffer[0]) {
  case 'G':
  case 'g':
    go();
    break;

  case 'M':
  case 'm':
    if (scanf("%i %i", &start, &stop) != 2)
        break;

    mdump(start, stop);
    break;

  case '?':
    help();
    break;
  case 'Q':
  case 'q':
    printf("Bye.\n");
    exit(0);

  case 'R':
  case 'r':
    if (buffer[1] == 'd' || buffer[1] == 'D')
        rdump();
    else {
	    if (scanf("%d", &cycles) != 1) break;
	    run(cycles);
    }
    break;

  case 'I':
  case 'i':
   if (scanf("%i %i", &register_no, &register_value) != 2)
      break;
   
   printf("%i %i\n", register_no, register_value);
   pipe.REGS[register_no] = register_value;
   break;
   
  case 'H':
  case 'h':
   if (scanf("%i", &register_value) != 1)
      break;

   pipe.HI = register_value; 
   break;
  
  case 'L':
  case 'l':
   if (scanf("%i", &register_value) != 1)
      break;

   pipe.LO = register_value; 
   break;

  default:
    printf("Invalid Command\n");
    break;
  }
}

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

/**************************************************************/
/*                                                            */
/* Procedure : load_program                                   */
/*                                                            */
/* Purpose   : Load program and service routines into mem.    */
/*                                                            */
/**************************************************************/
void load_program(char *program_filename) {                   
  FILE * prog;
  int ii, word;

  /* Open program file. */
  prog = fopen(program_filename, "r");
  if (prog == NULL) {
    printf("Error: Can't open program file %s\n", program_filename);
    exit(-1);
  }

  /* Read in the program. */

  ii = 0;
  while (fscanf(prog, "%x\n", &word) != EOF) {
    mem_write_32(MEM_TEXT_START + ii, word);
    ii += 4;
  }

  printf("Read %d words from program into memory.\n\n", ii/4);
}

/************************************************************/
/*                                                          */
/* Procedure : initialize                                   */
/*                                                          */
/* Purpose   : Load machine language program                */ 
/*             and set up initial state of the machine.     */
/*                                                          */
/************************************************************/
void initialize(char *program_filename, int num_prog_files) { 
  int i;

  init_memory();
  pipe_init();
  for ( i = 0; i < num_prog_files; i++ ) {
    load_program(program_filename);
    while(*program_filename++ != '\0');
  }
    
  RUN_BIT = TRUE;
}

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

/***************************************************************/
/*                                                             */
/* Procedure : main                                            */
/*                                                             */
/***************************************************************/
int main(int argc, char *argv[]) {                              

  /* Error Checking */
  if (argc < 2) {
    printf("Error: usage: %s <program_file_1> <program_file_2> ...\n",
           argv[0]);
    exit(1);
  }

  printf("MIPS Simulator\n\n");

  initialize(argv[1], argc - 1);

  while (1)
    get_command();
    
}
