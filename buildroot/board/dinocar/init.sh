#!/bin/sh
# Dino-Car init script - DEBUG

mount -t proc proc /proc 2>/dev/null
mount -t sysfs sysfs /sys 2>/dev/null
mount -t devtmpfs devtmpfs /dev 2>/dev/null

hostname dinocar

sleep 2

echo "==========================" > /dev/tty1
echo "  Dino-Car booted OK!     " > /dev/tty1
echo "==========================" > /dev/tty1
echo "" > /dev/tty1
uname -a > /dev/tty1
echo "" > /dev/tty1
echo "DRI:" > /dev/tty1
ls -la /dev/dri/ > /dev/tty1 2>&1
echo "FB:" > /dev/tty1
ls -la /dev/fb* > /dev/tty1 2>&1
echo "" > /dev/tty1
echo "Loading vc4..." > /dev/tty1
modprobe vc4 > /dev/tty1 2>&1
sleep 1
echo "DRI after vc4:" > /dev/tty1
ls -la /dev/dri/ > /dev/tty1 2>&1
echo "" > /dev/tty1

# Start Cluster app
echo "Starting Cluster..." > /dev/tty1
export SDL_VIDEODRIVER=kmsdrm
export SDL_RENDER_DRIVER=opengles2
cd /usr/share/cluster
/usr/bin/cluster > /dev/tty1 2>&1
echo "Cluster exited: $?" > /dev/tty1

exec /bin/sh </dev/tty1 >/dev/tty1 2>&1

exec /bin/sh </dev/tty1 >/dev/tty1 2>&1
