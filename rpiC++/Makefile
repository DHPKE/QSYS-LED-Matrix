# Makefile for RPi C++ LED Matrix Controller

# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O3 -march=native
CXXFLAGS += -I/usr/local/include -I./
LDFLAGS = -L/usr/local/lib
LIBS = -lrgbmatrix -lpthread -lfreetype -lboost_system -lboost_thread

# FreeType flags
CXXFLAGS += $(shell pkg-config --cflags freetype2)
LIBS += $(shell pkg-config --libs freetype2)

# Target binary
TARGET = led-matrix

# Source files
SOURCES = main.cpp segment_manager.cpp udp_handler.cpp text_renderer.cpp
OBJECTS = $(SOURCES:.cpp=.o)

# Build targets
all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)
	@echo "Build complete: $(TARGET)"
	@echo "Run with: sudo ./$(TARGET)"

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)
	@echo "Clean complete"

install: $(TARGET)
	@echo "Installing $(TARGET)..."
	sudo cp $(TARGET) /usr/local/bin/
	sudo cp led-matrix.service /etc/systemd/system/
	sudo mkdir -p /var/lib/led-matrix
	sudo chmod 755 /var/lib/led-matrix
	sudo systemctl daemon-reload
	sudo systemctl enable led-matrix
	@echo "Installation complete"
	@echo "Start with: sudo systemctl start led-matrix"
	@echo "Check status: sudo systemctl status led-matrix"
	@echo "View logs: sudo journalctl -u led-matrix -f"

uninstall:
	@echo "Uninstalling..."
	sudo systemctl stop led-matrix || true
	sudo systemctl disable led-matrix || true
	sudo rm -f /etc/systemd/system/led-matrix.service
	sudo rm -f /usr/local/bin/$(TARGET)
	sudo systemctl daemon-reload
	@echo "Uninstall complete"
	@echo "Config files remain in /var/lib/led-matrix (remove manually if needed)"

.PHONY: all clean install uninstall
