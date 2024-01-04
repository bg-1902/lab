#include <stdint.h>

#define ICACHE_SET_SIZE 64
#define ICACHE_N_WAYS 4

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

extern cache_mem_t* icache;

uint8_t read_cache();