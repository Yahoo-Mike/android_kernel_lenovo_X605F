
#  [UNOFFICIAL][3.8.71] kernel for Lenovo Smart Tab M10 wifi (TB-X605F)

Lenovo has not officially released the M10 kernel source. So this is a modified copy of the released source for the P10 (TB-X705F), which is a similar device.  You will also need to compile and sign the /vendor/lib/modules/pronto/wlan_pronto.ko from CAF to make wifi work.

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
	make $_OPTS -C . modules
	make $_OPTS -C . INSTALL_MOD_PATH=../../system INSTALL_MOD_STRIP=1 modules_install

To compile pronto_wlan against a successfully compiled kernel:

_WLAN="WLAN_ROOT=prima_dir MODNAME=pronto_wlan CONFIG_PRONTO_WLAN=m"

	cd prima_dir
	#make clean
	make $_OPTS $_WLAN -C kernel_dir M=prima_dir modules
	make $_OPTS $_WLAN -C kernel_dir M=prima_dir INSTALL_MOD_PATH=../../system INSTALL_MOD_STRIP=1 modules_install

**Note**: prima_dir is ./vendor/qcom/opensource/wlan/prima.  Use CAF QAEP msm8953 release for Android Pie. msm8953.
