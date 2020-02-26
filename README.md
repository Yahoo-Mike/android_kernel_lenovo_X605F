
#  [UNOFFICIAL][3.8.71] kernel for Lenovo Smart Tab M10 wifi (TB-X605F)

Lenovo has not officially released the M10 kernel source. So this is a modified copy of the released source for the P10 (TB-X705F), which is a similar device using the same SOC (SDA450).

Despite releasing 3.8.120 on all stock ROMs, Lenovo only released the source for 3.18.71.  That's why this first port is at 3.18.71. 

**This is still a work in progress.**

See XDA thread for list of what's working and what's not.

## compile
To compile kernel:

_OPTS="-j8 -o ../out/target/product/msm8953_64/obj/KERNEL_OBJ ARCH=arm64 CROSS_COMPILE=aarch64-linux-android- KCFLAGS=-mno-android "

	cd kernel_dir
	#make clean & make mrproper
	make $_OPTS -C . YM_x605f_defconfig
	make $_OPTS -C . headers_install
	make $_OPTS -C .

## building modules
It is not necessary, but you can build modules with:

	make $_OPTS -C . modules
	make $_OPTS -C . INSTALL_MOD_PATH=../../system INSTALL_MOD_STRIP=1 modules_install

## enforcing kernel equivalence for modules
If you want to ensure that modules are all build for the same kernel, set CONFIG_MODULE_SIG_FORCE=y in YM_x605f_defconfig.

If you do this, you will also need to compile and sign the /vendor/lib/modules/pronto/wlan_pronto.ko from CAF to make wifi work.

To compile pronto_wlan against a successfully compiled kernel:

_WLAN="WLAN_ROOT=prima_dir MODNAME=pronto_wlan CONFIG_PRONTO_WLAN=m"

	cd prima_dir
	#make clean
	make $_OPTS $_WLAN -C kernel_dir M=prima_dir modules
	make $_OPTS $_WLAN -C kernel_dir M=prima_dir INSTALL_MOD_PATH=../../system INSTALL_MOD_STRIP=1 modules_install

**Note**: prima_dir is ./vendor/qcom/opensource/wlan/prima.  Use CAF msm8953_64 release for the version of Android you're building for.
