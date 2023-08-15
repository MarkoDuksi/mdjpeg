MAIN_BASENAME = main
SRC_DIR = src
OBJ_DIR = obj
LIB_DIRS = lib
HDR_DIRS = include
BIN_DIR = bin
DEP_DIR = .deps

CXX = g++
CXX_FLAGS = -std=c++17 -pthread -fdiagnostics-color=always -pedantic -Wall -Wextra -Wunreachable-code -Wfatal-errors
CXX_DEBUG_FLAGS = -g -DDEBUG
CXX_RELEASE_FLAGS = -DNDEBUG -DRELEASE
DEP_FLAGS = -MT $@ -MMD -MP -MF $(DEP_DIR)/$(*F).$(BUILD_TYPE).d.tmp
LIB_INCLUDE_DIRS_FLAGS = $(addprefix -L, $(LIB_DIRS))
HDR_INCLUDE_DIRS_FLAGS = $(addprefix -I, $(HDR_DIRS))
LD_FLAGS = -lX11 -lfmt

POSTCOMPILE = @mv -f $(DEP_DIR)/$(*F).$(BUILD_TYPE).d.tmp $(DEP_DIR)/$(*F).$(BUILD_TYPE).d && touch $@

SRCS = $(wildcard $(SRC_DIR)/*.cpp)
DEPS = $(SRCS:$(SRC_DIR)/%.cpp=$(DEP_DIR)/%.debug.d) $(SRCS:$(SRC_DIR)/%.cpp=$(DEP_DIR)/%.release.d)

DEBUG_OBJ_DIR = $(OBJ_DIR)/debug
DEBUG_BIN_DIR = $(BIN_DIR)/debug
DEBUG_OBJS = $(SRCS:$(SRC_DIR)/%.cpp=$(DEBUG_OBJ_DIR)/%.o)
DEBUG_BIN = $(BIN_DIR)/debug/$(MAIN_BASENAME).out

RELEASE_OBJ_DIR = $(OBJ_DIR)/release
RELEASE_BIN_DIR = $(BIN_DIR)/release
RELEASE_OBJS = $(SRCS:$(SRC_DIR)/%.cpp=$(RELEASE_OBJ_DIR)/%.o)
RELEASE_BIN = $(BIN_DIR)/release/$(MAIN_BASENAME).out


.PHONY: all
all: debug release

.PHONY: debug
debug: $(DEBUG_BIN)

.PHONY: release
release: $(RELEASE_BIN)


$(DEBUG_BIN): $(DEBUG_OBJS) | $(DEBUG_BIN_DIR)
	$(CXX) $(CXX_DEBUG_FLAGS) $(CXX_FLAGS) $^ -o $@ $(LIB_INCLUDE_DIRS_FLAGS) $(LD_FLAGS)

$(DEBUG_OBJ_DIR)/%.o: BUILD_TYPE = debug
$(DEBUG_OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(DEP_DIR)/%.debug.d | $(DEBUG_OBJ_DIR) $(DEP_DIR)
	$(CXX) $(DEP_FLAGS) $(CXX_DEBUG_FLAGS) $(CXX_FLAGS) $(HDR_INCLUDE_DIRS_FLAGS) -c $< -o $@
	$(POSTCOMPILE)


$(RELEASE_BIN): $(RELEASE_OBJS) | $(RELEASE_BIN_DIR)
	$(CXX) $(CXX_RELEASE_FLAGS) $(CXX_FLAGS) $^ -o $@ $(LIB_INCLUDE_DIRS_FLAGS) $(LD_FLAGS)

$(RELEASE_OBJ_DIR)/%.o: BUILD_TYPE = release
$(RELEASE_OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(DEP_DIR)/%.release.d | $(RELEASE_OBJ_DIR) $(DEP_DIR)
	$(CXX) $(DEP_FLAGS) $(CXX_RELEASE_FLAGS) $(CXX_FLAGS) $(HDR_INCLUDE_DIRS_FLAGS) -c $< -o $@
	$(POSTCOMPILE)


$(DEBUG_BIN_DIR) $(DEBUG_OBJ_DIR) $(RELEASE_BIN_DIR) $(RELEASE_OBJ_DIR) $(DEP_DIR):
	@mkdir -p $@


.PHONY: clean-debug
clean-debug:
	rm -rf $(DEBUG_OBJ_DIR) $(DEBUG_BIN_DIR) $(DEP_DIR)/*debug.d

.PHONY: clean-release
clean-release:
	rm -rf $(RELEASE_OBJ_DIR) $(RELEASE_BIN_DIR) $(DEP_DIR)/*release.d

.PHONY: clean
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR) $(DEP_DIR)


$(DEPS):
include $(wildcard $(DEPS))
