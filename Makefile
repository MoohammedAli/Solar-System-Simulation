CXX = g++
CC = gcc
CXXFLAGS = -Iinclude -std=c++17
LDFLAGS = -lglfw -framework OpenGL

SRCS = main.cpp \
	shader/shader.cpp \
	utils/mesh/mesh.cpp \
	utils/texture/texture.cpp \
	utils/skybox/skybox.cpp \
	utils/scene/scene.cpp \
	utils/dust/dust.cpp \
	utils/asteroids/asteroids.cpp
OBJS = $(SRCS:.cpp=.o) glad.o

all: app

glad.o: include/glad.c
	$(CC) -Iinclude -c include/glad.c -o glad.o

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

app: $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o app $(LDFLAGS)

clean:
	rm -f app *.o
