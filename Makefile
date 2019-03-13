export INCLUDE=$(shell script/get_config INCLUDE)
export LIB_PATH=$(shell script/get_config LIB_PATH)
export PREFIX=$(shell script/get_config PREFIX)
export LIBS=$(shell script/get_config LIBS)
export LINK_TYPE=$(shell script/get_config LINK_TYPE)
export NAME=$(shell script/get_config NAME)
export DEBUG=$(shell script/get_config DEBUG)

build:
	make -C src build
tools:
	make -C src tools

tests:
	make -C test test

clean:
	make -C src clean
	make -C test clean
	rm -rf out/*
