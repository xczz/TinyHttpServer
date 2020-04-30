HttpServer: Task.cpp SocketOps.cpp http_server.cpp
	g++ Task.cpp SocketOps.cpp http_server.cpp -pthread -std=c++11 -o HttpServer