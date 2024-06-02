# Common make file rules for the metadata tools

# Set the type of operating system
OS = $(shell uname)

# Directory for the intermediate build files
BUILD = ./build

# Directory for the executable programs
BINDIR = ../bin

# Use the default C compiler
CC = gcc

# Only need to create debug configurations since the tools are not compute-intensive
CFLAGS = -g

# Define the include paths for the header files
INCLUDES = -I../include -I../../common/include -I../../../external/argparse

# List of object files for linking
OBJS = $(addprefix $(BUILD)/, $(SRCS:.c=.o))

# List of dependencies
DEPS = $(addprefix $(BUILD)/, $(SRCS:.c=.d))

# Prefix for the install location (can be overridden on the command line)
PREFIX = ..

# Location for installing programs
BINDIR = $(PREFIX)/bin

ifeq ($(OS),Darwin)
# Dynamic libraries required by the programs
DYLIBS = ../../../external/argparse/build/libargparse.dylib
endif

ifeq ($(OS),Linux)
SHARED_LIBS = ../../../external/argparse/build/libargparse.so
endif


all: prepare $(BINDIR)/$(PROGRAM)


prepare:
	@#echo "make -f $(MAKEFILE) `date +\"%Y-%m-%d\"`" >$(LOGFILE)
	@#for dir in $(BUILD); do if [ ! -d $${dir} ]; then echo "Creating directory: $${dir}"; mkdir -p $${dir}; fi; done
	[ -d  $(BUILD) ] || mkdir -p $(BUILD)


$(BINDIR)/$(PROGRAM): $(OBJS)
	$(CC) $^ $(LIBPATH) $(LIBS) -o $@


$(BUILD)/%.o: %.c $(BUILD)/%.d
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@


# Read the dependency information
-include $(DEPS)


$(BUILD)/%.d: %.c
	$(CC) -MM $(CFLAGS) $(INCLUDES) $< >$@


# install: $(PROGRAM)
# 	@if [ ! -d $(BINDIR) ]; then echo "Creating directory: $(BINDIR)"; mkdir -p $(BINDIR); fi
# 	@for program in $(PROGRAM); do cp -p $${program} $(BINDIR); done
# ifeq ($(OS),Darwin)
# 	@for dylib in $(DYLIBS); do if [ ! -f $(BINDIR)/`basename $${dylib}` ]; then echo "Installing dynamic library: $${dylib}"; cp -p $${dylib} $(BINDIR); fi; done
# 	@for program in $(PROGRAM); do install_name_tool -add_rpath @executable_path/. $(BINDIR)/$${program}; done
# endif
# ifeq ($(OS),Linux)
# 	@for lib in $(SHARED_LIBS); do if [ ! -f $(BINDIR)/`basename $${lib}` ]; then echo "Installing shared library: $${lib}"; cp -p $${lib} $(BINDIR); fi; done
# 	@for program in $(PROGRAM); do patchelf --set-rpath `realpath $(BINDIR)` $(BINDIR)/$${program}; done
# endif


clean:
	rm -f $(OBJS) $(PROGRAM)


clean-all: clean
	@#for program in $(PROGRAM); do echo "Removing installed program: $(BINDIR)/$${program}"; rm -f $(BINDIR)/$${program}; done
	rm -f $(DEPS)
	@rm -f $(BINDIR)/${PROGRAM}

