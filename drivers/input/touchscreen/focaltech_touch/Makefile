#
# Makefile for the focaltech touchscreen drivers.
#

# Each configuration option enables a list of files.

obj-$(CONFIG_TOUCHSCREEN_FTS)	+=  focaltech_core.o
obj-$(CONFIG_TOUCHSCREEN_FTS)	+=  focaltech_ex_fun.o
obj-$(CONFIG_TOUCHSCREEN_FTS)	+=  focaltech_ex_mode.o
obj-$(CONFIG_TOUCHSCREEN_FTS)	+=  focaltech_flash.o
obj-$(CONFIG_TOUCHSCREEN_FTS)	+=  focaltech_gesture.o
obj-$(CONFIG_TOUCHSCREEN_FTS)	+=  focaltech_esdcheck.o
obj-$(CONFIG_TOUCHSCREEN_FTS)   +=  focaltech_i2c.o
obj-$(CONFIG_TOUCHSCREEN_FTS)   +=  focaltech_point_report_check.o
obj-$(CONFIG_TOUCHSCREEN_FTS)   +=  focaltech_flash/

#panben@wind-mobi.com add for factory ffbm remove TP 2017.07.30 start
ifeq ($(TARGET_BUILD_WIND_FOR_FACTORY),true)
DEFS = -DBUILD_WIND_FOR_FACTORY=1
ccflags-y += $(DEFS)
endif
#panben@wind-mobi.com add for factory ffbm remove TP 2017.07.30 end