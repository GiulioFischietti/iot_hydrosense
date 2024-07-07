CONTIKI_PROJECT = rpl-server
all: $(CONTIKI_PROJECT)

# The BR is either native or embedded, and in the latter case must support SLIP

CONTIKI = ../../../..

PLATFORMS_EXCLUDE = z1

# Include RPL BR module
include $(CONTIKI)/Makefile.dir-variables
MODULES += $(CONTIKI_NG_SERVICES_DIR)/rpl-border-router



include $(CONTIKI)/Makefile.include




