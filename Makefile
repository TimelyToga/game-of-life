CC = clang
CFLAGS = -Wall -Wextra -std=c99 -O3 -I/opt/homebrew/include -I/usr/local/include
LDFLAGS = -L/opt/homebrew/lib -L/usr/local/lib -lraylib -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo

BUILD_DIR = build
TARGET_MAIN = $(BUILD_DIR)/gol
SRC_MAIN = main.c

all: $(TARGET_MAIN)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET_MAIN): $(SRC_MAIN) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(SRC_MAIN) -o $(TARGET_MAIN) $(LDFLAGS)

clean:
	rm -rf $(BUILD_DIR)

run: $(TARGET_MAIN)
	./$(TARGET_MAIN)

.PHONY: all clean run