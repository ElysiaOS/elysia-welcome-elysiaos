# Makefile for ElysiaOS Welcome Application

# Compiler and flags
CXX = g++
CC = gcc
CXXFLAGS = -Wall -Wextra -std=c++17 `pkg-config --cflags gtk4 glib-2.0`
CFLAGS = -Wall -Wextra `pkg-config --cflags gtk4 glib-2.0`
LIBS = `pkg-config --libs gtk4 glib-2.0`
LDFLAGS =

# Check for NetworkManager
HAVE_NM := $(shell pkg-config --exists libnm && echo YES)
ifneq ($(HAVE_NM),YES)
  $(error NetworkManager development headers not found. Please install libnm-dev or similar package)
else
  CXXFLAGS += `pkg-config --cflags libnm`
  CFLAGS += `pkg-config --cflags libnm`
  LIBS += `pkg-config --libs libnm`
endif

# Resource compilation
RESOURCE_XML = resources.gresource.xml
RESOURCE_C = resources.c
RESOURCE_O = resources.o

# Application
SRCS = welcome.cpp
OBJS = welcome.o $(RESOURCE_O)
TARGET = elysia-welcome

# Default target
all: $(TARGET)

# Compile resources
$(RESOURCE_C): $(RESOURCE_XML)
	glib-compile-resources --target=$@ --generate-source $<

# Compile object files
welcome.o: welcome.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# Compile resources as C code
$(RESOURCE_O): $(RESOURCE_C)
	$(CC) $(CFLAGS) -c -o $@ $<

# Link the application
$(TARGET): $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)

# Clean build files
clean:
	rm -f $(OBJS) $(TARGET) $(RESOURCE_C)

# Install the application
install: $(TARGET)
	install -Dm755 $(TARGET) /usr/local/bin/$(TARGET)

# Phony targets
.PHONY: all clean install