CONTIKI_PROJECT = ml-model
# coap-example-observe-client is outdated but will be ported at a later point
all: $(CONTIKI_PROJECT)

CONTIKI=../../../..

# Include the CoAP implementation
include $(CONTIKI)/Makefile.dir-variables
CFLAGS += -I$(CONTIKI)/os/net/app-layer/coap
MODULES += $(CONTIKI_NG_APP_LAYER_DIR)/coap
MODULES += core/net/ip/uip-debug
MODULES += $(CONTIKI)/utilities

MODULES_REL += /home/iot_ubuntu_intel/.local/lib/python3.10/site-packages/emlearn
TARGET_LIBFILES += -lm
INC += /home/iot_ubuntu_intel/.local/lib/python3.10/site-packages/emlearn



include $(CONTIKI)/Makefile.include
