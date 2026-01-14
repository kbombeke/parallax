CC = gcc
CXX = g++
CFLAGS = -Wall -Wextra -std=c11 -O2
CXXFLAGS = -Wall -Wextra -std=c++11 -O2
LDFLAGS = 

# Platform detection
UNAME_S := $(shell uname -s)

# SDL2
SDL2_CFLAGS = $(shell pkg-config --cflags sdl2)
SDL2_LIBS = $(shell pkg-config --libs sdl2)

# OpenGL
ifeq ($(UNAME_S),Darwin)
	OPENGL_LIBS = -framework OpenGL
else
	OPENGL_LIBS = -lGL
endif

# OpenCV
OPENCV_CFLAGS = $(shell pkg-config --cflags opencv4 2>/dev/null || pkg-config --cflags opencv)
OPENCV_LIBS = $(shell pkg-config --libs opencv4 2>/dev/null || pkg-config --libs opencv)

CFLAGS += $(SDL2_CFLAGS) $(OPENCV_CFLAGS)
CXXFLAGS += $(SDL2_CFLAGS) $(OPENCV_CFLAGS)
LDFLAGS += $(SDL2_LIBS) $(OPENGL_LIBS) $(OPENCV_LIBS)

SRCDIR = src
BUILDDIR = build
CSOURCES = $(wildcard $(SRCDIR)/*.c)
CXXSOURCES = $(wildcard $(SRCDIR)/*.cpp)
COBJECTS = $(CSOURCES:$(SRCDIR)/%.c=$(BUILDDIR)/%.o)
CXXOBJECTS = $(CXXSOURCES:$(SRCDIR)/%.cpp=$(BUILDDIR)/%.o)
OBJECTS = $(COBJECTS) $(CXXOBJECTS)
TARGET = stereoscopicgame

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

$(BUILDDIR)/%.o: $(SRCDIR)/%.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

clean:
	rm -rf $(BUILDDIR) $(TARGET)

run: $(TARGET)
	./$(TARGET)
