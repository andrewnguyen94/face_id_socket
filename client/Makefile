CFLAGS =	-I/usr/include -g -Wall -std=c++11
LIBS = -pthread -lboost_system -ljsoncpp -lboost_thread
LIBS += -ldlib
chatclient: chat_client.o
	g++ $(CFLAGS) $^ -o $@ $(LIBS)

chat_client.o: chat_client.cpp
	g++ -std=c++11 -c $^ 

clean:
	rm -f chatclient *.o *~ 
