
PROG=g++
ARCHIVE=ar

COMP=${PREFIX}${PROG}${COMP_VERSION}
CC_ARGS=-std=c++17 ${INCLUDE} ${DEBUG} ${USER_CC_ARGS} -c
LIBS_ARGS=${LIB_PATH} -Wl,-B${LINK_TYPE} ${LIBS} -Wl,-Bdynamic
LD=${COMP} ${USER_CC_ARGS}
CC=${COMP} ${CC_ARGS}
AR=${ARCHIVE} rvs

SOURCES := $(filter-out hardcoded-m.cpp,$(wildcard *.cpp))
OBJECTS := $(filter-out main.o,$(patsubst %.cpp,%.o,$(SOURCES)))

build: main.o lib${NAME}.a
	mkdir -p ../out
	${LD} $^ ${LIBS_ARGS} -o ../out/${NAME}
lib${NAME}.a: $(OBJECTS)
	${AR} $@ $^
clean:
	rm -f *.o *.a *.s *.ii hardcoded-m.cpp
tools:
	echo ${CC}
	echo ${LD}
	echo ${AR}
	echo ${LIBS_ARGS}

hardcoded.o: hardcoded.cpp
	../script/hardcoded .. < $^ > hardcoded-m.cpp
	${CC} hardcoded-m.cpp -o $@
%.o: %.cpp
	${CC} $^
