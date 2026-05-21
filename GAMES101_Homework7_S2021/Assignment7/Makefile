CXX := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -pthread
TARGET := Raytracing
SRCS := main.cpp Vector.cpp Scene.cpp BVH.cpp Renderer.cpp
OBJS := $(SRCS:.cpp=.o)

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) $(OBJS) binary.ppm
