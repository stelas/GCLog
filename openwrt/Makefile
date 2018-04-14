#
# Copyright (C) 2008 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=gclog
PKG_VERSION:=0.2.4
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=http://www.dateihal.de/$(PKG_NAME)
#PKG_MD5SUM:=

include $(INCLUDE_DIR)/package.mk

define Package/gclog
	SECTION:=utils
	CATEGORY:=Utilities
	TITLE:=Geiger Counter Logger
	URL:=http://www.dateihal.de/cms/gclog
	MAINTAINER:=Steffen Lange <gclog@stelas.de>
endef

define Package/gclog/description
 GCLOG is a lightweight daemon running in the background and
 constantly reporting your Geiger counter readings to various
 radiation monitoring websites.
endef

define Package/gclog/conffiles
/etc/gclog.conf
endef

define Package/gclog/install
	$(INSTALL_DIR)  $(1)/usr/bin
	$(INSTALL_DIR)  $(1)/etc
	$(INSTALL_DIR)  $(1)/etc/init.d
	$(INSTALL_BIN)  $(PKG_BUILD_DIR)/gclog          $(1)/usr/bin/
	$(INSTALL_CONF) $(PKG_BUILD_DIR)/gclog.conf     $(1)/etc/
	$(INSTALL_BIN)  $(PKG_BUILD_DIR)/openwrt/gclogd $(1)/etc/init.d/
endef

$(eval $(call BuildPackage,gclog))
