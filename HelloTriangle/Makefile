CFLAGS = -std=c++23 -O2 # Defining the version of C++
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi # Linking the libraries

# -g is used by g++ to signal debug 
# -UNDEBUG means NDEBUG is not defined for the compiled program
DEBUG_PARAMETER = -g -UNDEBUG 

# -DNDEBUG is used to define NDEBUG (not debug) to disable validation layers
# NDEBUG is also tied with assertions
RELEASE_PARAMETER = -DNDEBUG
DEBUGFILE = VulkanTest.out	# Name of the compiled program in debug phase
RELEASEFILE = VulkanTestR.out	# Name of the compiled program in release phase

# Runs when running the "make" command in the directory 
# Compiles the program with validation layers and debug
VulkanTest: main.cpp
	g++ $(CFLAGS) $(DEBUG_PARAMETER) -o $(DEBUGFILE) main.cpp $(LDFLAGS)

# Defines the additional functions that can be used with the "make" command
# E.g.:  "make test" runs the test command defined below
.PHONY: test clean release rel

# For compiling program without validation layers
release: 
	g++ $(CFLAGS) $(RELEASE_PARAMETER) -o $(RELEASEFILE) main.cpp $(LDFLAGS)

# Runs the complied program with validation layers
test: 
	./$(DEBUGFILE)

# Removes the compiled debug-program
clean:
	rm -f $(DEBUGFILE)

# Runs the compiled release-program
rel: 
	./$(RELEASEFILE)
