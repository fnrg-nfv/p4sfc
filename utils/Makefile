BUILD_DIR = build
PCAP_DIR = pcaps
LOG_DIR = logs

ifndef PROG_PREFIX
PROG_PREFIX = test
endif

ifndef RUN_SCRIPT
RUN_SCRIPT = 1sw_demo.py
endif

P4C = p4c
P4C_ARGS = --target bmv2 --arch v1model --p4runtime-files $(BUILD_DIR)/$(PROG_PREFIX).txt

BMV2_SWITCH_EXE = simple_switch_grpc

RUN_ARGS = --behavioral-exe $(BMV2_SWITCH_EXE) --pcap-dump $(PCAP_DIR) -l $(LOG_DIR) --json $(BUILD_DIR)/$(PROG_PREFIX).json

all: run

run: build
	sudo python $(RUN_SCRIPT) $(RUN_ARGS)

stop:
	sudo mn -c

build: dirs 
	$(P4C) $(P4C_ARGS) $(PROG_PREFIX).p4 -o $(BUILD_DIR)

dirs:
	mkdir -p $(BUILD_DIR) $(PCAP_DIR) $(LOG_DIR)

clean: stop
	rm -f ./*.pcap
	rm -rf $(BUILD_DIR) $(PCAP_DIR) $(LOG_DIR)

