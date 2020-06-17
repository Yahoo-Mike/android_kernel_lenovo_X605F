
#  [unofficial][3.8.120] kernel for Lenovo Smart Tab M10 wifi (TB-X605F/L)

This is based on the official kernel source code released by Lenovo on 4 June 2020.

It has been modified to include a defconfig for LineageOS 17.1.

It also includes wlan drivers in drivers/staging/prima.  The drivers come from LA.UM.8.6.2.r1-07600-89xx.0 (1 June 2020) for msm8953_64 at Android 10.

The code is msm-3.18.120, which is the same version as included in the offically-released stock ROM (for Android Pie).

## compile
To compile the kernel:

	export PATH=$PWD/prebuilts/gcc/linux-x86/aarch64/aarch64-linux-android-4.9/bin/:$PATH
	rm -rf out/
	make ARCH=arm64 CROSS_COMPILE=aarch64-linux-android- O=out m10_lineageos_defconfig
	make ARCH=arm64 CROSS_COMPILE=aarch64-linux-android- O=out headers_install
	make ARCH=arm64 CROSS_COMPILE=aarch64-linux-android- O=out -j8

To include modules (including wlan.ko):
	make ARCH=arm64 CROSS_COMPILE=aarch64-linux-android- O=out modules
	make ARCH=arm64 CROSS_COMPILE=aarch64-linux-android- O=out INSTALL_MOD_PATH=../system INSTALL_MOD_STRIP=1 modules_install
	
The kernel is built to:  out/arch/arm64/boot/Image
Modules are built to:    system/lib/modules

