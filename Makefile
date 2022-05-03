#!/bin/make

# This version of the Makefile does not use setup, loop, or main symbols

# Adjust target config file for nrf52832 vs nrf52840
# TARGET_CONFIG_FILE=target_nrf52840.mk
TARGET_CONFIG_FILE=target_nrf52832.mk

include $(TARGET_CONFIG_FILE)
include config.mk
-include private.mk

# TFLM_PATH=$(HOME)/code/tflite-micro
# TFLM_LIB_PATH=$(TFLM_PATH)/tensorflow/lite/micro/tools/make/gen/cortex_m_generic_cortex-m4_default/lib
TFLM_DIR=$(SRC_DIR)/tensorflow
# THIRD_PARTY_DIR=$(SRC_DIR)/third_party
# C_SRC_DIR=$(shell find $(TFLM_DIR) -name "*.c" \( -path "*/c/*" -o -name "*util.c" \) )
# C_INC_DIR=$(shell find $(TFLM_DIR) -type d \( -path "*/c/*" -o -path "*/experimental/*" \) )
# TFLM_SRC_FILES=$(shell find $(TFLM_DIR) -name "*.c" ! -path "*/tools/*" ! -path "*/c/*" ! -name "*util.c") # exclude some directories
TFLM_SRC_FILES=$(shell find $(TFLM_DIR) -name "*.c" ! -path "*/tools/*") # exclude tools directory
TFLM_INC_DIRS=$(shell find $(TFLM_DIR) -type d)

SOURCE_FILES=	$(INC_DIR)/startup.S \
				$(wildcard $(SRC_DIR)/*.c) \
				$(wildcard $(SRC_DIR)/*.cpp) \
				$(wildcard $(TARGET_DIR)/*.c) \
				$(wildcard $(TARGET_DIR)/*.cpp) \
				$(TFLM_SRC_FILES) \
				$(SHARED_PATH)/ipc/cs_IpcRamData.c \
				$(TARGET).c

# First initialize, then create .hex file, then .bin file and file end with info
all: init $(TARGET).hex $(TARGET).bin $(TARGET).info
	@echo "Result: $(TARGET).hex (and $(TARGET).bin)"

clean:
	@rm -f $(TARGET).*
	@echo "Cleaned build directory"

init: $(TARGET_CONFIG_FILE)
	@echo "Use file: $(TARGET_CONFIG_FILE)"
	@echo 'Create build directory'
	@mkdir -p $(BUILD_PATH)
	@rm -f $(INC_DIR)/microapp_header_symbols.ld

.PHONY:
$(INC_DIR)/microapp_header_symbols.ld: $(TARGET).bin.tmp
	@echo "Use python script to generate $@ file with valid header symbols"
	@scripts/microapp_make.py -i $^ $@

.PHONY:
$(INC_DIR)/microapp_header_dummy_symbols.ld:
	@echo "Use python script to generate file with dummy values"
	@scripts/microapp_make.py $(INC_DIR)/microapp_header_symbols.ld

.tmp.TARGET_CONFIG_FILE.$(TARGET_CONFIG_FILE):
	@rm -f .tmp.TARGET_CONFIG_FILE.*
	touch $@

$(INC_DIR)/microapp_target_symbols.ld: $(TARGET_CONFIG_FILE) .tmp.TARGET_CONFIG_FILE.$(TARGET_CONFIG_FILE)
	@echo 'This script requires the presence of "bc" on the command-line'
	@echo 'Generate target symbols (from .mk file to .ld file)'
	@echo '/* Auto-generated file */' > $@
	@echo "APPLICATION_START_ADDRESS = $(START_ADDRESS);" >> $@
	@echo '' >> $@
	@echo "RAM_END = $(RAM_END);" >> $@

$(INC_DIR)/microapp_symbols.ld: $(INC_DIR)/microapp_symbols.ld.in
	@echo "Generate linker symbols using C header files (using the compiler)"
#	@$(CC) -CC -E -P -x c -I$(INC_DIR) -I$(LIB_DIR) -I$(TARGET_DIR) -ltensorflow-microlite -L$(TFLM_LIB_PATH) $^ -o $@ -lstdc++
	@$(CC) -CC -E -P -x c -I$(INC_DIR) -I$(LIB_DIR) -I$(TARGET_DIR) -I$(SRC_DIR) $^ -o $@ -lstdc++
	@echo "File $@ now up to date"

$(TARGET).elf.tmp.deps: $(INC_DIR)/microapp_header_dummy_symbols.ld $(INC_DIR)/microapp_symbols.ld $(INC_DIR)/microapp_target_symbols.ld
	@echo "Dependencies for $(TARGET).elf.tmp fulfilled"

$(TARGET).elf.tmp: $(SOURCE_FILES)
	@echo "Compile without firmware header"
#	@$(CC) -x c -I$(C_INC_DIR) -Tgeneric_gcc_nrf52.ld -o $@
#	@$(CC) $(FLAGS) $^ -I$(SHARED_PATH) -I$(INC_DIR) -I$(TARGET_DIR) -I$(LIB_DIR) -I$(TFLM_LIB_PATH) -ltensorflow-microlite -L$(TFLM_LIB_PATH)  -L$(INC_DIR) -Tgeneric_gcc_nrf52.ld -o $@ -lstdc++
	@$(CC) $(FLAGS) $^ -I$(SHARED_PATH) -I$(INC_DIR) -I$(TARGET_DIR) -I$(SRC_DIR) -L$(INC_DIR) -L$(SRC_DIR) -Tgeneric_gcc_nrf52.ld -o $@ -lstdc++

.ALWAYS:
$(TARGET).elf.deps: $(INC_DIR)/microapp_header_symbols.ld
	@echo "Run scripts"

$(TARGET).elf: $(SOURCE_FILES)
	@echo "Compile with firmware header"
#	@$(CC) -x c -I$(C_INC_DIR) -Tgeneric_gcc_nrf52.ld -o $@
#	@$(CC) $(FLAGS) $^ -I$(SHARED_PATH) -I$(INC_DIR) -I$(TARGET_DIR) -I$(LIB_DIR) -I$(TFLM_LIB_PATH) -ltensorflow-microlite -L$(TFLM_LIB_PATH) -L$(INC_DIR) -Tgeneric_gcc_nrf52.ld -o $@ -lstdc++
	@$(CC) $(FLAGS) $^ -I$(SHARED_PATH) -I$(INC_DIR) -I$(TARGET_DIR) -I$(SRC_DIR) -L$(INC_DIR) -L$(SRC_DIR) -Tgeneric_gcc_nrf52.ld -o $@ -lstdc++

$(TARGET).c: $(TARGET_SOURCE)
	@echo "Script from .ino file to .c file (just adding Arduino.h header)"
	@echo '#include <Arduino.h>' > $(TARGET).c
	@cat $(TARGET_SOURCE) >> $(TARGET).c

$(TARGET).hex: $(TARGET).elf.deps $(TARGET).elf
	@echo "Create hex file from elf file"
	@$(OBJCOPY) -O ihex $(TARGET).elf $@

$(TARGET).bin.tmp: $(TARGET).elf.tmp.deps $(TARGET).elf.tmp
	@echo "Create temporary bin file from temporary elf file"
	@$(OBJCOPY) -O binary $(TARGET).elf.tmp $@

$(TARGET).bin: $(TARGET).elf
	@echo "Create final binary file"
	@$(OBJCOPY) -O binary $(TARGET).elf $@

$(TARGET).info:
	@echo "$(shell cat $(INC_DIR)/microapp_header_symbols.ld)"

flash: all
	echo nrfjprog -f nrf52 --program $(TARGET).hex --sectorerase
	nrfjprog -f nrf52 --program $(TARGET).hex --sectorerase

read:
	nrfjprog -f nrf52 --memrd $(START_ADDRESS) --w 8 --n 400

download: init
	nrfjprog -f nrf52 --memrd $(START_ADDRESS) --w 16 --n 0x2000 | tr [:upper:] [:lower:] | cut -f2 -d':' | cut -f1 -d'|' > $(BUILD_PATH)/download.txt

dump: $(TARGET).bin
	hexdump -v $^ | cut -f2- -d' ' > $(TARGET).txt

compare: dump download
	meld $(TARGET).txt $(BUILD_PATH)/download.txt

erase:
	$(eval STOP_ADDRESS_WITHOUT_PREFIX=$(shell echo 'obase=16;ibase=16;$(START_ADDRESS_WITHOUT_PREFIX)+$(MICROAPP_PAGES)*1000' | bc))
	echo nrfjprog --erasepage $(START_ADDRESS)-0x$(STOP_ADDRESS_WITHOUT_PREFIX)
	nrfjprog --erasepage $(START_ADDRESS)-0x$(STOP_ADDRESS_WITHOUT_PREFIX)

reset:
	nrfjprog --reset

ota-upload:
	scripts/upload_microapp.py --keyFile $(KEYS_JSON) -a $(BLE_ADDRESS) -f $(TARGET).bin

inspect: $(TARGET).elf
	$(OBJDUMP) -x $^

inspect-headers: $(TARGET).elf
	$(OBJDUMP) -h $^

size: $(TARGET).elf
	$(SIZE) -B $^ | tail -n1 | tr '\t' ' ' | tr -s ' ' | sed 's/^ //g' | cut -f1,2 -d ' ' | tr ' ' '+' \
		| bc | xargs -i echo "Total size: {} B"
	$(SIZE) -B $^ | tail -n1 | tr '\t' ' ' | tr -s ' ' | sed 's/^ //g' | cut -f1 -d ' ' | tr ' ' '+' \
		| bc | xargs -i echo "     flash: {} B"
	$(SIZE) -B $^ | tail -n1 | tr '\t' ' ' | tr -s ' ' | sed 's/^ //g' | cut -f1 -d ' ' | tr ' ' '+' \
		| xargs -i echo "({} + 1023) / 1024" | bc | xargs -i echo "     pages: {}"

help:
	echo "make\t\t\tbuild .elf and .hex files (requires the ARM cross-compiler)"
	echo "make flash\t\tflash .hex file to target (requires nrfjprog)"
	echo "make inspect\t\tobjdump everything"
	echo "make size\t\tshow size information"

.PHONY: flash inspect help read reset erase all

.SILENT: all init flash inspect size help read reset erase clean

print: $(SOURCE_FILES)
	@echo $(SOURCE_FILES)