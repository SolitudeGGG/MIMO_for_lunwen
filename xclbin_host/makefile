############################## Help Section ##############################
.PHONY: help

help::
	$(ECHO) "Makefile Usage:"
	$(ECHO) "	make clean
	$(ECHO) "		Command to remove the generated files."
	$(ECHO) ""
	$(ECHO) "	make all
	$(ECHO) "		Command to generate hardware accelerator(.xclbin)."
	$(ECHO) ""

# ####################### Setting file directory #######################################
MK_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
CUR_DIR := $(patsubst %/,%,$(dir $(MK_PATH)))
SRCDIR := $(CUR_DIR)/src
BUILD_DIR := $(CUR_DIR)/build
BUILD_REPORT_DIR := $(BUILD_DIR)/reports/build
TEMP_DIR := $(BUILD_DIR)/temp
TEMP_REPORT_DIR := $(BUILD_DIR)/reports/temp
LOG_DIR=$(BUILD_DIR)/logs

# ####################### Setting objects and source files #############################
BUILD_XCLBIN := $(BUILD_DIR)/kernel.xclbin
BUILD_XOBJECT += $(TEMP_DIR)/krnl_MHGD.xo
BUILD_SOURCE += $(SRCDIR)/MHGD_accel_hw.cpp
BUILD_SOURCE += $(SRCDIR)/MHGD_accel_hw.h
BUILD_SOURCE += $(SRCDIR)/MyComplex_1.h
# ####################### Setting compile environment ##################################
VPP ?= ${XILINX_VITIS}/bin/v++
TARGET ?= hw	# can be configured with sw_emu or hw_emu
PLATFORM ?= xilinx_u50_gen3x16_xdma_5_202210_1
KERNEL_NAME = MHGD_detect_accel_hw
CLOCK_FREQ_MHZ = 100000000

# ####################### Setting compile and link flags ##################################
VPP_FLAGS += -I $(SRCDIR)
VPP_FLAGS += -t $(TARGET) --config MHGD_compile.cfg 
VPP_FLAGS += --platform $(PLATFORM)
VPP_FLAGS += --hls.clock $(CLOCK_FREQ_MHZ):$(KERNEL_NAME)
VPP_FLAGS += --hls.pre_tcl $(SRCDIR)/hls_config.tcl

############################## Setting Makefile Targets ##############################
all: $(BUILD_XCLBIN)
	@echo "GENERATE $(BUILD_XCLBIN) Done!"

# ################ Setting Rules for Binary Containers (Building Kernels) ###############
#  ###### Linking Kernel ######
$(BUILD_XCLBIN): $(BUILD_XOBJECT)
	@echo  "Linking Kernel: $(VPP) -l $(VPP_FLAGS) $(BUILD_XOBJECT) -o $@ $^"
	mkdir -p $(BUILD_DIR)
	$(VPP) -l $(VPP_FLAGS) $(BUILD_XOBJECT) --temp_dir $(TEMP_DIR) --report_dir $(BUILD_REPORT_DIR)/$(KERNEL_NAME) -o $@ $^

#  ###### Compile Kernel ######
$(BUILD_XOBJECT): $(BUILD_SOURCE)
	@echo  "Compiling Kernel: $(VPP) -c $(VPP_FLAGS) $(BUILD_SOURCE) -k $(KERNEL_NAME) -o $@ $^"
	mkdir -p $(TEMP_DIR)
	$(VPP) -c $(VPP_FLAGS) $(BUILD_SOURCE) -k $(KERNEL_NAME) --temp_dir $(TEMP_DIR) --report_dir $(TEMP_REPORT_DIR) -o $@ $^

clean:
	rm -rf *.json .run .Xil .ipcache *.jou *.log $(TEMP_DIR) $(TEMP_REPORT_DIR) $(BUILD_DIR) $(BUILD_REPORT_DIR)