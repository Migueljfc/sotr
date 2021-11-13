CC =  gcc # Path to compiler
L_FLAGS = -lrt -lpthread -lm
#C_FLAGS = -g

all: a1 a2 a3
.PHONY: all

# Project compilation
a1: a1.c 
	$(CC) $< -o $@ $(C_FLAGS) $(L_FLAGS)
a2: a2.c 
	$(CC) $< -o $@ $(C_FLAGS) $(L_FLAGS)
a3: a3.c 
	$(CC) $< -o $@ $(C_FLAGS) $(L_FLAGS)

	
.PHONY: clean 

clean:
	rm -f *.c~ 
	rm -f *.o
	rm a1 a2 a3

# Some notes
# $@ represents the left side of the ":"
# $^ represents the right side of the ":"
# $< represents the first item in the dependency list   

