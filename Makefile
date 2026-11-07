CXX ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -O1
SANFLAGS = -std=c++17 -Wall -Wextra -Werror -g -O1 -fsanitize=address,undefined -fno-sanitize-recover=all

test: tests/test_logic
	./tests/test_logic

# Same tests with AddressSanitizer and UndefinedBehaviorSanitizer, warnings as errors
test-sanitize: tests/test_logic_san
	./tests/test_logic_san

tests/test_logic: tests/test_logic.cpp elpris_led/elpris_logic.h
	$(CXX) $(CXXFLAGS) -o $@ tests/test_logic.cpp

tests/test_logic_san: tests/test_logic.cpp elpris_led/elpris_logic.h
	$(CXX) $(SANFLAGS) -o $@ tests/test_logic.cpp

clean:
	rm -f tests/test_logic tests/test_logic_san

.PHONY: test test-sanitize clean
