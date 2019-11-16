BUILD_DIR = build

# P4C = p4c-bm2-ss
P4C = p4c
P4C_ARGS += --target bmv2 --arch v1model --p4runtime-files $(BUILD_DIR)/$(PROG_PREFIX).txt
PROG_PREFIX = test

BMV2_SWITCH_EXE = simple_switch_grpc

RUN_SCRIPT = 1sw_demo.py

# source = $(wildcard *.p4)

run_args += -j $(DEFAULT_JSON)

all: run

run: build
	sudo python $(RUN_SCRIPT) --behavioral-exe $(BMV2_SWITCH_EXE) --json $(BUILD_DIR)/$(PROG_PREFIX).json

stop:
	sudo mn -c

build: dirs 
	$(P4C) $(P4C_ARGS) $(PROG_PREFIX).p4 -o $(BUILD_DIR)

dirs:
	mkdir -p $(BUILD_DIR)

clean: stop
	rm -rf $(BUILD_DIR)
