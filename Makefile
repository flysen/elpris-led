CXX ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -O1

test: tests/test_logic
	./tests/test_logic

tests/test_logic: tests/test_logic.cpp elpris_led/elpris_logic.h
	$(CXX) $(CXXFLAGS) -o $@ tests/test_logic.cpp

clean:
	rm -f tests/test_logic

.PHONY: test clean
