all:
	gcc server.c -o server -lsqlite3
	gcc client.c -o client
	echo "make ok"
clean:
	rm server
	rm client
			
