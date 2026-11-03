CXX = g++
CXXFLAGS = -std=c++17 -O3 -Wall -Wextra -Iinclude $(shell pkg-config --cflags sdl2)
LDFLAGS = $(shell pkg-config --libs sdl2)

TARGET = GroundEffect
SRCS = $(wildcard src/*.cpp)
OBJS = $(SRCS:.cpp=.o)

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $(TARGET) $(LDFLAGS)

src/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

run: $(TARGET)
	./$(TARGET)
