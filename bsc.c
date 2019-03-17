#include <stdio.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <time.h>
#include <inttypes.h>
#include <unistd.h>
#include "bsc.h"

#define CORE_CLK_IN_HZ  250000000
// >> 2 because these are used with uint32_t*
#define GPIO_BASE       (0x200000 >> 2)
// BSC1
#define BSC_BASE        (0x804000 >> 2)

// uint32_t offsets to registers
#define GPIO_FSEL   0
#define GPIO_SET    (0x1C >> 2)
#define GPIO_CLR    (0x28 >> 2)
#define BSC_C       0
#define BSC_S       1
#define BSC_DLEN    2
#define BSC_A       3 
#define BSC_FIFO    4
#define BSC_DIV     5
#define BSC_DEL     6
#define BSC_CLKT    7

// BSC_C fields
#define BSC_C_I2CEN     (1 << 15)
#define BSC_C_INTR      (1 << 10)
#define BSC_C_INTT      (1 << 9)
#define BSC_C_INTD      (1 << 8)
#define BSC_C_ST        (1 << 7)
#define BSC_C_CLEAR     ((1 << 5) | (1 << 4))
#define BSC_C_READ      1
 
#define START_READ      (BSC_C_I2CEN|BSC_C_ST|BSC_C_CLEAR|BSC_C_READ)
#define START_WRITE     (BSC_C_I2CEN|BSC_C_ST)
 
// BSC_S fields
#define BSC_S_CLKT  	  (1 << 9)
#define BSC_S_ERR       (1 << 8)
#define BSC_S_RXF       (1 << 7)
#define BSC_S_TXE       (1 << 6)
#define BSC_S_RXD       (1 << 5)
#define BSC_S_TXD       (1 << 4)
#define BSC_S_RXR       (1 << 3)
#define BSC_S_TXW       (1 << 2)
#define BSC_S_DONE      (1 << 1)
#define BSC_S_TA        1
 
#define CLEAR_STATUS    (BSC_S_CLKT|BSC_S_ERR|BSC_S_DONE)
#define TRANSFER_DONE   (BSC_S_DONE|BSC_S_ERR|BSC_S_CLKT)

struct bcm2835_peripheral {
	unsigned long addr_p;
	int mem_fd;
	void *map;
  size_t len;
	volatile uint32_t *addr;
} peri = {0};

static inline void set_reg(uint32_t offset, uint32_t val) {
#ifdef BSC_DEBUG
  printf("0x%" PRIu32 " <<< 0x%" PRIu32 "\n", offset<<2, val);
#endif
  *(peri.addr + offset) = val;
}

static inline uint32_t get_reg(uint32_t offset) {
  uint32_t val = *(peri.addr + offset);
#ifdef BSC_DEBUG
  printf("0x%" PRIu32 " >>> 0x%" PRIu32 "\n", offset<<2, val);
#endif
  return val;
}

static void set_bits(
    uint32_t offset, 
    uint32_t val, 
    uint32_t mask) {

  uint32_t v = get_reg(offset);
  v = (v & ~mask) | (val & mask);
  set_reg(offset, v);

}

static void set_bsc_reg(uint32_t offset, uint32_t val) {
  set_reg(BSC_BASE + offset, val);
}

static uint32_t get_bsc_reg(uint32_t offset) {
  return get_reg(BSC_BASE + offset);
}

static int map_peripheral(
    struct bcm2835_peripheral *p, 
    off_t base_addr,
    size_t len) {

  if ((p->mem_fd = open("/dev/mem", O_RDWR|O_SYNC) ) < 0) {

    fprintf(stderr, "Failed to open /dev/mem, try checking permissions.\n");
    return -1;

  } else {

    p->map = mmap(
      NULL,
      len,
      PROT_READ|PROT_WRITE,
      MAP_SHARED,
      p->mem_fd,
      base_addr
    );

    if (p->map == MAP_FAILED) {

      perror("Failed mmap");
      return -1;

    } else {

      p->len = len;
      p->addr = (volatile uint32_t *)p->map;

      return 0;

    }

  }

}
 
static void unmap_peripheral(struct bcm2835_peripheral *p) {
  munmap(p->map, p->len);
  close(p->mem_fd);
}

static void set_pin_func(uint8_t pin, uint16_t func) {
  uint32_t fsel_reg_addr = pin / 10;
  uint32_t fsel_reg_offset = (pin % 10) * 3;
  uint32_t val = func << fsel_reg_offset;
  uint32_t mask = 7 << fsel_reg_offset;
  set_bits(GPIO_BASE + GPIO_FSEL + fsel_reg_addr, val, mask);
}

static void set_pin_func_to_alt0(uint8_t pin) {
  // 100
  set_pin_func(pin, 4);
}

static void init_bsc_pins(uint8_t sda, uint8_t scl) {
  set_pin_func_to_alt0(sda);
  set_pin_func_to_alt0(scl);
} 
 
#ifdef BSC_DEBUG
static void print_buffer(void* input, size_t len) {

  uint8_t* buffer = input;

  for (size_t i = 0; i < len; i++) {
    printf("0x%02x ", buffer[i]);
  }

  printf("\n");

}
#endif

ssize_t bsc_write(
    void* input,
    size_t len) {

  assert (input != NULL);
  assert (len > 0);

#ifdef BSC_DEBUG
  printf("bsc_write request: ");
  print_buffer(input, len);
#endif

  uint8_t* buffer = input;

  set_bsc_reg(BSC_C, BSC_C_CLEAR);

  for (size_t i = 0; i < len; i++) {
    set_bsc_reg(BSC_FIFO, buffer[i]);
  }

  set_bsc_reg(BSC_DLEN,   len);
  set_bsc_reg(BSC_S,      CLEAR_STATUS);
  set_bsc_reg(BSC_C,      START_WRITE);

  uint32_t s;

  while (true) {

    s = get_bsc_reg(BSC_S);
    if (s & TRANSFER_DONE) {
      break;
    } else {
      // continue
    }

  }

  if (s & BSC_S_ERR) {

    fprintf(stderr, "bsc_write ERROR: ACK\n");
    return -1;

  } else if (s & BSC_S_CLKT) {

    fprintf(stderr, "bsc_write ERROR: CLKT TIMEOUT\n");
    return -1;

  } else {

#ifdef BSC_DEBUG
  printf("bsc_write OK\n");
#endif

    return len;

  }

}

ssize_t bsc_read(
    void* input,
    size_t len) {

  assert (input != NULL);
  assert (len > 0);

#ifdef BSC_DEBUG
  printf("bsc_read request: %zu\n", len);
#endif

  uint8_t* buffer = input;

  set_bsc_reg(BSC_DLEN, len);
  set_bsc_reg(BSC_S,    CLEAR_STATUS);
  set_bsc_reg(BSC_C,    START_READ);

  uint32_t s;

  size_t read = 0;

  while (true) {

    s = get_bsc_reg(BSC_S);

    if (s & BSC_S_RXD) {
      if (read < len) { 
        buffer[read++] = get_bsc_reg(BSC_FIFO);
      } else {
        fprintf(stderr, "bsc_read: BUFFER_OVERFLOW ERROR\n");
        assert (false);
      }
    } else {
      // do nothing
    }
    
    if (s & TRANSFER_DONE) {
      break;
    } else {
      // do nothing
    }

  }

  if (s & BSC_S_ERR) {

    fprintf(stderr, "bsc_read ERROR: ACK\n");
    return -1;

  } else if (s & BSC_S_CLKT) {

    fprintf(stderr, "bsc_read ERROR: CLKT TIMEOUT\n");
    return -1;

  } else {

    // because while above is "breaked" immediately after done
    // there can still be data in fifo, so read that
    while (get_bsc_reg(BSC_S) & BSC_S_RXD) {

      if (read < len) { 

        buffer[read++] = get_bsc_reg(BSC_FIFO);

      } else {

        fprintf(stderr, "bsc_read: BUFFER_OVERFLOW ERROR\n");
        assert (false);

      }

    }

#ifdef BSC_DEBUG
    printf("bsc_read: ");
    print_buffer(buffer, read);

    if (read == len) {
      printf("bsc_read OK\n");
    } else {
      printf("bsc_read PARTIAL READ: %zu\n", read);
    }
#endif

    return read;

  }

}

void bsc_set_slave_addr(uint8_t slave_addr) {
  set_bsc_reg(BSC_A, slave_addr & 0x7F);
}  

uint8_t bsc_get_slave_addr(void) {
  return get_bsc_reg(BSC_A) & 0x7F;
}

bool bsc_init(off_t peri_base, size_t peri_size) {

  // 28 to cover all gpio function select registers
  if (map_peripheral(
        &peri,
        (off_t) peri_base,
        (size_t) peri_size) < 0) {
    return false;
  }

#ifdef BSC_DEBUG
  printf("mapped to %p\n", (void*) peri.addr);
#endif

  init_bsc_pins(2, 3);

#ifdef BSC_DEBUG
  printf("scl freq: %" PRIu32 "\n", bsc_get_scl_freq());
  printf("clkt.tout: %" PRIu16 "\n", bsc_get_clkt_tout());
#endif

  return true;

}

void bsc_deinit(void) {
  unmap_peripheral(&peri);
}

void bsc_set_cdiv(uint16_t cdiv) {
  // only down rounded even numbers are used
  // so better to round before setting
  set_bsc_reg(BSC_DIV, cdiv & 0xFFFE);

}

uint16_t bsc_get_cdiv(void) {
  return get_bsc_reg(BSC_DIV) & 0xFFFF;
}

bool bsc_set_scl_freq(uint32_t freq) {

  uint32_t cdiv = CORE_CLK_IN_HZ / freq;

  if (cdiv > 32768) {
    fprintf(stderr, "bsc_set_scl_freq ERROR: requested freq is too small\n");
    return false;
  }

  if (cdiv == 32768) cdiv = 0;

  bsc_set_cdiv(cdiv & 0xFFFF);

  return true;

}

void bsc_set_scl_freq_to_minimum(void) {
  bsc_set_cdiv(0);
}

void bsc_set_scl_freq_to_maximum(void) {
  bsc_set_cdiv(2);
}

uint32_t bsc_get_scl_freq(void) {

  uint32_t cdiv = bsc_get_cdiv();

  if (cdiv == 0) cdiv = 32768;

  if (CORE_CLK_IN_HZ % cdiv == 0) {
    return CORE_CLK_IN_HZ / cdiv;
  } else {
    return (CORE_CLK_IN_HZ / cdiv) + 1;
  }

}

void bsc_set_clkt_tout(uint16_t tout) {
  set_bsc_reg(BSC_CLKT, tout);
}

uint16_t bsc_get_clkt_tout(void) {
  return get_bsc_reg(BSC_CLKT) & 0xFFFF;
}
