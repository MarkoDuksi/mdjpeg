MAIN_BASENAME = main
LIBRARY_BASENAME = libmdjpeg
SRC_DIR = src
LIB_INCLUDE_DIRS = lib
HDR_INCLUDE_DIRS = include
OBJ_DIR = obj
BIN_DIR = bin
LIB_DIR = $(OBJ_DIR)/library
DEP_DIR = .deps
TESTS_DIR = test_imgs
DOXY_TREE = doc/doxy*

CXX = g++
CXX_FLAGS = -std=c++17 -fdiagnostics-color=always -pedantic -Wall -Wextra -Wunreachable-code -Wfatal-errors
CXX_DEBUG_FLAGS = -g -DDEBUG
CXX_RELEASE_FLAGS = -O3 -DNDEBUG -DRELEASE
DEP_FLAGS = -MT $@ -MMD -MP -MF $(DEP_DIR)/$(*D)/$(*F).$(BUILD_TYPE).d.tmp
LIB_INCLUDE_DIRS_FLAGS = $(addprefix -L, $(LIB_INCLUDE_DIRS))
HDR_INCLUDE_DIRS_FLAGS = $(addprefix -I, $(HDR_INCLUDE_DIRS))
LD_FLAGS = -lfmt

PRECOMPILE = @mkdir -p $(@D) $(@D:$(OBJ_DIR)/$(BUILD_TYPE)%=$(DEP_DIR)%)
POSTCOMPILE = @mv -f $(DEP_DIR)/$(*D)/$(*F).$(BUILD_TYPE).d.tmp $(DEP_DIR)/$(*D)/$(*F).$(BUILD_TYPE).d && touch $@

LIB_ARCHIVER = ar
LIB_ARCHIVER_FLAGS = -crs

SRCS = $(wildcard $(shell find $(SRC_DIR) -name '*.cpp'))
SRCS_SUBDIRS = $(wildcard $(shell find $(SRC_DIR)/* -type d))
DEPS = $(SRCS:$(SRC_DIR)/%.cpp=$(DEP_DIR)/%.debug.d) $(SRCS:$(SRC_DIR)/%.cpp=$(DEP_DIR)/%.release.d)
DEP_SUBDIRS = $(SRCS_SUBDIRS:$(SRC_DIR)/%=$(DEP_DIR)/%)
DEP_TREE = $(DEP_DIR) $(DEP_SUBDIRS)

DEBUG_OBJ_DIR = $(OBJ_DIR)/debug
DEBUG_BIN_DIR = $(BIN_DIR)/debug
DEBUG_OBJS = $(SRCS:$(SRC_DIR)/%.cpp=$(DEBUG_OBJ_DIR)/%.o)
DEBUG_BIN = $(DEBUG_BIN_DIR)/$(MAIN_BASENAME).out

RELEASE_OBJ_DIR = $(OBJ_DIR)/release
RELEASE_BIN_DIR = $(BIN_DIR)/release
RELEASE_OBJS = $(SRCS:$(SRC_DIR)/%.cpp=$(RELEASE_OBJ_DIR)/%.o)
RELEASE_BIN = $(RELEASE_BIN_DIR)/$(MAIN_BASENAME).out


LIBRARY_OBJS = $(filter-out %tests.o, $(RELEASE_OBJS))
LIBRARY = $(LIB_DIR)/$(LIBRARY_BASENAME).a


.PHONY: all
all: release library

.PHONY: debug
debug: $(DEBUG_BIN)

.PHONY: release
release: $(RELEASE_BIN)

.PHONY: library
library: $(LIBRARY)


$(DEBUG_BIN): $(DEBUG_OBJS) | $(DEBUG_BIN_DIR)
	$(CXX) $(CXX_DEBUG_FLAGS) $(CXX_FLAGS) $^ -o $@ $(LIB_INCLUDE_DIRS_FLAGS) $(LD_FLAGS)

$(DEBUG_OBJ_DIR)/%.o: BUILD_TYPE = debug
.SECONDEXPANSION:
$(DEBUG_OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(DEP_DIR)/%.$$(BUILD_TYPE).d
	$(PRECOMPILE)
	$(CXX) $(DEP_FLAGS) $(CXX_DEBUG_FLAGS) $(CXX_FLAGS) $(HDR_INCLUDE_DIRS_FLAGS) -c $< -o $@
	$(POSTCOMPILE)


$(RELEASE_BIN): $(RELEASE_OBJS) | $(RELEASE_BIN_DIR)
	$(CXX) $(CXX_RELEASE_FLAGS) $(CXX_FLAGS) $^ -o $@ $(LIB_INCLUDE_DIRS_FLAGS) $(LD_FLAGS)

$(RELEASE_OBJ_DIR)/%.o: BUILD_TYPE = release
.SECONDEXPANSION:
$(RELEASE_OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(DEP_DIR)/%.$$(BUILD_TYPE).d
	$(PRECOMPILE)
	$(CXX) $(DEP_FLAGS) $(CXX_RELEASE_FLAGS) $(CXX_FLAGS) $(HDR_INCLUDE_DIRS_FLAGS) -c $< -o $@
	$(POSTCOMPILE)


$(LIBRARY): $(LIBRARY_OBJS) src/protoapi.h | $(LIB_DIR)
	$(LIB_ARCHIVER) $(LIB_ARCHIVER_FLAGS) $@ $?
	scripts/generate_lib_header.sh $(SRC_DIR)


$(DEBUG_BIN_DIR) $(RELEASE_BIN_DIR) $(LIB_DIR):
	mkdir -p $@


.PHONY: tests-validate
tests-validate:
	@scripts/validate_checksums.sh $(TESTS_DIR) || true

.PHONY: tests-update
tests-update:
	@scripts/update_checksums.sh $(TESTS_DIR) || true

.PHONY: doxy
doxy: clean-doxy
	doxygen

.PHONY: clean
clean: clean-tests clean-doxy
	rm -rf $(OBJ_DIR) $(BIN_DIR) $(DEP_DIR)

.PHONY: clean-debug
clean-debug:
	rm -rf $(DEBUG_OBJ_DIR) $(DEBUG_BIN_DIR) $(addsuffix /*debug.d, $(DEP_TREE))
	find . -type d -name $(DEP_DIR) -empty -delete
	find . -type d -name $(OBJ_DIR) -empty -delete
	find . -type d -name $(BIN_DIR) -empty -delete

.PHONY: clean-release
clean-release:
	rm -rf $(RELEASE_OBJ_DIR) $(RELEASE_BIN_DIR) $(addsuffix /*release.d, $(DEP_TREE))
	find . -type d -name $(DEP_DIR) -empty -delete
	find . -type d -name $(OBJ_DIR) -empty -delete
	find . -type d -name $(BIN_DIR) -empty -delete

.PHONY: clean-library
clean-library:
	rm -rf $(LIB_DIR)

.PHONY: clean-tests
clean-tests:
	find $(TESTS_DIR) -type f -name '*.pgm' ! -regex '.+_ref\/.+' -delete
	find $(TESTS_DIR) -type d -empty -delete

.PHONY: clean-doxy
clean-doxy:
	rm -rf $(DOXY_TREE)


$(DEPS):
include $(wildcard $(DEPS))
