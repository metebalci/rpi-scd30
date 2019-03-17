#pragma once

#include <stdint.h> 
#include <stdbool.h>

bool bsc_init(off_t peri_addr, size_t peri_size);
void bsc_deinit(void);

void bsc_set_slave_addr(uint8_t addr);
uint8_t bsc_get_slave_addr(void);

void bsc_set_cdiv(uint16_t cdiv);
uint16_t bsc_get_cdiv(void);

bool bsc_set_scl_freq(uint32_t freq);
uint32_t bsc_get_scl_freq(void);
void bsc_set_scl_freq_to_minimum(void);
void bsc_set_scl_freq_to_maximum(void);

void bsc_set_clkt_tout(uint16_t tout);
uint16_t bsc_get_clkt_tout(void);

ssize_t bsc_write(void* input, size_t len);
ssize_t bsc_read(void* output, size_t len);
