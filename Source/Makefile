CXX = g++
FLAGS = -Wall -Werror -Wno-sign-compare -Wno-unused-function  -O2 -std=c++11
BUILDIR = ../build
SOURCES = common.cpp csv_reader.cpp dataframe.cpp rules.cpp algorithm.cpp
OBJECTS = $(addprefix $(BUILDIR)/,$(SOURCES:cpp=o))
LIBRARY = $(BUILDIR)/librise.so
SOURCES_BIN = common_test.cpp csv_reader_test.cpp dataframe_test.cpp rules_test.cpp algorithm_test.cpp rise_classifier.cpp
BINARIES = $(addprefix $(BUILDIR)/,$(basename $(SOURCES_BIN)))

all: $(LIBRARY) $(BINARIES) 

define COMPILE_OBJ
$(BUILDIR)/$(1:cpp=o): $(1) $(1:cpp=h)
	$(CXX) -c $(FLAGS) -fpic $(1) -o $(BUILDIR)/$(1:cpp=o)
endef

define COMPILE_BIN
$(BUILDIR)/$(basename $(1)): $(BUILDIR)/librise.so $(1)
	$(CXX) -c $(FLAGS) $(1) -o $(BUILDIR)/$(1:cpp=o)
	g++ -L$(BUILDIR) -Wl,-rpath=$(realpath $(BUILDIR)) -o $(BUILDIR)/$(basename $(1)) $(BUILDIR)/$(1:cpp=o) -lrise
endef

$(foreach source,$(SOURCES),$(eval $(call COMPILE_OBJ,$(source))))

$(BUILDIR)/librise.so: $(OBJECTS)
	g++ -shared -o $(BUILDIR)/librise.so $(OBJECTS)

$(foreach source,$(SOURCES_BIN),$(eval $(call COMPILE_BIN,$(source))))


