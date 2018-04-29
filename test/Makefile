CXX := g++
STD := -std=c++14
INCLUDES := ..
CPPFLAGS := $(STD) $(INCLUDES:%=-I%)
CXXFLAGS := $(STD) -Wall -g -fmax-errors=3 $(INCLUDES:%=-I%)

.PHONY: all clean

SRCS := $(shell find . -type f -name '*.cc')
EXES := $(SRCS:.cc=)
DEPS := $(SRCS:.cc=.d)

ifeq (0, $(words $(findstring $(MAKECMDGOALS), clean)))

C_program_options := ../src/program_options/program_options.cc

all: $(EXES)

-include $(DEPS)

$(DEPS): %.d: %.cc
	$(CXX) $(CPPFLAGS) $(C_$*) -MM -MT '$(@:.d=)' $< -MF $@

$(EXES): %: %.cc
	$(CXX) $(CXXFLAGS) $(C_$*) $(filter %.cc %.o,$^) -o $@ $(LDLIBS) $(L_$*)

endif

clean:
	@rm -rfv $(EXES) $(DEPS)

