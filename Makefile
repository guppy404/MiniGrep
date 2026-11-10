CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Wpedantic
INCLUDES := -Isrc

ENGINE_SRC := src/parser.cpp src/nfa.cpp
CLI_SRC := src/main.cpp
TEST_SRC := tests/test_regex.cpp

.PHONY: all test sanitize clean run

all: minigrep

minigrep: $(CLI_SRC) $(ENGINE_SRC) src/parser.hpp src/nfa.hpp
	$(CXX) $(CXXFLAGS) -O2 $(INCLUDES) $(CLI_SRC) $(ENGINE_SRC) -o $@

test_regex: $(TEST_SRC) $(ENGINE_SRC) src/parser.hpp src/nfa.hpp
	$(CXX) $(CXXFLAGS) -g $(INCLUDES) $(TEST_SRC) $(ENGINE_SRC) -o $@

test: minigrep test_regex
	./test_regex
	bash tests/test_cli.sh

sanitize:
	$(CXX) $(CXXFLAGS) -g -O1 -fno-omit-frame-pointer \
	    -fsanitize=address,undefined $(INCLUDES) \
	    $(TEST_SRC) $(ENGINE_SRC) -o test_regex_san
	./test_regex_san

run: minigrep
	./minigrep -n 'error|warning' sample.txt

clean:
	rm -f minigrep test_regex test_regex_san
