SHELL := /bin/bash
.SHELLFLAGS := -e -o pipefail -c
.ONESHELL:
.DEFAULT_GOAL := all

PROJECT_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
PROJECT_NAME := esp32-arduino

FIRMWARE_DIR := $(PROJECT_DIR)/firmware
ESP32_DIR := $(FIRMWARE_DIR)/esp32
ESP8266_DIR := $(FIRMWARE_DIR)/esp8266
HOST_PACKED_DIR := $(PROJECT_DIR)/host/packed
PACK_DIR := $(PROJECT_DIR)/pack
VENDOR_DIR := $(PROJECT_DIR)/vendor

PORTAL_DIR := $(FIRMWARE_DIR)/portal
PORTAL_CONFIG := $(PORTAL_DIR)/config.h
PORTAL_SOURCES := $(wildcard $(PORTAL_DIR)/*.h) $(wildcard $(PORTAL_DIR)/*.cpp)
ESP32_SKETCH := $(ESP32_DIR)/$(PROJECT_NAME).ino
ESP8266_SKETCH := $(ESP8266_DIR)/esp8266-arduino.ino
ESP32_PARTITIONS := $(ESP32_DIR)/partitions.csv
ESP32_PARTITIONS_8M := $(ESP32_DIR)/partitions-8m.csv
ESP32_SKETCH_SOURCES := $(wildcard $(ESP32_DIR)/*.ino) $(wildcard $(ESP32_DIR)/*.h) $(wildcard $(ESP32_DIR)/*.cpp) $(PORTAL_SOURCES)
ESP8266_SKETCH_SOURCES := $(wildcard $(ESP8266_DIR)/*.ino) $(wildcard $(ESP8266_DIR)/*.h) $(wildcard $(ESP8266_DIR)/*.cpp) $(PORTAL_SOURCES)
DNS_PREPARER := $(FIRMWARE_DIR)/dns/prepare-dns-library.py

BUILD_DIR := $(PROJECT_DIR)/build
SOURCE_DATA_DIR := $(HOST_PACKED_DIR)
MINIFIED_DATA_DIR := $(BUILD_DIR)/packed.mini
DATA_DIR := $(BUILD_DIR)/littlefs-data
SERVER_CERT := $(BUILD_DIR)/tls/server.crt
SERVER_KEY := $(BUILD_DIR)/tls/server.key
SERVER_HEADER := $(BUILD_DIR)/tls/server_certs.h
CERT_EMBEDDER := $(FIRMWARE_DIR)/tls/embed_cert.py
DATA_PREPARER := $(PACK_DIR)/prepare-data.sh
COMMON_BUILD_DIR := $(BUILD_DIR)/common
PICO_BUILD_DIR := $(BUILD_DIR)/pico
S2_BUILD_DIR := $(BUILD_DIR)/s2
S3_BUILD_DIR := $(BUILD_DIR)/s3
C3_BUILD_DIR := $(BUILD_DIR)/c3
ESP8266_BUILD_DIR := $(BUILD_DIR)/8266
ESP8266_SKETCH_DIR := $(ESP8266_BUILD_DIR)/sketch/esp8266-arduino
PICO_SKETCH_DIR := $(PICO_BUILD_DIR)/sketch/$(PROJECT_NAME)
S2_SKETCH_DIR := $(S2_BUILD_DIR)/sketch/$(PROJECT_NAME)
S3_SKETCH_DIR := $(S3_BUILD_DIR)/sketch/$(PROJECT_NAME)
C3_SKETCH_DIR := $(C3_BUILD_DIR)/sketch/$(PROJECT_NAME)
PICO_8M_BUILD_DIR := $(BUILD_DIR)/pico-8m
PICO_8M_SKETCH_DIR := $(PICO_8M_BUILD_DIR)/sketch/$(PROJECT_NAME)
PICO_8M_OUTPUT_DIR := $(PICO_8M_BUILD_DIR)/output

ARDUINO_DATA_DIR := $(HOME)/.arduino15-esp32-2
ARDUINO_CONFIG := $(HOME)/.arduino15/arduino-cli-esp32-2.yaml
ARDUINO_ROOT := $(ARDUINO_DATA_DIR)/packages/esp32
ESP8266_ROOT := $(ARDUINO_DATA_DIR)/packages/esp8266
ARDUINO_CLI := arduino-cli --config-file $(ARDUINO_CONFIG)
CORE_VERSION := 2.0.11
CORE_DIR := $(ARDUINO_ROOT)/hardware/esp32/$(CORE_VERSION)
DNS_LIBRARY := $(BUILD_DIR)/dns-library
DNS_LIBRARY_STAMP := $(DNS_LIBRARY)/.ready
DNS_LIBRARY_ARG := --library "$(DNS_LIBRARY)"
ESP8266_CORE_VERSION ?= 3.1.2
ESP8266_CORE_DIR := $(ESP8266_ROOT)/hardware/esp8266/$(ESP8266_CORE_VERSION)
ESPTOOL := $(ARDUINO_ROOT)/tools/esptool_py/4.5.1/esptool.py
ESP8266_ESPTOOL := $(ESPTOOL)
ESP8266_MKLITTLEFS := $(ESP8266_ROOT)/tools/mklittlefs/3.1.0-gcc10.3-e5f9fec/mklittlefs
PARTITION_TOOL := $(CORE_DIR)/tools/gen_esp32part.py
LITTLEFS_TOOL := $(VENDOR_DIR)/mklittlefs/mklittlefs

define stage-esp32-sketch
	rm -rf "$(1)"
	mkdir -p "$(1)"
	cp "$(ESP32_DIR)"/*.ino "$(ESP32_DIR)"/*.h "$(ESP32_DIR)"/*.cpp "$(PORTAL_DIR)"/*.h "$(PORTAL_DIR)"/*.cpp "$(SERVER_HEADER)" "$(1)/"
	cp "$(2)" "$(1)/partitions.csv"
endef

define stage-esp8266-sketch
	rm -rf "$(1)" "$(ESP8266_BUILD_DIR)/output"
	mkdir -p "$(1)" "$(ESP8266_BUILD_DIR)/output"
	cp "$(ESP8266_DIR)"/*.ino "$(ESP8266_DIR)"/*.h "$(ESP8266_DIR)"/*.cpp "$(PORTAL_DIR)"/*.h "$(PORTAL_DIR)"/*.cpp "$(SERVER_HEADER)" "$(1)/"
endef

define pad-merged
	size=$$(stat -c %s "$(1)")
	test "$$size" -le $(2) || { echo "Error: $(3) exceeds $(4): $$size bytes" >&2; exit 1; }
	if [ "$$size" -lt $(2) ]; then
	    padding=$$(($(2) - size))
	    head -c "$$padding" /dev/zero | tr '\000' '\377' >> "$(1)"
	fi
	test "$$(stat -c %s "$(1)")" -eq $(2)
endef

define merge-esp32-image
	python3 "$(ESPTOOL)" \
	    --chip $(1) \
	    merge_bin \
	    -o "$(2)" \
	    --flash_mode dio \
	    --flash_freq $(3) \
	    --flash_size $(4) \
	    $(5) "$(6)" \
	    0x8000 "$(7)" \
	    0x10000 "$(8)" \
	    "$(9)" "$(10)"
	$(call pad-merged,$(2),$(11),$(12),$(4))
endef

USB_DEBUG ?= 0
ifeq ($(USB_DEBUG),1)
DEBUG_LEVEL := verbose
S2_USB_OPTIONS := CDCOnBoot=cdc
S3_USB_OPTIONS := USBMode=hwcdc,CDCOnBoot=cdc
C3_USB_OPTIONS := CDCOnBoot=cdc
else
DEBUG_LEVEL := none
S2_USB_OPTIONS := CDCOnBoot=default
S3_USB_OPTIONS := USBMode=default,CDCOnBoot=default
C3_USB_OPTIONS := CDCOnBoot=default
endif
USB_DEBUG_FLAG := -DUSB_DEBUG=$(USB_DEBUG)
ifeq ($(USB_DEBUG),1)
S2_TLS_DEBUG_LINK := --build-property "compiler.c.elf.extra_flags=-Wl,--wrap=esp_tls_server_session_create"
endif
PICO_FQBN := esp32:esp32:esp32:CPUFreq=240,FlashFreq=40,FlashMode=dio,FlashSize=4M,DebugLevel=$(DEBUG_LEVEL),PSRAM=disabled
S2_FQBN := esp32:esp32:esp32s2:$(S2_USB_OPTIONS),MSCOnBoot=default,DFUOnBoot=default,UploadMode=default,CPUFreq=240,FlashFreq=40,FlashMode=dio,FlashSize=4M,DebugLevel=$(DEBUG_LEVEL),PSRAM=disabled
S3_FQBN := esp32:esp32:esp32s3:$(S3_USB_OPTIONS),MSCOnBoot=default,DFUOnBoot=default,UploadMode=default,CPUFreq=240,FlashMode=dio,FlashSize=4M,DebugLevel=$(DEBUG_LEVEL),PSRAM=disabled
PICO_8M_FQBN := esp32:esp32:esp32:CPUFreq=240,FlashFreq=40,FlashMode=dio,FlashSize=8M,DebugLevel=$(DEBUG_LEVEL),PSRAM=disabled
C3_FQBN := esp32:esp32:esp32c3:$(C3_USB_OPTIONS),CPUFreq=160,FlashFreq=40,FlashMode=dio,FlashSize=4M,DebugLevel=$(DEBUG_LEVEL)
ESP8266_FQBN := esp8266:esp8266:generic:eesz=4M3M,FlashMode=dout,FlashFreq=40,xtal=80,CrystalFreq=26,baud=921600,ssl=all,mmu=3232,non32xfer=fast,vt=flash,exception=disabled,stacksmash=disabled,ip=lm2f,sdk=nonosdk_190703,lvl=None____,dbg=Disabled,wipe=none,led=2

FLASH_SIZE := 4194304
APP_PARTITION_SIZE := 1048576
LITTLEFS_SIZE := 0x2F0000
LITTLEFS_OFFSET := 0x110000
PICO_8M_FLASH_SIZE := 8388608
PICO_8M_APP_PARTITION_SIZE := 1572864
PICO_8M_LITTLEFS_SIZE := 0x660000
PICO_8M_LITTLEFS_OFFSET := 0x190000

LITTLEFS_IMAGE := $(COMMON_BUILD_DIR)/$(PROJECT_NAME).littlefs.bin
PICO_APP := $(PICO_BUILD_DIR)/$(PROJECT_NAME).ino.bin
PICO_BOOTLOADER := $(PICO_BUILD_DIR)/$(PROJECT_NAME).ino.bootloader.bin
PICO_PARTITIONS := $(PICO_BUILD_DIR)/$(PROJECT_NAME).ino.partitions.bin
PICO_MERGED := $(PICO_BUILD_DIR)/$(PROJECT_NAME).pico.merged.bin
S2_APP := $(S2_BUILD_DIR)/$(PROJECT_NAME).ino.bin
S2_BOOTLOADER := $(S2_BUILD_DIR)/$(PROJECT_NAME).ino.bootloader.bin
S2_PARTITIONS := $(S2_BUILD_DIR)/$(PROJECT_NAME).ino.partitions.bin
S2_MERGED := $(S2_BUILD_DIR)/$(PROJECT_NAME).s2.merged.bin
S3_APP := $(S3_BUILD_DIR)/$(PROJECT_NAME).ino.bin
S3_BOOTLOADER := $(S3_BUILD_DIR)/$(PROJECT_NAME).ino.bootloader.bin
S3_PARTITIONS := $(S3_BUILD_DIR)/$(PROJECT_NAME).ino.partitions.bin
S3_MERGED := $(S3_BUILD_DIR)/$(PROJECT_NAME).s3.merged.bin
PICO_8M_LITTLEFS_IMAGE := $(PICO_8M_BUILD_DIR)/$(PROJECT_NAME).littlefs.bin
PICO_8M_APP := $(PICO_8M_OUTPUT_DIR)/$(PROJECT_NAME).ino.bin
PICO_8M_BOOTLOADER := $(PICO_8M_OUTPUT_DIR)/$(PROJECT_NAME).ino.bootloader.bin
PICO_8M_PARTITIONS := $(PICO_8M_OUTPUT_DIR)/$(PROJECT_NAME).ino.partitions.bin
PICO_8M_MERGED := $(PICO_8M_BUILD_DIR)/$(PROJECT_NAME).pico-8m.merged.bin
C3_APP := $(C3_BUILD_DIR)/$(PROJECT_NAME).ino.bin
C3_BOOTLOADER := $(C3_BUILD_DIR)/$(PROJECT_NAME).ino.bootloader.bin
C3_PARTITIONS := $(C3_BUILD_DIR)/$(PROJECT_NAME).ino.partitions.bin
C3_MERGED := $(C3_BUILD_DIR)/$(PROJECT_NAME).c3.merged.bin
ESP8266_APP := $(ESP8266_BUILD_DIR)/output/esp8266-arduino.ino.bin
ESP8266_LITTLEFS_IMAGE := $(ESP8266_BUILD_DIR)/esp8266-arduino.littlefs.bin
ESP8266_MERGED := $(ESP8266_BUILD_DIR)/esp8266-arduino.esp12f.merged.bin
ESP8266_FLASH_SIZE := 4194304
ESP8266_LITTLEFS_SIZE := 0x2FA000
ESP8266_LITTLEFS_OFFSET := 0x100000

.PHONY: all common debug debug-s2 pico pico-8m s2 s3 c3 8266 littlefs datadir minimize check check-minimize check-data check-littlefs check-8266 clean FORCE

all: check $(PICO_MERGED) $(S2_MERGED) $(S3_MERGED) $(C3_MERGED)
	@echo
	echo "Build complete:"
	ls -lh "$(PICO_MERGED)" "$(S2_MERGED)" $(S3_MERGED) $(C3_MERGED) "$(LITTLEFS_IMAGE)"

common: check $(SERVER_HEADER) $(DNS_LIBRARY_STAMP) $(LITTLEFS_IMAGE)
	@ls -lh "$(SERVER_HEADER)" "$(LITTLEFS_IMAGE)"

debug:
	@$(MAKE) USB_DEBUG=1 BUILD_DIR="$(PROJECT_DIR)/build/debug" all

debug-s2:
	@$(MAKE) USB_DEBUG=1 BUILD_DIR="$(PROJECT_DIR)/build/debug" s2

pico: check $(PICO_MERGED)
	@ls -lh "$(PICO_MERGED)"

pico-8m: check $(PICO_8M_MERGED)
	@ls -lh "$(PICO_8M_MERGED)" "$(PICO_8M_LITTLEFS_IMAGE)"

s2: check $(S2_MERGED)
	@ls -lh "$(S2_MERGED)"

s3: check $(S3_MERGED)
	@ls -lh "$(S3_MERGED)"

c3: check $(C3_MERGED)
	@ls -lh "$(C3_MERGED)"

8266: check-8266 $(ESP8266_MERGED)
	@ls -lh "$(ESP8266_MERGED)" "$(ESP8266_LITTLEFS_IMAGE)"

littlefs: check-littlefs $(LITTLEFS_IMAGE)
	@ls -lh "$(LITTLEFS_IMAGE)"

minimize: check-minimize FORCE
	@"$(DATA_PREPARER)" minimize "$(SOURCE_DATA_DIR)" "$(MINIFIED_DATA_DIR)"
	du -sh "$(MINIFIED_DATA_DIR)"

datadir: check-data minimize FORCE
	@"$(DATA_PREPARER)" datadir "$(MINIFIED_DATA_DIR)" "$(DATA_DIR)"
	du -sh "$(DATA_DIR)"

check-minimize:
	@missing=0
	for tool in terser cleancss html-minifier-terser jq svgo; do
		if ! command -v "$$tool" >/dev/null 2>&1; then
			echo "Error: $$tool is required (run ./install-deps.sh)" >&2
			missing=1
		fi
	done
	test "$$missing" -eq 0
	test -x "$(DATA_PREPARER)" || { echo "Error: $(DATA_PREPARER) is not executable" >&2; exit 1; }
	test -d "$(SOURCE_DATA_DIR)" || { echo "Error: $(SOURCE_DATA_DIR) was not found" >&2; exit 1; }

check-data:
	@command -v gzip >/dev/null 2>&1 || { echo "Error: gzip is required (run ./install-deps.sh)" >&2; exit 1; }
	test -x "$(DATA_PREPARER)" || { echo "Error: $(DATA_PREPARER) is not executable" >&2; exit 1; }

check-littlefs: check-data
	@test -x "$(LITTLEFS_TOOL)" || { echo "Error: mklittlefs was not found" >&2; exit 1; }

check: check-littlefs
	@command -v $(ARDUINO_CLI) >/dev/null
	command -v openssl >/dev/null
	command -v python3 >/dev/null
	test -d "$(CORE_DIR)" || { echo "Error: esp32:esp32@$(CORE_VERSION) is not installed" >&2; exit 1; }
	test -f "$(ESPTOOL)" || { echo "Error: $(ESPTOOL) was not found" >&2; exit 1; }
	test -f "$(PARTITION_TOOL)" || { echo "Error: $(PARTITION_TOOL) was not found" >&2; exit 1; }
	test -f "$(ESP32_SKETCH)"

check-8266: check-data
	@command -v $(ARDUINO_CLI) >/dev/null
	command -v python3 >/dev/null
	test -d "$(ESP8266_CORE_DIR)" || { echo "Error: esp8266:esp8266@$(ESP8266_CORE_VERSION) is not installed" >&2; exit 1; }
	test -f "$(ESP8266_ESPTOOL)" || { echo "Error: ESP8266 esptool was not found" >&2; exit 1; }
	test -x "$(ESP8266_MKLITTLEFS)" || { echo "Error: ESP8266 mklittlefs was not found" >&2; exit 1; }
	test -f "$(ESP8266_SKETCH)"

$(SERVER_CERT) $(SERVER_KEY) &:
	@echo "Generating a self-signed HTTPS certificate..."
	mkdir -p "$(dir $(SERVER_CERT))"
	openssl ecparam -name prime256v1 \
		-genkey \
		-noout \
		-out "$(SERVER_KEY)"
	openssl req -new -x509 \
		-sha256 \
		-days 3650 \
		-key "$(SERVER_KEY)" \
		-out "$(SERVER_CERT)" \
		-subj "/CN=manuals.playstation.net" \
		-addext "subjectAltName=DNS:manuals.playstation.net,IP:192.168.4.1"
	chmod 600 "$(SERVER_KEY)"

$(SERVER_HEADER): $(SERVER_CERT) $(SERVER_KEY) $(CERT_EMBEDDER)
	@python3 "$(CERT_EMBEDDER)" "$(SERVER_CERT)" "$(SERVER_KEY)" "$(SERVER_HEADER)"

$(DNS_LIBRARY_STAMP): $(CORE_DIR)/libraries/DNSServer/src/DNSServer.cpp $(CORE_DIR)/libraries/DNSServer/src/DNSServer.h $(DNS_PREPARER) Makefile
	@python3 "$(DNS_PREPARER)" "$(CORE_DIR)/libraries/DNSServer" "$(DNS_LIBRARY)"

$(PICO_APP): $(ESP32_SKETCH_SOURCES) $(SERVER_HEADER) $(ESP32_PARTITIONS) Makefile $(DNS_LIBRARY_STAMP)
	@$(call stage-esp32-sketch,$(PICO_SKETCH_DIR),$(ESP32_PARTITIONS))
	$(ARDUINO_CLI) compile \
	    $(DNS_LIBRARY_ARG) \
	    --fqbn "$(PICO_FQBN)" \
	    --output-dir "$(PICO_BUILD_DIR)" \
	    --build-property "build.partitions=partitions" \
	    --build-property "build.filesystem=littlefs" \
	    "$(PICO_SKETCH_DIR)"
	size=$$(stat -c %s "$(PICO_APP)")
	test "$$size" -le "$(APP_PARTITION_SIZE)" || { echo "Error: Pico application exceeds the 1 MB partition: $$size bytes" >&2; exit 1; }

$(S2_APP): $(ESP32_SKETCH_SOURCES) $(SERVER_HEADER) $(ESP32_PARTITIONS) Makefile $(DNS_LIBRARY_STAMP)
	@$(call stage-esp32-sketch,$(S2_SKETCH_DIR),$(ESP32_PARTITIONS))
	$(ARDUINO_CLI) compile \
	    $(DNS_LIBRARY_ARG) \
	    --fqbn "$(S2_FQBN)" \
	    --build-property "compiler.cpp.extra_flags=$(USB_DEBUG_FLAG)" \
	    $(S2_TLS_DEBUG_LINK) \
	    --output-dir "$(S2_BUILD_DIR)" \
	    --build-property "build.partitions=partitions" \
	    --build-property "build.filesystem=littlefs" \
	    "$(S2_SKETCH_DIR)"
	size=$$(stat -c %s "$(S2_APP)")
	test "$$size" -le "$(APP_PARTITION_SIZE)" || { echo "Error: S2 application exceeds the 1 MB partition: $$size bytes" >&2; exit 1; }

$(S3_APP): $(ESP32_SKETCH_SOURCES) $(SERVER_HEADER) $(ESP32_PARTITIONS) Makefile $(DNS_LIBRARY_STAMP)
	@$(call stage-esp32-sketch,$(S3_SKETCH_DIR),$(ESP32_PARTITIONS))
	$(ARDUINO_CLI) compile \
	    $(DNS_LIBRARY_ARG) \
	    --fqbn "$(S3_FQBN)" \
	    --build-property "compiler.cpp.extra_flags=$(USB_DEBUG_FLAG)" \
	    --output-dir "$(S3_BUILD_DIR)" \
	    --build-property "build.partitions=partitions" \
	    --build-property "build.filesystem=littlefs" \
	    "$(S3_SKETCH_DIR)"
	size=$$(stat -c %s "$(S3_APP)")
	test "$$size" -le "$(APP_PARTITION_SIZE)" || { echo "Error: S3 application exceeds the 1 MB partition: $$size bytes" >&2; exit 1; }

$(PICO_8M_APP): $(ESP32_SKETCH_SOURCES) $(SERVER_HEADER) $(ESP32_PARTITIONS_8M) Makefile $(DNS_LIBRARY_STAMP)
	@rm -rf "$(PICO_8M_OUTPUT_DIR)"
	$(call stage-esp32-sketch,$(PICO_8M_SKETCH_DIR),$(ESP32_PARTITIONS_8M))
	mkdir -p "$(PICO_8M_OUTPUT_DIR)"
	$(ARDUINO_CLI) compile \
	    $(DNS_LIBRARY_ARG) \
	    --fqbn "$(PICO_8M_FQBN)" \
	    --output-dir "$(PICO_8M_OUTPUT_DIR)" \
	    --build-property "build.partitions=partitions" \
	    --build-property "build.filesystem=littlefs" \
	    "$(PICO_8M_SKETCH_DIR)"
	size=$$(stat -c %s "$(PICO_8M_APP)")
	test "$$size" -le "$(PICO_8M_APP_PARTITION_SIZE)" || { echo "Error: Pico 8 MB application exceeds the 1.5 MB partition: $$size bytes" >&2; exit 1; }

$(C3_APP): $(ESP32_SKETCH_SOURCES) $(SERVER_HEADER) $(ESP32_PARTITIONS) Makefile $(DNS_LIBRARY_STAMP)
	@$(call stage-esp32-sketch,$(C3_SKETCH_DIR),$(ESP32_PARTITIONS))
	$(ARDUINO_CLI) compile \
	    $(DNS_LIBRARY_ARG) \
	    --fqbn "$(C3_FQBN)" \
	    --build-property "compiler.cpp.extra_flags=$(USB_DEBUG_FLAG)" \
	    --output-dir "$(C3_BUILD_DIR)" \
	    --build-property "build.partitions=partitions" \
	    --build-property "build.filesystem=littlefs" \
	    "$(C3_SKETCH_DIR)"
	size=$$(stat -c %s "$(C3_APP)")
	test "$$size" -le "$(APP_PARTITION_SIZE)" || { echo "Error: C3 application exceeds the 1 MB partition: $$size bytes" >&2; exit 1; }

$(ESP8266_APP): $(ESP8266_SKETCH_SOURCES) $(SERVER_HEADER) Makefile
	@$(call stage-esp8266-sketch,$(ESP8266_SKETCH_DIR))
	$(ARDUINO_CLI) compile --fqbn "$(ESP8266_FQBN)" --output-dir "$(ESP8266_BUILD_DIR)/output" "$(ESP8266_SKETCH_DIR)"
	size=$$(stat -c %s "$(ESP8266_APP)")
	test "$$size" -le 1044464 || { echo "Error: ESP8266 application exceeds the 4M3M sketch area: $$size bytes" >&2; exit 1; }

$(ESP8266_LITTLEFS_IMAGE): datadir
	@mkdir -p "$(ESP8266_BUILD_DIR)"
	"$(ESP8266_MKLITTLEFS)" -c "$(DATA_DIR)" -p 256 -b 8192 -s "$(ESP8266_LITTLEFS_SIZE)" "$(ESP8266_LITTLEFS_IMAGE)"

$(ESP8266_MERGED): $(ESP8266_APP) $(ESP8266_LITTLEFS_IMAGE)
	@python3 "$(ESP8266_ESPTOOL)" --chip esp8266 merge_bin -o "$(ESP8266_MERGED)" --flash_mode dout --flash_freq 40m --flash_size 4MB 0x0000 "$(ESP8266_APP)" "$(ESP8266_LITTLEFS_OFFSET)" "$(ESP8266_LITTLEFS_IMAGE)"
	$(call pad-merged,$(ESP8266_MERGED),$(ESP8266_FLASH_SIZE),ESP8266 image,4 MB)

ifeq ($(REUSE_FS),1)
$(LITTLEFS_IMAGE):
	@test -f "$(LITTLEFS_IMAGE)" || { echo "Error: $(LITTLEFS_IMAGE) is missing" >&2; exit 1; }
else
$(LITTLEFS_IMAGE): datadir
	@echo
	echo "Creating shared LittleFS image..."
	mkdir -p "$(COMMON_BUILD_DIR)"
	"$(LITTLEFS_TOOL)" \
	    -c "$(DATA_DIR)" \
	    -p 256 \
	    -b 4096 \
	    -s "$(LITTLEFS_SIZE)" \
	    "$(LITTLEFS_IMAGE)"
endif

$(PICO_MERGED): $(PICO_APP) $(LITTLEFS_IMAGE)
	@$(call merge-esp32-image,esp32,$(PICO_MERGED),40m,4MB,0x1000,$(PICO_BOOTLOADER),$(PICO_PARTITIONS),$(PICO_APP),$(LITTLEFS_OFFSET),$(LITTLEFS_IMAGE),$(FLASH_SIZE),Pico image)

$(S2_MERGED): $(S2_APP) $(LITTLEFS_IMAGE)
	@$(call merge-esp32-image,esp32s2,$(S2_MERGED),40m,4MB,0x1000,$(S2_BOOTLOADER),$(S2_PARTITIONS),$(S2_APP),$(LITTLEFS_OFFSET),$(LITTLEFS_IMAGE),$(FLASH_SIZE),S2 image)

$(S3_MERGED): $(S3_APP) $(LITTLEFS_IMAGE)
	@$(call merge-esp32-image,esp32s3,$(S3_MERGED),80m,4MB,0x0,$(S3_BOOTLOADER),$(S3_PARTITIONS),$(S3_APP),$(LITTLEFS_OFFSET),$(LITTLEFS_IMAGE),$(FLASH_SIZE),S3 image)

$(C3_MERGED): $(C3_APP) $(LITTLEFS_IMAGE)
	@$(call merge-esp32-image,esp32c3,$(C3_MERGED),40m,4MB,0x0,$(C3_BOOTLOADER),$(C3_PARTITIONS),$(C3_APP),$(LITTLEFS_OFFSET),$(LITTLEFS_IMAGE),$(FLASH_SIZE),C3 image)

$(PICO_8M_LITTLEFS_IMAGE): datadir
	@echo
	echo "Creating Pico 8 MB LittleFS image..."
	mkdir -p "$(PICO_8M_BUILD_DIR)"
	"$(LITTLEFS_TOOL)" \
	    -c "$(DATA_DIR)" \
	    -p 256 \
	    -b 4096 \
	    -s "$(PICO_8M_LITTLEFS_SIZE)" \
	    "$(PICO_8M_LITTLEFS_IMAGE)"

$(PICO_8M_MERGED): $(PICO_8M_APP) $(PICO_8M_LITTLEFS_IMAGE)
	@$(call merge-esp32-image,esp32,$(PICO_8M_MERGED),40m,8MB,0x1000,$(PICO_8M_BOOTLOADER),$(PICO_8M_PARTITIONS),$(PICO_8M_APP),$(PICO_8M_LITTLEFS_OFFSET),$(PICO_8M_LITTLEFS_IMAGE),$(PICO_8M_FLASH_SIZE),Pico image)

clean:
	@rm -rf "$(BUILD_DIR)"

FORCE:
