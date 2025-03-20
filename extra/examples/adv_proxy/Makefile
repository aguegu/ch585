BIN     = ../../riscv-embedded-gcc/bin
PREFIX  = riscv-wch-elf
CC      = $(BIN)/$(PREFIX)-gcc
ISP 		= ../../isp/bin/x64/WCHISPTool_CMD
LINTER = clang-format --style=LLVM -i

OUTPUT_DIR = ./dist

CFLAGS  = -march=rv32imc_zba_zbb_zbc_zbs_xw -mabi=ilp32 -mcmodel=medany -msmall-data-limit=8 -mno-save-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common --param=highcode-gen-section-name=1 -g
DEFINES = -DDEBUG=0 -DBLE_MEMHEAP_SIZE=4096 -DHAL_KEY=1

TARGET     := $(addprefix $(OUTPUT_DIR)/, $(notdir $(CURDIR)))

APP_ROOT    = ./APP
APP_INC_DIR = $(APP_ROOT)/include
APP_SRC_DIR = $(APP_ROOT)
APP_SOURCE  = $(notdir $(wildcard $(APP_SRC_DIR)/*.c))
APP_OBJECTS = $(addprefix $(OUTPUT_DIR)/, $(APP_SOURCE:.c=.o))
APP_HEADERS = $(addprefix $(APP_INC_DIR)/, $(notdir $(wildcard $(APP_INC_DIR)/*.h)))

StdPeriphDriver_ROOT    = ../../EVT/EXAM/SRC/StdPeriphDriver
StdPeriphDriver_INC_DIR = $(StdPeriphDriver_ROOT)/inc
StdPeriphDriver_SRC_DIR = $(StdPeriphDriver_ROOT)
StdPeriphDriver_SOURCE  = CH58x_sys.c CH58x_clk.c CH58x_gpio.c CH58x_uart0.c CH58x_adc.c
StdPeriphDriver_OBJECTS = $(addprefix $(OUTPUT_DIR)/, $(StdPeriphDriver_SOURCE:.c=.o))

HAL_ROOT    = ../../EVT/EXAM/BLE/HAL
HAL_INC_DIR = $(HAL_ROOT)/include
HAL_SOURCE  = MCU.c RTC.c KEY.c
HAL_OBJECTS = $(addprefix $(OUTPUT_DIR)/, $(HAL_SOURCE:.c=.o))

RVMSIS_ROOT = ../../EVT/EXAM/SRC/RVMSIS

Startup_ROOT    = ../../EVT/EXAM/SRC/Startup
Startup_SOURCE  = startup_CH585.S
Startup_OBJECTS = $(addprefix $(OUTPUT_DIR)/asm/, $(Startup_SOURCE:.S=.o))

BleLib_ROOT    = ../../EVT/EXAM/BLE/LIB
BleLib_SOURCE  = ble_task_scheduler.S
BleLib_OBJECTS = $(addprefix $(OUTPUT_DIR)/asm/, $(BleLib_SOURCE:.S=.o))

BleMesh_ROOT   = ../../EVT/EXAM/BLE/MESH/MESH_LIB

LINK_FILE = ../../EVT/EXAM/SRC/Ld/Link.ld

INCLUDES = -I $(StdPeriphDriver_INC_DIR) -I $(RVMSIS_ROOT) -I $(APP_INC_DIR) -I $(HAL_INC_DIR) \
	-I $(BleLib_ROOT) \
	-I $(BleMesh_ROOT)

VPATH = $(StdPeriphDriver_SRC_DIR):$(RVMSIS_ROOT):$(Startup_ROOT):$(APP_ROOT):$(HAL_ROOT):$(BleLib_ROOT)

LIBS := -lISP585 -lCH58xBLE -lMESH

all: $(OUTPUT_DIR) $(TARGET).hex

$(OUTPUT_DIR):
	mkdir -p $(OUTPUT_DIR)/asm

version:
	$(CC) --version

clean:
	rm -rf $(OUTPUT_DIR)
	mkdir -p $(OUTPUT_DIR)/asm

$(OUTPUT_DIR)/asm/%.o: %.S
	$(CC) $(CFLAGS) -MMD -MP -x assembler-with-cpp -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

$(OUTPUT_DIR)/%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) $(DEFINES) -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

$(TARGET).elf: $(StdPeriphDriver_OBJECTS) $(APP_OBJECTS) $(Startup_OBJECTS) $(HAL_OBJECTS) $(BleLib_OBJECTS)
	$(CC) $(CFLAGS) -T "$(LINK_FILE)" -nostartfiles -Xlinker --gc-sections \
	-L"$(StdPeriphDriver_ROOT)" \
	-L"$(BleLib_ROOT)" \
	-L"$(BleMesh_ROOT)" \
	-Xlinker --print-memory-usage -Wl,-Map,"$(TARGET).map" --specs=nano.specs --specs=nosys.specs -o "$(TARGET).elf" $^ $(LIBS)

$(TARGET).hex: $(TARGET).elf
	$(BIN)/$(PREFIX)-objcopy -O ihex "$<" "$@"

flash: $(TARGET).hex
	sudo $(ISP) -p /dev/ch37x0 -c ./config.ini -o program -f $<

lint: $(APP_HEADERS) $(APP_SOURCE)
	$(foreach f, $^, $(LINTER) $(f);)

.PHONY: version clean lint
