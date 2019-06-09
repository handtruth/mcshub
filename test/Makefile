SOURCES:=$(wildcard *.cpp)
DEFAULT_TESTS:=$(patsubst %.cpp,%.test,$(SOURCES))
TESTS:=${or ${TESTS},${TESTS},${DEFAULT_TESTS}}

.PHONY: test
test: $(TESTS)

.PHONY: clean
clean:
	rm -f *.test dns_packet.txt

%.test: %.cpp ../src/lib$(NAME).a
	$(CXX) $(CXXFLAGS) -I../src $< $(LDFLAGS) -L../src -l$(NAME) $(LDLIBS) -o $@
	$(GDB) ./$@
