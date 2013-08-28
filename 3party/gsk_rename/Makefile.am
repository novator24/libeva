SUBDIRS = src doc

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = eva-1.0.pc
EXTRA_DIST = eva-1.0.pc.in
DISTCHECK_CONFIGURE_FLAGS = --enable-gtk-doc

DEBARCH = @ARCH@
deb:
	test -r eva-@VERSION@.tar.gz || $(MAKE) dist
	$(srcdir)/utils/pkgwrite --format=debian \
	  --tarball=eva-@VERSION@.tar.gz \
	  --output=eva-packages \
	  --pkgwriteinfo-file=pkgwriteinfo \
	  --arch=$(DEBARCH)

rpm:
	test -r eva-@VERSION@.tar.gz || $(MAKE) dist
	$(srcdir)/utils/pkgwrite --format=redhat \
	  --tarball=eva-@VERSION@.tar.gz \
	  --output=eva-packages \
	  --pkgwriteinfo-file=pkgwriteinfo \
	  --arch=$(DEBARCH)

clean-packages:
	rm -f eva-@VERSION@.tar.gz
	$(MAKE) deb
