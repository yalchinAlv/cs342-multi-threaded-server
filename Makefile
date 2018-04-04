servclimake: server.c client.c
	gcc server.c -lpthread -lrt -o server
	gcc client.c -lpthread -lrt -o client
