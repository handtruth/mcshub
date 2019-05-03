
PROG=g++
ARCHIVE=ar

COMP=${PREFIX}${PROG}${COMP_VERSION}
CC_ARGS=-std=c++17 ${INCLUDE} ${DEBUG} ${USER_CC_ARGS} -c
LIBS_ARGS=${LIB_PATH} -Wl,-B${LINK_TYPE} ${LIBS} -Wl,-Bdynamic
LD=${COMP} ${USER_CC_ARGS}
CC=${COMP} ${CC_ARGS}
AR=${ARCHIVE} rvs

build: main.o lib${NAME}.a
	mkdir -p ../out
	${LD} $^ ${LIBS_ARGS} -o ../out/${NAME}
lib${NAME}.a: uuid.o response_props.o log.o settings.o hardcoded.o paket.o mc_pakets.o event_pull.o socket_d.o dns_server.o reallobuf.o server.o prog_args.o directory_cache.o inotify_d.o
	${AR} $@ $^
clean:
	rm -f *.o *.a *.s *.ii hardcoded-m.cpp
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
event_pull.o: event_pull.cpp
	${CC} $^
socket_d.o: socket_d.cpp
	${CC} $^
dns_server.o: dns_server.cpp
	${CC} $^
reallobuf.o: reallobuf.cpp
	${CC} $^
server.o: server.cpp
	${CC} $^
prog_args.o: prog_args.cpp
	${CC} $^
directory_cache.o: directory_cache.cpp
	${CC} $^
inotify_d.o: inotify_d.cpp
	${CC} $^
response_props.o: response_props.cpp
	${CC} $^
uuid.o: uuid.cpp
	${CC} $^
