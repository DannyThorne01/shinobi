.PHONY: build
build:
	docker build -t web_server .

run:
	docker run --rm -p 3490:3490 web_server

# Remove the Docker image
clean:
	docker rmi -f web_server