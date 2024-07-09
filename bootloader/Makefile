IMAGE_NAME := bootloader
STAGE_ONE := stage1/bin/boot.bin
STAGE_TWO := stage2/bin/boot2.bin

all: $(IMAGE_NAME).bin

$(IMAGE_NAME).bin: $(STAGE_ONE) $(STAGE_TWO)
	rm -f $@
	dd if=$(STAGE_ONE) of=$@ bs=1 seek=0 count=$$(stat -c %s $(STAGE_ONE)) conv=notrunc
	dd if=$(STAGE_TWO) of=$@ bs=1 seek=512 conv=notrunc

.PHONY: $(STAGE_ONE)
$(STAGE_ONE):
	$(MAKE) -C stage1

.PHONY: $(STAGE_TWO)
$(STAGE_TWO):
	$(MAKE) -C stage2

clean:
	$(MAKE) clean -C stage1
	$(MAKE) clean -C stage2
	rm bootloader.bin

.PHONY: all clean
