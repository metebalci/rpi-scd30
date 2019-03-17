# rpi-scd30

This is a sample of how to use Sensirion SCD30 on Raspberry Pi 3.

It does not use i2c device on Linux, so you should disable the i2c support with raspi-config. 
That means there is no /dev/i2c* devices. It maps the peripheral memory, and requires sudo rights for this.

# bsc.*

bsc controls the Broadcom Serial Controller directly to use the i2c bus.

bsc uses the BSC1 on Raspberry Pi 3. This is defined in bsc.c as:

```
#define BSC_BASE (0x804000 >> 2)
```

0x804000 is the offset of BSC1 in peripheral address space.

In order to use scd30 in your project, you should init bsc first, and deinit bsc last in your code. 
You can get the arguments peri_addr and peri_size from bcm_host.h on Raspberry Pi 3. See co2trh.c as an example.

# crc8scd30.*

CRC implementation for SCD30 i2c interface.

crc8scd30.* is taken from https://github.com/metebalci/crc8scd30. 

# scd30.*

scd30.* is the interface to SCD30.

All functions in scd30 return a bool indicating if the command was successful.

# Build

Just use `make`.

If you want to use it in your project, add bsc.o, crc8scd30.o and scd30.o and scd30.h to your project.

# Build Sample

The repo contains also a sample application called co2trh (co2, temperature, relative humidity).

Use `make` to compile the required objects first, then use `make -f Makefile.rpi3` to compile the sample app.

Running `sudo ./co2trh` with a SCD30 connected to a Raspberry Pi 3 on i2c bus will return an output like:

```
594.0 26.0 35.8
```

First is CO2 in ppm, second is temperature in C, third is relative humidity in %.

co2trh is set to compensate for the altitude, 420m.

# How am I using SCD30 and this code ? 

- SCD30 is connected to a Raspberry Pi 3 at home. 
- I have a cron job on Raspberry Pi 3 running every minute.
- This cron job runs co2trh and pipes the output to a python script.
- Python script sends the data to a graphite carbon instance in another computer.

So I collect the data like this:

! [SCD30 CO2 Chart](https://raw.githubusercontent.com/metebalci/rpi-scd30/master/scd30_co2.png)

! [SCD30 T-RH Chart](https://raw.githubusercontent.com/metebalci/rpi-scd30/master/scd30_trh.png)
