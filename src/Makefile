CXX ?= g++
LVL ?= -g -O0
OPT += $(LVL)

LDLIBCPP = -static-libstdc++
CFLAGS = -std=c++11 -Wall -w $(OPT)
INCFLAGS =
LDFLAGS += $(LDLIBCPP) -rdynamic -lpthread

-include .make-settings

target = raft
objs = \
	 sds.o \
	 slice.o \
	 util.o \
	 logger.o \
	 eventloop.o \
	 main.o

.PHONY : default debug clean

default: $(target)
	@echo ""
	@echo "Compile Done."
	@echo ""

$(target): $(objs)
	$(CXX) $(CFLAGS) -o $@ $^ $(LDFLAGS)

debug:
	@make LVL=-g

clean:
	@rm -rf $(objs) $(target)
	@(rm -rf .make-*)
	@echo ""
	@echo "Clean Done."
	@echo ""

%.o : %.cpp
	$(CXX) $(CFLAGS) -c $^ $(INCFLAGS)
