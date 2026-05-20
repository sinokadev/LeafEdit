default: build run

setup:
    cmake -S . -B build

debug:
    cmake -S . -B build -D CMAKE_BUILD_TYPE=Debug

build:
    cmake --build build

run:
    ./build/leafedit

exec:
    ./build/leafedit

clean:
    rm -rf build/

list:
    @just --list

help: list