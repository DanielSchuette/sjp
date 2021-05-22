.DELETE_ON_ERROR:
.EXPORT_ALL_VARIABLES:
SRCS = $(wildcard *.cc)
OBJS = $(SRCS:.cc=.o)
DEPS = $(SRCS:.cc=.d)

# Since we create all build artifacts in a dedicated build directory, we must
# reconstruct an absolute path for every object file and the resulting binary.
# Also, for them to work, dependency files must be in the same directory as the
# related object file.
BUILD_DIR_PATH = ../$(BUILD_DIR)
BIN_PATH 	   = $(BUILD_DIR_PATH)/$(BIN)
OBJS_PATH      = $(addprefix $(BUILD_DIR_PATH)/,$(OBJS))
DEPS_PATH      = $(addprefix $(BUILD_DIR_PATH)/,$(DEPS))

.PHONY: all

all: $(BIN_PATH)

# The resulting binary is created in the build directory. The main Makefile can
# then go and place it wherever it might seem appropriate.
$(BIN_PATH): $(OBJS_PATH)
	$(CC) $(LDFLAGS) -o $@ $^

# We create all object files directly in the build directory.
$(OBJS_PATH): $(BUILD_DIR_PATH)/%.o: %.cc
	$(CC) $(CCFLAGS) -MMD -MP -c -o $@ $<

-include $(DEPS_PATH)
