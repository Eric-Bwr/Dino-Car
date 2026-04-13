#!/bin/sh
# Dino-Car init script

mount -t proc proc /proc 2>/dev/null
mount -t sysfs sysfs /sys 2>/dev/null
mount -t devtmpfs devtmpfs /dev 2>/dev/null

hostname dinocar

sleep 2

modprobe vc4 2>/dev/null
sleep 1

# Start Cluster app
export SDL_VIDEODRIVER=kmsdrm
export SDL_RENDER_DRIVER=opengles2
cd /usr/share/cluster
/usr/bin/cluster > /dev/tty1 2>&1
echo "Cluster exited: $?" > /dev/tty1

exec /bin/sh </dev/tty1 >/dev/tty1 2>&1
