FROM ubuntu:24.04
# install build tools and GDAL
RUN apt-get update && apt-get install -y gcc cmake make libgdal-dev && apt-get clean

# set working directory inside the container
WORKDIR /app

# copy the project files into container
COPY . .

# build the project with cmake
RUN mkdir -p build && cd build && cmake .. && make

# make html assets available at the path the server expects
RUN cp -r src/html html

# command to run the compiled executable
CMD ["./build/web_server"]