
SOURCES:=$(wildcard *.cpp)
OBJECTS:=${filter-out main.o,${SOURCES:.cpp=.o}} resources.o
DEPENDS:=${SOURCES:.cpp=.d}
RESOURC:=${shell find ../res -type f}

../out/$(NAME): main.o lib$(NAME).a
	mkdir -p ../out
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o ../out/$(NAME)
lib$(NAME).a: $(OBJECTS)
	$(AR) $(ARFLAGS) $@ $^

.PHONY: clean
clean:
	$(RM) main.o $(DEPENDS) $(OBJECTS) lib$(NAME).a resources.r

-include $(DEPENDS)

%.d: %.cpp resources.r
	@$(CPP) $(CXXFLAGS) $< -MM -MT $(@:.d=.o) > $@

resources.h: resources.r
resources.r: $(RESOURC)
	../script/resources resources.h > resources.r

resources.o: resources.r
	$(CXX) -x c++ $(CXXFLAGS) -c $< -o $@
