CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pthread
SRC      = src/main.cpp src/Memory.cpp src/Scheduler.cpp src/OS.cpp src/tests.cpp
TARGET   = os_sim

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

run: all
	./$(TARGET)
