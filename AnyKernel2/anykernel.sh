# AnyKernel2 Ramdisk Mod Script
# osm0sis @ xda-developers

## AnyKernel setup
# begin properties
properties() { '
kernel.string=Fire by theimpulson @ xda-developers
do.devicecheck=0
do.modules=1
do.cleanup=1
do.cleanuponabort=0
device.name1=
device.name2=
'; } # end properties

# shell variables
block=/dev/block/bootdevice/by-name/boot;
is_slot_device=0;
ramdisk_compression=auto;


# Detect whether we're in recovery or booted up
ps | grep zygote | grep -v grep >/dev/null && in_recovery=false || in_recovery=true;
! $in_recovery || ps -A 2>/dev/null | grep zygote | grep -v grep >/dev/null && in_recovery=false;
! $in_recovery || id | grep -q 'uid=0' || in_recovery=false;


## AnyKernel methods (DO NOT CHANGE)
# import patching functions/variables - see for reference
. /tmp/anykernel/tools/ak2-core.sh;


## AnyKernel install
split_boot;


# Mount system to get some information about the user's setup (only needed in recovery)
if $in_recovery; then
  umount /system;
  umount /system 2>/dev/null;
  mkdir /system_root 2>/dev/null;
  mount -o ro -t auto /dev/block/bootdevice/by-name/system$slot /system_root;
  mount -o bind /system_root/system /system;
fi;

# Unmount system
if $in_recovery; then
  umount /system;
  umount /system_root;
  rmdir /system_root;
  mount -o ro -t auto /system;
fi;


# Install the boot image
flash_boot;
