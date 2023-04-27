.PHONY: all clean

PROJECT=sdp
SRC=main.c

all:
	gcc -o $(PROJECT) $(SRC)

clean:
	-rm -f $(PROJECT)
