CFLAGS =	-I/usr/include -g -Wall -std=c++11
LIBS = -pthread -lboost_system -ljsoncpp -lboost_thread
chatclient: chat_client.o
	g++ $(CFLAGS) -o $@ $^ $(LIBS)

clean:
	rm -f chatclient *~ 
