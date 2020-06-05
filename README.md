
#  [OFFICIAL][3.8.120] kernel for Lenovo Smart Tab M10 wifi (TB-X605F/L)

Lenovo released this source code on 4 June 2020.

According to Lenovo, the same kernel works for the TB-X605F (wifi) and TB-X605L (LTE) variants.

The code is msm-3.18.120, which is the same version as included in the offically-released stock ROM for Android Pie.

## compile
To compile the kernel:

	export PATH=$PWD/prebuilts/gcc/linux-x86/aarch64/aarch64-linux-android-4.9/bin/:$PATH
	rm -rf out/
	make ARCH=arm64 CROSS_COMPILE=aarch64-linux-android- O=out m10_msmcortex_defconfig
	make ARCH=arm64 CROSS_COMPILE=aarch64-linux-android- O=out -j8

The kernel is built to:  out/arch/arm64/boot/image.gz
