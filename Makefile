CXX=cc
CFLAGS=-std=c99
PROGRAM=fan_control
OBJECTS=fan_control.o

$(PROGRAM):$(OBJECTS)
	$(CXX) $(CFLAGS) $(OBJECTS) -o $(PROGRAM) && strip -s $(PROGRAM)

clean:
	rm *.o -v

