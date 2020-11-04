CC := gcc # This is the main compiler
OPENAIRCN_DIR :=../../..
OAI_HSS_DIR := $(OPENAIRCN_DIR)/src/hss_rel14
OAI_HSS_HSSSEC_DIR := $(OPENAIRCN_DIR)/src/hss_rel14/hsssec
OAI_MODULES_DIR := $(OPENAIRCN_DIR)/build/hss_rel14
OAI_BUILD_DIR := $(OPENAIRCN_DIR)/build/hss_rel14/hsssec

SECURITY_FLAGS := -D_FORTIFY_SOURCE=2 -fasynchronous-unwind-tables -fexceptions
SECURITY_FLAGS += -fstack-protector-all -fstack-protector-strong -Wall
SECURITY_FLAGS += -Werror=format-security -Werror=implicit-function-declaration

SRCDIR := $(OAI_HSS_HSSSEC_DIR)/src
BUILDDIR := $(OAI_BUILD_DIR)/build
TARGETDIR := $(OAI_BUILD_DIR)/lib
LIBNAME := libhsssec.a
TARGET := $(TARGETDIR)/$(LIBNAME)
INSTDIRINC := /usr/local/include/hsssec
INSTDIRLIB := /usr/local/lib
INSTLIB := $(INSTDIRLIB)/$(LIBNAME)

SRCEXT := c
SOURCES := $(shell find $(SRCDIR) -type f -name *.$(SRCEXT))
OBJECTS := $(patsubst $(SRCDIR)/%,$(BUILDDIR)/%,$(SOURCES:.$(SRCEXT)=.o))
DEPENDS := $(OBJECTS:%.o=%.d)
COPT_FLAGS := -g
CFLAGS := -std=c99 -Wreturn-type $(COPT_FLAGS) -pthread -lrt $(SECURITY_FLAGS)# -Wall -DNODEBUG
INC := -I $(OAI_HSS_HSSSEC_DIR)/include

$(TARGET): $(OBJECTS)
	@echo " Linking..."
	@mkdir -p $(TARGETDIR)
	@echo " ar rcs $(TARGET) $(OBJECTS)"; ar rcs $(TARGET) $(OBJECTS)

$(BUILDDIR)/%.o: $(SRCDIR)/%.$(SRCEXT) Makefile
	@mkdir -p $(BUILDDIR)
	@echo " $(CC) $(CFLAGS) $(INC) -MMD -c -o $@ $<"; $(CC) $(CFLAGS) $(INC) -MMD -c -o $@ $<

clean:
	@echo " Cleaning...";
	@echo " $(RM) -r $(BUILDDIR) $(TARGETDIR)"; $(RM) -r $(BUILDDIR) $(TARGETDIR)

install: $(TARGET)
	@echo " Installing..."
	@echo "  creating/verifying $(INSTDIRINC)"
	@mkdir -p $(INSTDIRINC)
	@echo "  copying headers to $(INSTDIRINC)"
	@cp -a include/* $(INSTDIRINC)
	@echo "  creating/verifying $(INSTDIRLIB)"
	@mkdir -p $(INSTDIRLIB)
	@echo "  copying $(TARGET) to $(INSTLIB)"
	@cp -a $(TARGET) $(INSTLIB)
	@echo " Installation complete"

-include $(DEPENDS)

.PHONY: clean
