
PROG=g++
ARCHIVE=ar

COMP=${PREFIX}${PROG}
CC_ARGS=-std=c++17 ${INCLUDE} -c
LIBS_ARGS=${LIB_PATH} -Wl,-B${LINK_TYPE} ${LIBS} -Wl,-Bdynamic
LD=${COMP}
CC=${COMP} ${CC_ARGS}
AR=${ARCHIVE} rvs

build: main.o lib${NAME}.a
	mkdir -p ../out
	${LD} $^ ${LIBS_ARGS} -o ../out/${NAME}
lib${NAME}.a: log.o settings.o hardcoded.o paket.o mc_pakets.o
	${AR} $@ $^
clean:
	rm -f *.o *.a hardcoded-m.cpp
tools:
	echo ${CC}
	echo ${LD}
	echo ${AR}
	echo ${LIBS_ARGS}

main.o: main.cpp
	${CC} $^
log.o: log.cpp
	${CC} $^
settings.o: settings.cpp
	${CC} $^
hardcoded.o: hardcoded.cpp
	../script/hardcoded .. < $^ > hardcoded-m.cpp
	${CC} hardcoded-m.cpp -o $@
paket.o: paket.cpp
	${CC} $^
mc_pakets.o: mc_pakets.cpp
	${CC} $^
