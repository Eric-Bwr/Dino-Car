CLUSTER_SITE = $(BR2_EXTERNAL_DINOCAR_PATH)/../InstrumentCluster
CLUSTER_SITE_METHOD = local
CLUSTER_DEPENDENCIES = sdl2 sdl2_ttf sdl2_gfx sdl2_image libgpiod
CLUSTER_INSTALL_TARGET = YES

define CLUSTER_BUILD_CMDS
	mkdir -p $(@D)/build
	cd $(@D)/build && $(TARGET_CONFIGURE_OPTS) \
		PKG_CONFIG="$(HOST_DIR)/bin/pkg-config" \
		PKG_CONFIG_SYSROOT_DIR="$(STAGING_DIR)" \
		PKG_CONFIG_LIBDIR="$(STAGING_DIR)/usr/lib/pkgconfig" \
		cmake .. \
		-DCMAKE_C_COMPILER=$(TARGET_CC) \
		-DCMAKE_CXX_COMPILER=$(TARGET_CXX) \
		-DCMAKE_FIND_ROOT_PATH=$(STAGING_DIR) \
		-DCMAKE_SYSROOT=$(STAGING_DIR) \
		-DCMAKE_SYSTEM_NAME=Linux \
		-DCMAKE_CXX_FLAGS="$(TARGET_CXXFLAGS) -DIS_RASPI=1" \
		&& $(MAKE)
endef

define CLUSTER_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/build/Cluster $(TARGET_DIR)/usr/bin/cluster
	mkdir -p $(TARGET_DIR)/usr/share/cluster/assets
	cp -r $(@D)/assets/* $(TARGET_DIR)/usr/share/cluster/assets/
endef

$(eval $(generic-package))
