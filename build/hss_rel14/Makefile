CC := g++ # This is the main compiler

OPENAIRCN_DIR :=../..
OAI_HSS_DIR := $(OPENAIRCN_DIR)/src/hss_rel14
OAI_MODULES_DIR := $(OPENAIRCN_DIR)/build/hss_rel14
OAI_HSS_BUILD_DIR := $(OPENAIRCN_DIR)/build/hss_rel14



SRCDIR := $(OAI_HSS_DIR)/src
BINDIR := $(OAI_HSS_BUILD_DIR)/bin
BUILDDIR := $(OAI_HSS_BUILD_DIR)/build
TARGETDIR := $(OAI_HSS_BUILD_DIR)/bin
TARGET := $(TARGETDIR)/hss
 
SRCEXT := cpp
SOURCES := $(shell find $(SRCDIR) -type f -name *.$(SRCEXT))
OBJECTS := $(patsubst $(SRCDIR)/%,$(BUILDDIR)/%,$(SOURCES:.$(SRCEXT)=.o))
DEPENDS := $(OBJECTS:%.o=%.d)
COPT_FLAGS := -g
CFLAGS := $(COPT_FLAGS) -pthread -std=c++11 # -Wall
LFLAGS := $(COPT_FLAGS) -pthread -lpthread -Wl,-rpath,/usr/local/lib/x86_64-linux-gnu:/usr/local/lib
LIBS := \
 $(OAI_HSS_BUILD_DIR)/util/lib/libc3po.a \
 $(OAI_HSS_BUILD_DIR)/hsssec/lib/libhsssec.a \
 /usr/local/lib/libpistache.a \
 /usr/local/lib/libfdcore.so \
 /usr/local/lib/libfdproto.so \
 -L/usr/local/lib/x86_64-linux-gnu \
 -lcassandra \
 -lrt \
 -lnettle \
 -lgmp 

INCS := \
 -I $(OAI_HSS_DIR)/include \
 -I $(OAI_HSS_DIR)/util/include \
 -I $(OAI_MODULES_DIR)/../git_submodules/rapidjson/include \
 -I $(OAI_MODULES_DIR)/../git_submodules/spdlog/include \
 -I /usr/local/include/freeDiameter \
 -I $(OAI_HSS_DIR)/hsssec/include \
 -I /usr/local/include

$(TARGET): $(OBJECTS)
	@echo " Linking..."
	@mkdir -p $(BINDIR)
	@echo " $(CC) $(LFLAGS) $^ -o $(TARGET) $(LIBS)"; $(CC) $(LFLAGS) $^ -o $(TARGET) $(LIBS)

$(BUILDDIR)/%.o: $(SRCDIR)/%.$(SRCEXT)
	@mkdir -p $(BUILDDIR)
	@echo " $(CC) $(CFLAGS) $(INCS) -MMD -c -o $@ $<"; $(CC) $(CFLAGS) $(INCS) -MMD -c -o $@ $<

clean:
	@echo " Cleaning..."; 
	@echo " $(RM) -r $(BUILDDIR) $(TARGET)"; $(RM) -r $(BUILDDIR) $(TARGET)

-include $(DEPENDS)

.PHONY: clean
