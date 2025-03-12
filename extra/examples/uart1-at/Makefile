BIN     = ../../riscv-embedded-gcc/bin
PREFIX  = riscv-wch-elf-
CC      = $(BIN)/$(PREFIX)gcc
ISP 		= ../../isp/bin/x64/WCHISPTool_CMD

CFLAGS  = -march=rv32imc_zba_zbb_zbc_zbs_xw -mabi=ilp32 -mcmodel=medany -msmall-data-limit=8 -mno-save-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common --param=highcode-gen-section-name=1 -g
DEFINES =

OUTPUT_DIR = ./dist

TARGET     := $(addprefix $(OUTPUT_DIR)/, $(notdir $(CURDIR)))

Project_ROOT    = .
Project_INC		= $(Project_ROOT)/inc
Project_SRC_DIR = $(Project_ROOT)
Project_SOURCE  = $(notdir $(wildcard $(Project_SRC_DIR)/*.c))
Project_OBJECTS := $(addprefix $(OUTPUT_DIR)/, $(Project_SOURCE:.c=.o))

StdPeriphDriver_ROOT    = ../../EVT/EXAM/SRC/StdPeriphDriver
StdPeriphDriver_INC_DIR = $(StdPeriphDriver_ROOT)/inc
StdPeriphDriver_SRC_DIR = $(StdPeriphDriver_ROOT)
StdPeriphDriver_SOURCE  = CH58x_sys.c CH58x_clk.c CH58x_gpio.c CH58x_uart1.c CH58x_spi0.c CH58x_i2c.c
StdPeriphDriver_OBJECTS := $(addprefix $(OUTPUT_DIR)/, $(StdPeriphDriver_SOURCE:.c=.o))

RVMSIS_ROOT		= ../../EVT/EXAM/SRC/RVMSIS
# RVMSIS_SOURCE = core_riscv.c
# RVMSIS_OBJECTS := $(addprefix $(OUTPUT_DIR)/, $(RVMSIS_SOURCE:.c=.o))

Startup_ROOT		= ../../EVT/EXAM/SRC/Startup
Startup_SOURCE = startup_CH585.S
Startup_OBJECTS := $(addprefix $(OUTPUT_DIR)/asm/, $(Startup_SOURCE:.S=.o))

LINK_FILE = ../../EVT/EXAM/SRC/Ld/Link.ld

INCLUDE = -I $(StdPeriphDriver_INC_DIR) -I $(RVMSIS_ROOT) -I $(Project_INC)

VPATH=$(StdPeriphDriver_SRC_DIR):$(RVMSIS_ROOT):$(Startup_ROOT)

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
	$(CC) $(CFLAGS) $(INCLUDE) $(DEFINES) -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

$(TARGET).elf: $(StdPeriphDriver_OBJECTS) $(Project_OBJECTS) $(Startup_OBJECTS)
	$(CC) $(CFLAGS) -T "$(LINK_FILE)" -nostartfiles -Xlinker --gc-sections -L"../" -L"$(StdPeriphDriver_ROOT)" -Xlinker --print-memory-usage -Wl,-Map,"$(TARGET).map" --specs=nano.specs --specs=nosys.specs -o "$(TARGET).elf" $^ -lISP585

$(TARGET).hex: $(TARGET).elf
	$(BIN)/$(PREFIX)objcopy -O ihex "$<" "$@"

flash: $(TARGET).hex
	sudo $(ISP) -p /dev/ch37x0 -b 115200 -c ../../isp/ch585.ini -o program -f $<

.PHONY: version clean
