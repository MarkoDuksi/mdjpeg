MAIN_BASENAME = main
SRC_DIR = src
OBJ_DIR = obj
LIB_DIRS = lib
HDR_DIRS = include
BIN_DIR = bin
DEP_DIR = .deps
TESTS_DIR = tests
DOXY_TREE = doc/doxy*

CXX = g++
CXX_FLAGS = -std=c++17 -fdiagnostics-color=always -pedantic -Wall -Wextra -Wunreachable-code -Wfatal-errors
CXX_DEBUG_FLAGS = -g -DDEBUG
CXX_RELEASE_FLAGS = -O3 -DNDEBUG -DRELEASE
DEP_FLAGS = -MT $@ -MMD -MP -MF $(DEP_DIR)/$(*D)/$(*F).$(BUILD_TYPE).d.tmp
LIB_INCLUDE_DIRS_FLAGS = $(addprefix -L, $(LIB_DIRS))
HDR_INCLUDE_DIRS_FLAGS = $(addprefix -I, $(HDR_DIRS))
LD_FLAGS = -lfmt

POSTCOMPILE = @mv -f $(DEP_DIR)/$(*D)/$(*F).$(BUILD_TYPE).d.tmp $(DEP_DIR)/$(*D)/$(*F).$(BUILD_TYPE).d && touch $@

SRCS = $(wildcard $(shell find $(SRC_DIR) -name '*.cpp'))
SRCS_SUBDIRS = $(wildcard $(shell find $(SRC_DIR)/* -type d))
DEPS = $(SRCS:$(SRC_DIR)/%.cpp=$(DEP_DIR)/%.debug.d) $(SRCS:$(SRC_DIR)/%.cpp=$(DEP_DIR)/%.release.d)
DEP_SUBDIRS = $(SRCS_SUBDIRS:$(SRC_DIR)/%=$(DEP_DIR)/%)
DEP_TREE = $(DEP_DIR) $(DEP_SUBDIRS)

DEBUG_OBJ_DIR = $(OBJ_DIR)/debug
DEBUG_OBJ_SUBDIRS = $(SRCS_SUBDIRS:$(SRC_DIR)/%=$(DEBUG_OBJ_DIR)/%)
DEBUG_OBJ_TREE = $(DEBUG_OBJ_DIR) $(DEBUG_OBJ_SUBDIRS)
DEBUG_BIN_DIR = $(BIN_DIR)/debug
DEBUG_OBJS = $(SRCS:$(SRC_DIR)/%.cpp=$(DEBUG_OBJ_DIR)/%.o)
DEBUG_BIN = $(DEBUG_BIN_DIR)/$(MAIN_BASENAME).out

RELEASE_OBJ_DIR = $(OBJ_DIR)/release
RELEASE_OBJ_SUBDIRS = $(SRCS_SUBDIRS:$(SRC_DIR)/%=$(RELEASE_OBJ_DIR)/%)
RELEASE_OBJ_TREE = $(RELEASE_OBJ_DIR) $(RELEASE_OBJ_SUBDIRS)
RELEASE_BIN_DIR = $(BIN_DIR)/release
RELEASE_OBJS = $(SRCS:$(SRC_DIR)/%.cpp=$(RELEASE_OBJ_DIR)/%.o)
RELEASE_BIN = $(RELEASE_BIN_DIR)/$(MAIN_BASENAME).out


.PHONY: all
all: debug release

.PHONY: debug
debug: $(DEBUG_BIN)

.PHONY: release
release: $(RELEASE_BIN)


$(DEBUG_BIN): $(DEBUG_OBJS) | $(DEBUG_BIN_DIR)
	$(CXX) $(CXX_DEBUG_FLAGS) $(CXX_FLAGS) $^ -o $@ $(LIB_INCLUDE_DIRS_FLAGS) $(LD_FLAGS)

$(DEBUG_OBJ_DIR)/%.o: BUILD_TYPE = debug
$(DEBUG_OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(DEP_DIR)/%.debug.d | $(DEBUG_OBJ_TREE) $(DEP_TREE)
	$(CXX) $(DEP_FLAGS) $(CXX_DEBUG_FLAGS) $(CXX_FLAGS) $(HDR_INCLUDE_DIRS_FLAGS) -c $< -o $@
	$(POSTCOMPILE)


$(RELEASE_BIN): $(RELEASE_OBJS) | $(RELEASE_BIN_DIR)
	$(CXX) $(CXX_RELEASE_FLAGS) $(CXX_FLAGS) $^ -o $@ $(LIB_INCLUDE_DIRS_FLAGS) $(LD_FLAGS)

$(RELEASE_OBJ_DIR)/%.o: BUILD_TYPE = release
$(RELEASE_OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(DEP_DIR)/%.release.d | $(RELEASE_OBJ_TREE) $(DEP_TREE)
	$(CXX) $(DEP_FLAGS) $(CXX_RELEASE_FLAGS) $(CXX_FLAGS) $(HDR_INCLUDE_DIRS_FLAGS) -c $< -o $@
	$(POSTCOMPILE)


$(DEBUG_BIN_DIR) $(DEBUG_OBJ_TREE) $(RELEASE_BIN_DIR) $(RELEASE_OBJ_TREE) $(DEP_TREE):
	@mkdir -p $@

.PHONY: doxy
doxy: clean-doxy
	doxygen

.PHONY: clean-debug
clean-debug:
	rm -rf $(DEBUG_OBJ_DIR) $(DEBUG_BIN_DIR) $(addsuffix /*debug.d, $(DEP_TREE))

.PHONY: clean-release
clean-release:
	rm -rf $(RELEASE_OBJ_DIR) $(RELEASE_BIN_DIR) $(addsuffix /*release.d, $(DEP_TREE))

.PHONY: clean-all
clean-all:
	rm -rf $(OBJ_DIR) $(BIN_DIR) $(DEP_DIR)

.PHONY: clean-tests
clean-tests:
	find $(TESTS_DIR) -type f -regex '.+\/.+\.pgm' -delete
	find $(TESTS_DIR) -type d -regex '.+\/[0-9]+x[0-9]+\/.+' -delete

.PHONY: clean-doxy
clean-doxy:
	rm -rf $(DOXY_TREE)


$(DEPS):
include $(wildcard $(DEPS))
