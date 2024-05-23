CXX ?= g++

SRC_DIR = $(shell pwd)/src
OBJ_DIR = obj

CPP_FILES = $(wildcard src/*.cc)
OBJ_FILES = $(addprefix obj/,$(notdir $(CPP_FILES:.cc=.o)))

LD_FLAGS ?= -pthread -flto
CC_FLAGS ?= -Wall -std=c++11 -O3 -march=native -flto -pthread -fno-exceptions -D_INCBIN_

# Use INCBIN by default, and no incbin for release
EVALFILE = equi_1024x2F_5.6B_430.nnue
CC_FLAGS += -DEVALFILE=\"$(EVALFILE)\"


# can we use pext here?
# use it if is supported and no Ryzen1/2
ARCH_INFO = $(shell echo | gcc -march=native -dM -E -)

ifneq ($(findstring __BMI2__, $(ARCH_INFO)), )
	ifeq ($(findstring __znver1, $(ARCH_INFO)), )
		ifeq ($(findstring __znver2, $(ARCH_INFO)), )
			CC_FLAGS += -D_UPEXT_
		endif
	endif
endif

# Special tuning compilation
normal: CC_FLAGS  = -Wall -std=c++11 -O3 -march=native -flto -pthread -fopenmp -fno-exceptions
normal: LD_FLAGS  = -pthread -flto -fopenmp

EXE = Equisetum_dev
normal_EXE = Equisetum_normal

all: $(OBJ_DIR) $(EXE)

normal: $(OBJ_DIR) $(normal_EXE)


$(normal_EXE): $(OBJ_FILES)
	$(CXX) $(LD_FLAGS) -o $@ $^

$(EXE): $(OBJ_FILES)
	$(CXX) $(LD_FLAGS) -o $@ $^

obj/%.o: src/%.cc
	$(CXX) $(CC_FLAGS) -c -o $@ $<

$(OBJ_DIR):
	mkdir $(OBJ_DIR)

clean:
	rm -rf $(OBJ_DIR)
	rm -f $(TEST_BIN_NAME)
	rm -f $(normal_BIN_NAME)
	rm -f $(BIN_NAME)
