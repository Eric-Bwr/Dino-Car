#!/bin/sh
# Post-build script for Dino-Car Buildroot image

TARGET_DIR="$1"

# Install init script
install -m 0755 "$(dirname "$0")/init.sh" "${TARGET_DIR}/sbin/init"

# Remove unnecessary files to save space
rm -rf "${TARGET_DIR}/usr/share/man"
rm -rf "${TARGET_DIR}/usr/share/doc"
rm -rf "${TARGET_DIR}/usr/share/info"

# Create minimal /etc/fstab
cat > "${TARGET_DIR}/etc/fstab" << 'EOF'
/dev/mmcblk0p2  /       ext4    defaults,noatime  0  1
EOF


