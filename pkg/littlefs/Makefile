PKG_NAME=littlefs
PKG_URL=https://github.com/ARMmbed/littlefs.git
# v2.1.2
PKG_VERSION=fd204ac2fb4e05ffce6db6d1be67e0342c267cd7
PKG_LICENSE=Apache-2.0

.PHONY: all

all:
	"$(MAKE)" -C $(PKG_BUILDDIR) -f $(CURDIR)/Makefile.littlefs

include $(RIOTBASE)/pkg/pkg.mk
