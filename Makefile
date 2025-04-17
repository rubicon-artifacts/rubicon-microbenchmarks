CC = clang
CFLAGS = -O3 -Wall -Iinclude -Irubicon-mechanisms/include -Irubench -MMD -MP

SRC_DIR = src
BUILD_DIR = build
RUBICON_DIR = rubicon-mechanisms
RUBICON_SRC_DIR = $(RUBICON_DIR)/src
RUBICON_BUILD_DIR = $(RUBICON_DIR)/build

MAIN_SRC = $(wildcard *.c)
SRC = $(wildcard $(SRC_DIR)/*.c)
RUBICON_SRC = $(wildcard $(RUBICON_SRC_DIR)/*.c)

MAIN_OBJ = $(MAIN_SRC:%.c=$(BUILD_DIR)/%.o)
SRC_OBJ = $(SRC:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
RUBICON_OBJ = $(RUBICON_SRC:$(RUBICON_SRC_DIR)/%.c=$(RUBICON_BUILD_DIR)/%.o)
ALL_OBJ     = $(SRC_OBJ) $(MAIN_OBJ) $(RUBICON_OBJ)

EXEC = $(MAIN_SRC:.c=)

DEPS = $(ALL_OBJ:%.o=%.d)

all: $(EXEC)

$(EXEC): %: $(BUILD_DIR)/%.o $(SRC_OBJ) $(RUBICON_OBJ)
	$(CC) -o $@ $^ 

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(RUBICON_BUILD_DIR)/%.o: $(RUBICON_SRC_DIR)/%.c
	@mkdir -p $(RUBICON_BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

-include $(DEPS)

clean:
	rm -rf $(BUILD_DIR) $(EXEC) $(RUBICON_BUILD_DIR)

.PHONY: all clean