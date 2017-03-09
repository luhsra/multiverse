FROM ubuntu:17.04

WORKDIR /src

# add toolchain repository
RUN apt-get update \
    && apt-get install -y \
    software-properties-common \
    python-software-properties \
    && add-apt-repository ppa:ubuntu-toolchain-r/test

# install compiler toolchain
RUN apt-get update \
    && apt-get install -y \
    gcc-7 \
    gcc-7-plugin-dev \
    g++-7 \
    make

# clean up to reduce image size
RUN apt-get remove -y \
    software-properties-common \
    python-software-properties \
    && apt-get autoremove -y \
    && rm -rf /var/lib/apt/lists/*

# set environment variables
ENV CC=gcc-7
ENV MY_CC=gcc-7
ENV CXX=g++-7

CMD make clean && make
