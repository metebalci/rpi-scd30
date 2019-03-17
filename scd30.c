#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdbool.h>
#include <unistd.h>
#include "bsc.h"
#include "crc8scd30.h"
#include "scd30.h"

#ifdef SCD30_DEBUG
static void print_buffer(void* buffer, size_t len) {
  uint8_t* p = buffer;
  for (size_t i = 0; i < len; i++) {
    printf("0x%02X ", p[i]);
  }
}
#endif

// write 2 bytes command
// read 3 bytes result
// this is a common operation
static bool do23(uint8_t out[2], uint8_t in[3]) {

  if (bsc_write(out, 2) == 2) {

    if (bsc_read(in, 3) == 3) {

      uint8_t crc = crc8scd30(in, 2);

      if (crc != in[2])  {

        fprintf(stderr, "CRC ERROR");
        return false;

      } else {

        return true;

      }

    } else {

      return false;

    }

  } else {

    return false;

  }

}

bool scd30_soft_reset(void) {

  uint8_t out[] = {0xD3, 0x04};
  return bsc_write(out, 2) == 2;

}

bool scd30_read_firmware_version(uint16_t fv[1]) {

  uint8_t out[] = {0xD1, 0x00};
  uint8_t in[3];

  if (do23(out, in)) {

    fv[0] =  (in[0] << 8) | in[1];
    return true;

  } else {

    return false;

  }

}

bool scd30_get_data_ready_status(void) {

  uint8_t out[] = {0x02, 0x02};
  uint8_t in[3];

  if (do23(out, in)) {

    if ((in[0] == 0x00) && (in[1] == 0x01)) {

      return true;

    } else {

      return false;

    }

  } else {

    return false;

  }

}

bool scd30_read_measurement(float m[3]) {

  uint8_t out[] = {0x03, 0x00};
  
  if (bsc_write(out, 2) == 2) {

    uint8_t in[18];

    if (bsc_read(in, 18) == 18) {

      uint8_t crc1 = crc8scd30(in, 2);
      uint8_t crc2 = crc8scd30(in+3, 2);
      uint8_t crc3 = crc8scd30(in+6, 2);
      uint8_t crc4 = crc8scd30(in+9, 2);
      uint8_t crc5 = crc8scd30(in+12, 2);
      uint8_t crc6 = crc8scd30(in+15, 2);

      if ((crc1 != in[2]) ||
          (crc2 != in[5]) ||
          (crc3 != in[8]) ||
          (crc4 != in[11]) ||
          (crc5 != in[14]) ||
          (crc6 != in[17])) {

        fprintf(stderr, "CRC ERROR\n");

        return false;

      } else {

        uint32_t co2 = in[0] << 24 | in[1] << 16 | in[3] << 8 | in[4];
        uint32_t t = in[6] << 24 | in[7] << 16 | in[9] << 8 | in[10];
        uint32_t rh = in[12] << 24 | in[13] << 16 | in[15] << 8 | in[16];

        if (co2 == 0) return false;

        memcpy(m, &co2, 4);
        memcpy(m+1, &t, 4);
        memcpy(m+2, &rh, 4);

        return true;

      }

    } else {

      return false;

    }

  } else {

    return false;

  }

  
}

bool scd30_trigger_continuous_measurement(uint16_t pressure) {

  uint8_t out[] = {0x00, 0x10, pressure >> 8, pressure & 0xFF, 0x00};
  out[4] = crc8scd30(&out[2], 2);
  return bsc_write(out, 5) == 5;

}

bool scd30_stop_continuous_measurement(void) {

  uint8_t out[] = {0x01, 0x04};
  return bsc_write(out, 2) == 2;

}


bool scd30_set_measurement_interval(uint16_t interval) {

  if ((interval < 2) || (interval > 1800)) {
    fprintf(stderr, "measurement interval should be between 2 and 1800.\n");
    return false;
  }

  uint8_t out[] = {0x46, 0x00, interval >> 8, interval & 0xFF, 0x00};
  out[4] = crc8scd30(&out[2], 2);
  return bsc_write(out, 5) == 5;

}

bool scd30_get_measurement_interval(uint16_t mi[1]) {

  uint8_t out[] = {0x46, 0x00};
  uint8_t in[3];

  if (do23(out, in)) {

    mi[0] =  (in[0] << 8) | in[1];
    return true;

  } else {

    return false;

  }

}

bool scd30_set_altitude(uint16_t altitude) {

  uint8_t out[] = {0x51, 0x02, altitude >> 8, altitude & 0xFF, 0x00};
  out[4] = crc8scd30(&out[2], 2);
  return bsc_write(out, 5) == 5;

}

bool scd30_get_altitude(uint16_t alt[1]) {

  uint8_t out[] = {0x51, 0x02};
  uint8_t in[3];

  if (do23(out, in)) {

    alt[0] =  (in[0] << 8) | in[1];
    return true;

  } else {

    return false;

  }

}

bool scd30_set_temperature_offset(uint16_t offset) {

  uint8_t out[] = {0x54, 0x03, offset >> 8, offset & 0xFF, 0x00};
  out[4] = crc8scd30(&out[2], 2);
  return bsc_write(out, 5) == 5;

}

bool scd30_get_temperature_offset(uint16_t offset[1]) {

  uint8_t out[] = {0x54, 0x03};
  uint8_t in[3];

  if (do23(out, in)) {

    offset[0] =  (in[0] << 8) | in[1];
    return true;

  } else {

    return false;

  }

}
