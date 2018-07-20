CC = g++
NVCC = nvcc

ADDFLAGS = -L/usr/local/cuda/cuda-8.0/lib64
CFLAGS =	-I/usr/include -I/usr/local/cuda/cuda-8.0/include -g -Wall -std=c++11
NCFLAGS = -std=c++11 -arch=sm_61
LIBS = -pthread -lboost_system -ljsoncpp -ldlib -lcurl
LIBS += -lcudart -lcublas -lcurand -lopencv_core
chatserver: bucket_sort2d.o utils.o chat_server.o 
	g++ $(ADDFLAGS) $(CFLAGS) $^ -o $@ $(LIBS)

chat_server.o : chat_server.cpp
	$(NVCC) $(NCFLAGS) -c $^

utils.o : ../utils.cpp
	g++ -std=c++11 -c $^

bucket_sort2d.o : bucket_sort2d.cu
	$(NVCC) $(NCFLAGS) -c $^

clean:
	rm -f chatserver *.o *~ 
