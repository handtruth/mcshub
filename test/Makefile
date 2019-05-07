
PROG=g++
ARCHIVE=ar
CHMOD=chmod
DEFINES=-DTEST

COMP=${PREFIX}${PROG}${COMP_VERSION}
CC_ARGS=-std=c++17 ${INCLUDES} -I../src ${DEFINES} ${DEBUG} ${USER_CC_ARGS}
LIBS_ARGS=${LIB_PATH} -L../src -l${NAME} ${LIBS}
GCC=${COMP} ${CC_ARGS}
SOURCES:=$(wildcard *.cpp)
DEFAULT_TESTS:=$(patsubst %.cpp,%.test,$(SOURCES))
TESTS:=${or ${TESTS},${TESTS},${DEFAULT_TESTS}}

test: ${TESTS}

clean:
	rm -f *.test dns_packet.txt

%.test: %.cpp before
	${GCC} $< ${LIBS_ARGS} -o $@
	${GDB} ./$@

before:
	make -C ../src lib${NAME}.a
