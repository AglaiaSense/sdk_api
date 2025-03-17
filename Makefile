# Cross-compiler prefix
#CROSS_COMPILE = aarch64-poky-linux-

# Compiler and linker
CC = $(CROSS_COMPILE)g++
LD = $(CROSS_COMPILE)g++

# Compiler flags
CFLAGS = -Wall -O2 -std=c++17 -Iinc -DDBG_EN
LDFLAGS = -lssl -lcrypto

# Source files directory
SRCDIR = src
SRCS = $(SRCDIR)/sdk_server.cpp $(SRCDIR)/sdk_log.cpp $(SRCDIR)/date_utils.cpp $(SRCDIR)/sdk_pack_json.cpp

# Object files directory
OBJDIR = obj
OBJS = $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(SRCS))

# Output executable directory
BINDIR = bin
TARGET = $(BINDIR)/sdk_server.bin

# Default target
all: $(TARGET)

# Link the object files to create the executable
$(TARGET): $(OBJS) | $(BINDIR)
	$(LD) $(OBJS) -o $(TARGET) $(LDFLAGS)

# Compile source files to object files
$(OBJDIR)/%.o: $(SRCDIR)/%.cpp | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Create the object files directory
$(OBJDIR):
	mkdir -p $(OBJDIR)

# Create the output executable directory
$(BINDIR):
	mkdir -p $(BINDIR)

# Clean up build artifacts
clean:
	rm -f $(OBJDIR)/*.o $(TARGET)
	rmdir $(OBJDIR) $(BINDIR)

# Phony targets
.PHONY: all clean