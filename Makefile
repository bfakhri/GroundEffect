CXX = g++
CXXFLAGS = -std=c++17 -O3 -Wall -Wextra -Iinclude $(shell pkg-config --cflags sdl2)
LDFLAGS = $(shell pkg-config --libs sdl2)

TARGET = GroundEffect
TEST_TARGET = GroundEffectTests
SRCS = $(wildcard src/*.cpp)
LIB_SRCS = $(filter-out src/main.cpp,$(SRCS))
OBJS = $(SRCS:.cpp=.o)
LIB_OBJS = $(LIB_SRCS:.cpp=.o)
TEST_OBJS = tests/test_main.o

.PHONY: all clean run test

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $(TARGET) $(LDFLAGS)

$(TEST_TARGET): $(LIB_OBJS) $(TEST_OBJS)
	$(CXX) $(LIB_OBJS) $(TEST_OBJS) -o $(TEST_TARGET) $(LDFLAGS)

src/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

tests/%.o: tests/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

test: $(TEST_TARGET)
	SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ./$(TEST_TARGET)

clean:
	rm -f $(OBJS) $(TEST_OBJS) $(TARGET) $(TEST_TARGET)

run: $(TARGET)
	./$(TARGET)
