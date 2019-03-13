
PROG=g++
ARCHIVE=ar
CHMOD=chmod
DEFINES=-DTEST

COMP=${PREFIX}${PROG}
CC_ARGS=-std=c++17 ${INCLUDES} -I../src ${DEFINES} ${DEBUG}
LIBS_ARGS=${LIB_PATH} -L../src -l${NAME} ${LIBS}
GCC=${COMP} ${CC_ARGS}

test: inotify.test config.test paket.test varint.test dns_packet.test

clean:
	rm -f *.test

config.test: before
	${GCC} config.cpp ${LIBS_ARGS} -o $@
	./$@
paket.test: before
	${GCC} paket.cpp ${LIBS_ARGS} -o $@
	./$@
varint.test: before
	${GCC} varint.cpp ${LIBS_ARGS} -o $@
	./$@
inotify.test: before
	${GCC} inotify.cpp ${LIBS_ARGS} -o $@
	./$@
dns_packet.test: before
	${GCC} dns_packet.cpp ${LIBS_ARGS} -o $@
	./$@

before:
	make -C ../src lib${NAME}.a
