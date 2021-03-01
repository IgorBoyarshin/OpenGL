# The name of the main file and executable
mainFileName = main
# Files that have .h and .cpp versions
classFiles =
# Files that only have the .h version
justHeaderFiles = Shader Texture
# Compilation flags
OPTIMIZATION_FLAG = -O0
LANGUAGE_LEVEL = -std=c++20
COMPILER_FLAGS = -Wall -Wextra -Wno-unused-parameter
LINKER_FLAGS = -lm -lGL -lGLU -lglfw -lGLEW -lXi -lX11 -lpthread -lXrandr -ldl -lXmu


# Auxiliary
filesObj = $(addsuffix .o, $(mainFileName) $(classFiles))
filesH = $(addsuffix .h, $(classFiles) $(justHeaderFiles))


all: cleanExe $(mainFileName)


# Compiler
%.o: %.cpp $(filesH)
	g++ $(COMPILER_FLAGS) $(OPTIMIZATION_FLAG) $(LANGUAGE_LEVEL) -c $<

# Linker
$(mainFileName): $(filesObj)
	g++ $(COMPILER_FLAGS) $(OPTIMIZATION_FLAG) $(LANGUAGE_LEVEL) $(LINKER_FLAGS) $^ -o $@


# Utils
clean:
	rm -f a.out *.o *.gch .*.gch $(mainFileName)

cleanExe:
	rm -f $(mainFileName)
