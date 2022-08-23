CXX = g++
INCLUDES = -Iinclude
SRC = examples/example.cpp
OBJ = $(SRC:.cp = .o)

LFLAGS = -pthread
CFLAGS = -g -Wall -std=c++11 -DDEBUG -pipe

.PHONY:clean

test:$(OBJ)
	$(CXX) $(CFLAGS) $(LFLAGS) $(INCLUDES) $(OBJ) -o test
.cpp.o:
	$(CXX) $(CFLAGS) $(INCLUDES) -c $< -o $@
clean:
	rm $(OBJ)
