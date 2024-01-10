#include <stdint.h>

#define ICACHE_SET_SIZE 64
#define ICACHE_N_WAYS 4
#define DCACHE_SET_SIZE 256 
#define DCACHE_N_WAYS 8

#define DIRTY_MISS 0b10
#define CLEAN_MISS 0b01
#define HIT 0b00

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
    uint32_t out_data;
} cache_mem_t;


typedef struct {
    way_elem_t mem_elems[DCACHE_N_WAYS];
} d_set_elem_t;

typedef struct {
    d_set_elem_t sets[DCACHE_SET_SIZE];
    uint32_t out_data;
} d_cache_mem_t;


extern cache_mem_t* icache;
extern d_cache_mem_t* dcache;

uint8_t read_d_cache(d_cache_mem_t* cache_inst, uint32_t addr);
uint8_t read_cache(cache_mem_t* cache_inst, uint32_t addr);
