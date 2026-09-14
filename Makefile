CC := gcc
CFLAGS := -std=c23 -Wall -Wextra -Werror -g -O1 -Iinclude
SRC := src/rbtree.c
TSRC := tests/test_rbtree.c
BIN := build/test_rbtree
FUZZBIN := build/fuzz
FUZZSEED := 20260913
all: $(BIN) $(FUZZBIN)
$(BIN): $(SRC) $(TSRC) include/rbtree.h
	@mkdir -p build
	$(CC) $(CFLAGS) $(SRC) $(TSRC) -o $@
$(FUZZBIN): $(SRC) tests/fuzz.c include/rbtree.h
	@mkdir -p build
	$(CC) $(CFLAGS) $(SRC) tests/fuzz.c -o $@
test: $(BIN) $(FUZZBIN)
	./$(BIN) && ./$(FUZZBIN) 100000 $(FUZZSEED)
asan: CFLAGS += -fsanitize=address,undefined -fno-omit-frame-pointer
asan: clean test
memcheck: clean all
	valgrind --leak-check=full --show-leak-kinds=all \
	--error-exitcode=1 ./$(BIN)
	valgrind --leak-check=full --show-leak-kinds=all \
	--error-exitcode=1 ./$(FUZZBIN) 100000 $(FUZZSEED)
clean:
	rm -rf build
.PHONY: all test asan memcheck clean