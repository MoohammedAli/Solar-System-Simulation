# Compiler and flags
CXX = g++
CXXFLAGS = -Iinclude -std=c++17 -Wall
LDFLAGS = -lglfw -framework OpenGL

# Target executable
TARGET = app

# C++ Source files
CPP_SOURCES = main.cpp \
              shader/shader.cpp \
              utils/mesh/mesh.cpp \
              utils/texture/texture.cpp \
              utils/skybox/skybox.cpp \
              utils/scene/scene.cpp \
              utils/dust/dust.cpp \
              utils/asteroids/asteroids.cpp \
              utils/lensflare/lensflare.cpp

# C Source files
C_SOURCES = include/glad.c

# Object files
CPP_OBJECTS = $(CPP_SOURCES:.cpp=.o)
C_OBJECTS = $(C_SOURCES:.c=.o)
OBJECTS = $(CPP_OBJECTS) $(C_OBJECTS)

# Default target
all: $(TARGET)

# Link object files to create executable
$(TARGET): $(OBJECTS)
	@echo "Linking $(TARGET)..."
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o $(TARGET) $(LDFLAGS)
	@echo "✅ Build complete! Run with: ./$(TARGET)"

# Compile .cpp files to .o files
%.o: %.cpp
	@echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Compile .c files to .o files
%.o: %.c
	@echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	@echo "Cleaning build files..."
	rm -f $(OBJECTS) $(TARGET)
	rm -f shader/*.o
	rm -f utils/mesh/*.o
	rm -f utils/texture/*.o
	rm -f utils/skybox/*.o
	rm -f utils/scene/*.o
	rm -f utils/dust/*.o
	rm -f utils/asteroids/*.o
	rm -f utils/lensflare/*.o
	rm -f include/*.o
	@echo "✅ Clean complete!"

# Clean everything
cleanall: clean
	@echo "✅ All clean!"

# Run the application
run: $(TARGET)
	./$(TARGET)

# Rebuild everything from scratch
rebuild: clean all

# Help target
help:
	@echo "Available targets:"
	@echo "  make          - Build the project"
	@echo "  make clean    - Remove build files (keep glad.o)"
	@echo "  make cleanall - Remove ALL build files (including glad.o)"
	@echo "  make run      - Build and run the application"
	@echo "  make rebuild  - Clean and rebuild everything"
	@echo "  make help     - Show this help message"

# Phony targets (not actual files)
.PHONY: all clean cleanall run rebuild help
