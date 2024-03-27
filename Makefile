TARGET := avrds

CFLAGS := -Og -g

OBJECTS := atmega328p.o\
		   cpu.o \
		   instruction_set.o \
		   main.o

.PHONY: clean

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

$(TARGET): $(OBJECTS)
	$(CC) -o $(TARGET) $(OBJECTS)

clean:
	rm *.o $(TARGET)

