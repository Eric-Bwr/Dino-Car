# Dino-Car Buildroot Image

Minimal Raspberry Pi Zero 2 W Image - bootet direkt ins Instrument Cluster.
Kein Desktop, kein Login, kein Netzwerk - nur euer Display.

Buildroot ist als Git-Submodule eingebunden (Version 2026.02 LTS).

## Setup

```bash
# 1. Repo klonen (mit Submodule)
git clone --recurse-submodules https://github.com/EUER_REPO/Dino-Car.git
cd Dino-Car

# Falls schon geklont, Submodule nachladen:
git submodule update --init --depth 1

# 2. Dino-Car Config laden
cd buildroot/buildroot
make BR2_EXTERNAL=.. dinocar_defconfig

# 3. Bauen
make -j$(nproc)
```

## SD-Karte flashen

```bash
# SD-Karte finden (z.B. /dev/sdb)
lsblk

# Image schreiben
sudo dd if=output/images/sdcard.img of=/dev/sdX bs=4M status=progress conv=fsync
sync
```

## Was ist drin?

- Raspberry Pi Zero 2 W (64-bit, aarch64)
- Linux Kernel mit USB Serial (Arduino Kommunikation via /dev/ttyACM0)
- SDL2 + TTF + GFX + IMAGE (KMSDRM Backend)
- wiringPi (GPIO für Proximity-Sensor + Buttons)
- Cluster App mit allen Assets

## Rebuild nach Code-Änderungen

```bash
cd buildroot/buildroot
make cluster-rebuild && make -j$(nproc)
```
