
include $(TOPDIR)/rules.mk

PKG_NAME:=nproxy
PKG_VERSION:=0.1
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tgz
PKG_SOURCE_URL:=http://10.10.10.101
PKG_MD5SUM:=034dd5a90abc5cd5c0fdff10965093f1

PKG_INSTALL:=1

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/nproxy/Default
  SUBMENU:=Web Servers/Proxies
  SECTION:=net
  CATEGORY:=Network
  TITLE:=Lightweight Threaded HTTP Proxy
  URL:=http://tinyproxy.sourceforge.net/
endef

define Package/nproxy/Default/description
    NProxy is a fast, lightweight, threaded and programmable HTTP Proxy
endef

define Package/nproxy
 $(call Package/nproxy/Default)
 DEPENDS:=+libpthread +lua
endef

define Package/nproxy/conffiles
/etc/config/nproxy
endef

define Package/nproxy/description
 $(call Package/nproxy/Default/description)
endef

CMAKE_OPTIONS+= \
	-DNPROXY_USER=nobody \
	-DNPROXY_GROUP=nogroup \
	-DSYSCONF_INSTALL_DIR=/etc/nproxy/ \
	-DLUA_MATH_LIBRARY=""

TARGET_LDFLAGS += -lm

define Package/nproxy/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/sbin/nproxyd $(1)/usr/sbin/

	$(INSTALL_DIR) $(1)/usr/lib/nproxy/
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/nproxy/libnn.so* $(1)/usr/lib/nproxy/

	$(INSTALL_DIR) $(1)/usr/share/nproxy/templates
	$(CP) $(PKG_INSTALL_DIR)/usr/share/nproxy/templates/*.html $(1)/usr/share/nproxy/templates/

	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_CONF) ./files/nproxy.config $(1)/etc/config/nproxy

	$(INSTALL_DIR) $(1)/etc/nproxy
	$(CP) $(PKG_INSTALL_DIR)/etc/nproxy/nproxyd.conf.xml $(1)/etc/nproxy/
	$(INSTALL_DIR) $(1)/etc/nproxy/scripts
	$(CP) $(PKG_INSTALL_DIR)/etc/nproxy/scripts/*.lua $(1)/etc/nproxy/scripts/

	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/nproxy.init $(1)/etc/init.d/nproxy
endef

$(eval $(call BuildPackage,nproxy))
