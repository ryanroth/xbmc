NATIVE_BUILD=1
include ../Makefile.include
DEPS= ../Makefile.include Makefile

APPNAME=yasm
VERSION=1.1.0
SOURCE=$(APPNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

# configuration settings
CONFIGURE=./configure --prefix=$(PREFIX)

APP=$(PLATFORM)/$(APPNAME)

CLEAN_FILES=$(ARCHIVE) $(PLATFORM)

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	$(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	-rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	$(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); $(CONFIGURE)

$(APP): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)

.installed-$(PLATFORM): $(APP)
	$(MAKE) -C $(PLATFORM) install
	rm -rf $(PREFIX)/lib/libyasm.a
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm  .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

