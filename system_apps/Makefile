LIBC_DIR := libc
TARGET_DIRS := $(shell find . -mindepth 2 -maxdepth 2 -path ./libc/Makefile -prune -o -type f -name "Makefile" -exec dirname {} \;)

.PHONY: all
all: $(TARGET_DIRS)

.PHONY: $(TARGET_DIRS)
$(TARGET_DIRS): $(LIBC_DIR)
	$(MAKE) -C $@

.PHONY: $(LIBC_DIR)
$(LIBC_DIR):
	$(MAKE) -C $@

CLEAN_TARGETS := $(addsuffix /clean,$(TARGET_DIRS)) $(addsuffix /clean,$(LIBC_DIR))
.PHONY: clean
clean: $(CLEAN_TARGETS)

.PHONY: $(CLEAN_TARGETS)
$(CLEAN_TARGETS):
	$(MAKE) clean -C $(dir $@)
