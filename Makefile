CC = gcc
FLAGS = -std=c99

SERVER_LIBS = -pthread
SERVER_EXTRA = src/server.c src/http_parser.c src/parson.c src/db.c

clean:
	@rm -rf bin

generate: clean
	@mkdir -p bin/server/db
	@echo "[{\"id\":0,\"area\":\"Kyiv\",\"latitude\":50.45466,\"longitude\":30.5238,\"value\":0.8}]" >> bin/server/db/air.json

all: generate
	$(CC) $(FLAGS) $(SERVER_LIBS) -o bin/server/server server/main.c $(SERVER_EXTRA)