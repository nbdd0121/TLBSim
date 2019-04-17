OBJS = sim.o walker.o config.o stats.o util.o tlb.o validator.o offline.o

CXX=g++
CXX_FLAGS=-Iinclude/ -std=gnu++17 -O3 -flto -Wall -Werror -fpic $(shell pkg-config --cflags jsoncpp)

LD=g++
LD_FLAGS=-g -O3 -flto -shared -fpic

all: libtlbsim.so

.PHONY: all clean

clean:
	rm $(patsubst %,bin/%,$(OBJS) $(OBJS:.o=.d))

libtlbsim.so: $(patsubst %,bin/%,$(OBJS))
	$(LD) $(LD_FLAGS) $^  $(shell pkg-config --libs jsoncpp) -o $@

-include $(patsubst %,bin/%,$(OBJS:.o=.d))

bin/%.o: src/%.cc
	@mkdir -p $(dir $@)
	$(CXX) -c -MMD -MP $(CXX_FLAGS) $< -o $@
