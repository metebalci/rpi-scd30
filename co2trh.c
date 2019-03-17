#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdbool.h>
#include <unistd.h>
#include <bcm_host.h>
#include "bsc.h"
#include "crc8scd30.h"
#include "scd30.h"
 
int main() {

  off_t peri_base = bcm_host_get_peripheral_address();
  size_t peri_size = bcm_host_get_peripheral_size();
 
  if (!bsc_init(peri_base, peri_size)) {
    printf("bsc failed to init\n");
    return EXIT_FAILURE;
  }

  // 100 Khz
  bsc_set_scl_freq(10000);
  // 250 / 0.1ms = 2500
  bsc_set_clkt_tout(2500);

  bsc_set_slave_addr(0x61);

  if (!scd30_soft_reset()) {
    fprintf(stderr, "soft_reset error\n");
    return EXIT_FAILURE;
  }

  sleep(2);

  if (!scd30_set_altitude(420)) {
    fprintf(stderr, "set_altitude error\n");
    return EXIT_FAILURE;
  }
  if (!scd30_set_measurement_interval(2)) {
    fprintf(stderr, "set_measurement_interval error\n");
    return EXIT_FAILURE;
  }
  if (!scd30_trigger_continuous_measurement(0)) {
    fprintf(stderr, "trigger_continuous_measurement error\n");
    return EXIT_FAILURE;
  }

  bool done = false;

  for (int i = 0; i < 30; i++) {

    if (scd30_get_data_ready_status()) {

      float m[3];

      if (scd30_read_measurement(m)) {
        printf("%.1f %.1f %.1f\n", m[0], m[1], m[2]);
        done = true;
        break;
      }

    }

    sleep(1);

  }

  bsc_deinit();

  return done ? EXIT_SUCCESS : EXIT_FAILURE;

}
