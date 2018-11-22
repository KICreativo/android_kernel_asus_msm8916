#!/bin/bash
BUILD_START=$(date +"%s")

# Colours
blue='\033[0;34m'
cyan='\033[0;36m'
yellow='\033[0;33m'
red='\033[0;31m'
nocol='\033[0m'

# Kernel details
KERNEL_NAME="ZenProject"
VERSION="v1.0"
DATE=$(date +"%d-%m-%Y-%I-%M")
DEVICE="Z00RD"
OUT="msm8916"
FINAL_ZIP=$KERNEL_NAME-$VERSION-$DATE-$DEVICE.zip
defconfig=ze500kg-custom_defconfig

# Dirs
ANYKERNEL_DIR=$TRAVIS_BUILD_DIR/AnyKernel2
KERNEL_IMG=$TRAVIS_BUILD_DIR/arch/arm64/boot/Image.gz-dtb
UPLOAD_DIR=$TRAVIS_BUILD_DIR/$OUT

# Export
export ARCH=arm64
export CROSS_COMPILE=~/aarch64-linux-android-4.9/bin/aarch64-linux-android-
export KBUILD_BUILD_USER="KI.Lab-dev"
export KBUILD_BUILD_HOST="ZenProject-dev"

make $defconfig
make -j16

mkdir -p tmp_mod
make -j4 modules_install INSTALL_MOD_PATH=tmp_mod INSTALL_MOD_STRIP=1
find tmp_mod/ -name '*.ko' -type f -exec cp '{}' $ANYKERNEL_DIR/modules/system/lib/modules/ \;
cp $KERNEL_IMG $ANYKERNEL_DIR
mkdir -p $UPLOAD_DIR
cd $ANYKERNEL_DIR
zip -r9 UPDATE-AnyKernel2.zip * -x README UPDATE-AnyKernel2.zip
mv $ANYKERNEL_DIR/UPDATE-AnyKernel2.zip $UPLOAD_DIR/$FINAL_ZIP

# Cleanup
rm $ANYKERNEL_DIR/Image.gz-dtb

BUILD_END=$(date +"%s")
DIFF=$(($BUILD_END - $BUILD_START))
echo -e "$yellow Build completed in $(($DIFF / 60)) minute(s) and $(($DIFF % 60)) seconds.$nocol"
