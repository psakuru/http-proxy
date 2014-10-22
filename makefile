CC = g++
TARGET = proxy client
LIBS = 
DEP = httputil.o

all:    $(TARGET)

client: client.cpp $(DEP) 
	$(CC) -o $@ $^ $(LIBS)

proxy: proxy.cpp $(DEP) 
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

%.o:    %.cpp
	$(CC) -c $(CFLAGS) $< -o $@ $(LIBS)

clean:
	rm -f *.o 
	rm -f $(TARGET)
