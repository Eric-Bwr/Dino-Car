#!/bin/sh
# Dino-Car init script - starts Cluster directly on framebuffer

# Mount essential filesystems
mount -t proc proc /proc
mount -t sysfs sysfs /sys
mount -t devtmpfs devtmpfs /dev

# Set hostname
hostname dinocar

# Load kernel modules
modprobe spi-bcm2835
modprobe i2c-bcm2835
modprobe vc4

# Wait for display + USB devices
sleep 0.5

# Disable kernel messages on console
echo 0 > /proc/sys/kernel/printk

# Set SDL environment
export SDL_VIDEODRIVER=kmsdrm
export SDL_RENDER_DRIVER=opengles2

# Start the Cluster app
cd /usr/share/cluster
exec /usr/bin/cluster
