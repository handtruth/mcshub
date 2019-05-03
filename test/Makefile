
PROG=g++
ARCHIVE=ar
CHMOD=chmod
DEFINES=-DTEST

COMP=${PREFIX}${PROG}${COMP_VERSION}
CC_ARGS=-std=c++17 ${INCLUDES} -I../src ${DEFINES} ${DEBUG} ${USER_CC_ARGS}
LIBS_ARGS=${LIB_PATH} -L../src -l${NAME} ${LIBS}
GCC=${COMP} ${CC_ARGS}
DEFAULT_TESTS=util.test property.test vars.test args.test inotify.test config.test paket.test varint.test dns_packet.test
TESTS:=${or ${TESTS},${TESTS},${DEFAULT_TESTS}}

test: ${TESTS}

clean:
	rm -f *.test dns_packet.txt

config.test: before
	${GCC} config.cpp ${LIBS_ARGS} -o $@
	${GDB} ./$@
paket.test: before
	${GCC} paket.cpp ${LIBS_ARGS} -o $@
	${GDB} ./$@
varint.test: before
	${GCC} varint.cpp ${LIBS_ARGS} -o $@
	${GDB} ./$@
inotify.test: before
	${GCC} inotify.cpp ${LIBS_ARGS} -o $@
	${GDB} ./$@
dns_packet.test: before
	${GCC} dns_packet.cpp ${LIBS_ARGS} -o $@
	${GDB} ./$@
args.test: before
	${GCC} args.cpp ${LIBS_ARGS} -o $@
	${GDB} ./$@
vars.test: before
	${GCC} vars.cpp ${LIBS_ARGS} -o $@
	${GDB} ./$@
property.test: before
	${GCC} property.cpp ${LIBS_ARGS} -o $@
	${GDB} ./$@
util.test: before
	${GCC} util.cpp ${LIBS_ARGS} -o $@
	${GDB} ./$@

before:
	make -C ../src lib${NAME}.a
