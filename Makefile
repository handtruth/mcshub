export INCLUDE=$(shell script/get_config INCLUDE)
export LIB_PATH=$(shell script/get_config LIB_PATH)
export PREFIX=$(shell script/get_config PREFIX)
export LIBS=$(shell script/get_config LIBS)
export LINK_TYPE=$(shell script/get_config LINK_TYPE)
export NAME=$(shell script/get_config NAME)
export DEBUG=$(shell script/get_config DEBUG)
export COMP_VERSION=$(shell script/get_config COMP_VERSION)
export USER_CC_ARGS=$(shell script/get_config CC_ARGS)
export TESTS

build: deps
	make -C src build
deps:
	script/resolve_deps
tools:
	make -C src tools

tests:
	make -C test test

clean:
	make -C src clean
	make -C test clean
	rm -rf out/*
