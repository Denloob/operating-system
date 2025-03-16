LIBC_DIR := libc
LIBGX_DIR := libgx
TARGET_DIRS := $(shell find  -mindepth 2 -maxdepth 2 -path ./libc/Makefile -prune -o -path ./libgx/Makefile -prune -o -type f -name "Makefile" -exec dirname {} \;)

.PHONY: all
all: $(TARGET_DIRS)

.PHONY: $(TARGET_DIRS)
$(TARGET_DIRS): $(LIBC_DIR) $(LIBGX_DIR)
	$(MAKE) -C $@

.PHONY: $(LIBGX_DIR)
$(LIBGX_DIR):
	$(MAKE) -C $@

.PHONY: $(LIBC_DIR)
$(LIBC_DIR):
	$(MAKE) -C $@

CLEAN_TARGETS := $(addsuffix /clean,$(TARGET_DIRS)) $(addsuffix /clean,$(LIBC_DIR))  $(addsuffix /clean,$(LIBGX_DIR))
.PHONY: clean
clean: $(CLEAN_TARGETS)

.PHONY: $(CLEAN_TARGETS)
$(CLEAN_TARGETS):
	$(MAKE) clean -C $(dir $@)
