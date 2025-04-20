CXX := g++
CXXFLAGS := -std=c++20 -Wall -Wextra -Wpedantic -Werror -O2
LDFLAGS :=

# Source files and output binary
SRCS := $(wildcard *.cpp)
OBJS := $(SRCS:.cpp=.o)
TARGET := peer-time-sync

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean