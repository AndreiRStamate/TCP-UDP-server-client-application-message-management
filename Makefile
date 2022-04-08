build:
	g++ server.cc -o server
	g++ subscriber.cc -o subscriber

clean:
	rm -f server subscriber
