FROM ubuntu:latest
MAINTAINER Jake Holland <jholland@akamai.com>

RUN apt-get update && apt-get install -y build-essential git automake

RUN apt-get install -y cmake
RUN git clone https://github.com/GrumpyOldTroll/mcproxy && \
    cd mcproxy && cmake . && make

ENTRYPOINT ["/bin/bash"]

