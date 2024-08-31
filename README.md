# thinklmi
Utility for easy access to BIOS WMI settings

**WARNING: THIS DRIVER SHOULD NOT BE USED FOR KERNELS 5.17 AND NEWER**
Please use the upstream firmware-attributes class driver if available.
As long as it is included by your distro you can do
  sudo modprobe think-lmi
and then access the WMI attributes under /sys/class/firmware-attributes/thinklmi

The kernel documentation has details on how to use this: https://github.com/torvalds/linux/blob/master/Documentation/ABI/testing/sysfs-class-firmware-attributes

Ths driver and matching user-space utility here should only be used if the kernel driver is not available. Note that thinklmi-user does not currently work with the upstream kernel driver as they use different interfaces (ioctl vs sysfs)
