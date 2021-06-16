.DELETE_ON_ERROR:
.EXPORT_ALL_VARIABLES:
SRC_DIR   = src
BUILD_DIR = build
BIN_DIR   = bin
BIN       = sjp

# We specify globally applicable compiler options right here. They are exported
# to all recursively called Makefiles.
CC      = gcc
CCFLAGS = -Wall -Werror -Wpedantic -Wextra -Wwrite-strings -Warray-bounds \
	 	  -Weffc++ -fno-exceptions --std=c++20 -O0
LDFLAGS = -lm -lstdc++

DEBUG = no
ifeq ($(DEBUG), yes)
	CCFLAGS += -g -DNDEBUG
endif

.PHONY: all $(BIN) install test leak_test clean help

all: dirs $(BIN)

dirs: $(BUILD_DIR) $(BIN_DIR)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(BIN):
	ctags -R .
	cd $(SRC_DIR) && $(MAKE)

install: all
	cp $(BUILD_DIR)/$(BIN) $(BIN_DIR)

test: all
	./$(BUILD_DIR)/$(BIN)

leak-test: all
	valgrind -s --leak-check=full $(BUILD_DIR)/$(BIN)

# Since _all_ build artifacts are created in the build directory, we don't need
# to recursively call any subdirectory's Makefile for cleanup. We check whether
# the binary was installed in the base directory, because that might sometimes
# be useful.
clean:
	rm -f tags
	rm -rf $(BUILD_DIR)
	[[ '$(BIN_DIR)' != '.' ]] && rm -rf $(BIN_DIR) || rm -f $(BIN)

help:
	@printf "The following targets are available:\n"
	@printf " all:\t\tBuild \`%s'.\n" $(BIN)
	@printf " install:\tBuild and install \`%s' to \`%s'.\n" $(BIN) $(BIN_DIR)
	@printf " test:\t\tBuild and execute \`%s'.\n" $(BIN)
	@printf " clean:\t\tRemove all build artifacts.\n"
	@printf "To enable debugging, supply the argument \`DEBUG=yes'.\n"
