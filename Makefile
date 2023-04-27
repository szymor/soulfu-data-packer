.PHONY: all clean

PROJECT=sdp
SRC=main.c

all: $(PROJECT)

$(PROJECT): $(SRC)
	gcc -o $(PROJECT) $(SRC)

clean:
	-rm -f $(PROJECT)
