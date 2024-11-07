FILE := Makefile
OS_EXT =
ifeq ($(OS),Windows_NT)
	OS_EXT := .win32
	MYFIND :=C:/msys64/usr/bin/find.exe
else
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Linux)
		OS_EXT := .linux
    endif
    ifeq ($(UNAME_S),Darwin)
		OS_EXT := .mac
    endif
	MYFIND :=find
endif

TESTING					=
ASAN_USE				=
RELEASE_USE				=
TRACY_USE				=
GLFW3					=

COMMAND_CDEFINES		=-DCHICHI
COMMAND_CFLAGS			=-g3 -Wvarargs

ifeq ($(ASAN_USE),1)
	COMMAND_CFLAGS		+= -fsanitize=address -O3
else
	COMMAND_CFLAGS		+= -O3
endif
ifeq ($(TESTINGE),1)
	COMMAND_CDEFINES	+= -DTESTING
else
	COMMAND_CFLAGS		+=-Wall -Werror -Wextra
endif
ifeq ($(GLFW_USE),1)
	COMMAND_CDEFINES	+= -DYGLFW3
endif
ifeq ($(RELEASE_USE),1)
	COMMAND_CDEFINES	+= -D_RELEASE -DRELEASE -DYURELEASE=1
else
	COMMAND_CDEFINES	+= -D_DEBUG -DDEBUG -DYUDEBUG
endif

NAME			=yuseong
BUILD_DIR		=build
OBJ_DIR			=$(BUILD_DIR)/obj
OBJ				=obj
ECHO_E			=echo
JASB_NAME		=jasb
JASB_OUT		=jasb
JASB_FILE		=jasb.c
JASB_CMD		=*.o.json
CCJSON			=compile_commands.json
GLSLC			=glslc
CC				=clang
CLINKER			=clang
CFLAGS			=$(COMMAND_CFLAGS)
CFLAGS			+= -std=c23
CFLAGS			+= -fno-inline -fno-omit-frame-pointer
CFLAGS			+= -Wno-missing-field-initializers -Wno-unused-but-set-variable
CDEFINES		=$(COMMAND_CDEFINES)
CPP				=clang++
CPPFLAGS		=-Wno-format
CPPDEFINES		=

INCLUDE_DIRS	=-Isrc -Isrc/core
LIB_PATH		=
LIBS			=

SRC_DIR			=src
SHADER_DIR		=$(SRC_DIR)/shaders
CORE_DIR		=$(SRC_DIR)/core
RENDERER_DIR	=$(SRC_DIR)/renderer
WIN32_DIR		=$(SRC_DIR)/win32
LINUX_DIR		=$(SRC_DIR)/linux
APPLE_DIR		=$(SRC_DIR)/apple
VULKAN_DIR		=$(RENDERER_DIR)/vulkan
OPENGL_DIR		=$(RENDERER_DIR)/opengl
DIRECTX_DIR		=$(RENDERER_DIR)/directx
METAL_DIR		=$(RENDERER_DIR)/metal
ROOT			=$(shell $(MYFIND) $(SRC_DIR) -maxdepth 1 -type f -name '*.c')
ROOT_OBJS		=$(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(ROOT))
CORE_FILES		=$(shell $(MYFIND) $(CORE_DIR) -type f -name '*.c')
CORE_OBJS		=$(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(CORE_FILES))
RENDERER_FILES	=$(shell $(MYFIND) $(RENDERER_DIR) -maxdepth 1 -type f -name '*.c')
RENDERER_OBJS	=$(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(RENDERER_FILES))
WIN32_FILES		=$(shell $(MYFIND) $(WIN32_DIR) -type f -name '*.c')
WIN32_OBJS		=$(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(WIN32_FILES))
LINUX_FILES		=$(shell $(MYFIND) $(LINUX_DIR) -type f -name '*.c')
LINUX_OBJS		=$(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(LINUX_FILES))
VULKAN_FILES	=$(shell $(MYFIND) $(VULKAN_DIR) -type f -name '*.c')
VULKAN_OBJS		=$(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(VULKAN_FILES))
OPENGL_FILES	=$(shell $(MYFIND) $(OPENGL_DIR) -type f -name '*.c')
OPENGL_OBJS		=$(patsubst $(OPENGL_DIR)/%.c, $(OBJ_DIR)/%.o, $(OPENGL_FILES))
DIRECTX_FILES	=$(shell $(MYFIND) $(DIRECTX_DIR) -type f -name '*.c')
DIRECTX_OBJS	=$(patsubst $(DIRECTX_DIR)/%.c, $(OBJ_DIR)/%.o, $(DIRECTX_FILES))
METAL_FILES		=$(shell $(MYFIND) $(METAL_DIR) -type f -name '*.c')
METAL_OBJS		=$(patsubst $(METAL_DIR)/%.c, $(OBJ_DIR)/%.o, $(METAL_FILES))
SHADER_FILES	=$(shell $(MYFIND) $(SRC_DIR) -type f -name '*.comp')
SHADER_OBJS		=$(patsubst $(SRC_DIR)/%.comp, $(OBJ_DIR)/%.comp.spv, $(SHADER_FILES))

C_OBJS			=$(ROOT_OBJS) $(CORE_OBJS) $(RENDERER_OBJS)

ifdef TRACY_USE
	CDEFINES	+= -DTRACY_ENABLE
	CPP_FILES	+= $(SRC_DIR)/TracyClient.cpp
	CPP_OBJS	+= $(OBJ_DIR)/TracyClient.o
	CPPDEFINES	+= -DTRACY_ENABLE
	CPPFLAGS	+= -stdlib=libc++
endif

ifdef CPP_USE
	CPP_FILES	+= $(wildcard $(SRC_DIR)/*.cpp)
	CPP_OBJS	+= $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(CPP_FILES))
endif

color_link		=$(ECHO_E) "$(PURPLE)$(CC)$(NC) $(CFLAGS) -o $(YELLOW)$@$(NC) $(BLUE)$^$(NC) $(LIB_PATH) $(LIBS)"
cpp_compile		=$(ECHO_E) "$(PURPLE)$(CPP)$(NC) -c $(YELLOW)$<$(NC) -o $(BLUE)$@$(NC) $(CPPFLAGS) $(INCLUDE_DIRS)"
c_compile		=$(ECHO_E) "$(PURPLE)$(CC)$(NC) -c $(YELLOW)$<$(NC) -o $(BLUE)$@$(NC) $(CFLAGS) $(INCLUDE_DIRS)"

include $(FILE)$(OS_EXT)

CFLAGS			+= $(CDEFINES)
CPPFLAGS		+= $(CPPDEFINES)

MJJSON			=-MJ$@.json

#*************************** ALL ***************************************#

all: $(OBJ_DIR) $(BUILD_DIR)/$(OUTPUT) $(CCJSON) $(SHADER_OBJS)

#*************************** BUILDER ***********************************#

$(BUILD_DIR)/$(JASB_OUT): $(JASB_FILE)
	@$(CC) $(JASBFLAGS) -o $@ $^ $(JASB_LIB)

$(BUILD_DIR)/$(OBJ):
	@mkdir -p build/obj

#*************************** SHADERS ***********************************#

$(OBJ_DIR)/%.comp.spv: $(SRC_DIR)/%.comp
	@$(ECHO_E) "$(PURPLE)Compiling shaders..$(NC)"
	@mkdir -p $(dir $@)
	@$(GLSLC) $< -o $@

#*************************** LINK_OBJS *********************************#

$(BUILD_DIR)/$(OUTPUT): $(C_OBJS) $(CPP_OBJS)
	@$(color_link)
	@$(CC) $(CFLAGS) -o $@ $^ $(INCLUDE_DIRS) $(LIB_PATH) $(LIBS)

#*************************** COMPILE_FILES *****************************#

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	@$(cpp_compile)
	@$(CPP) $(CPPFLAGS) $(MJJSON) -c $< -o $@ $(INCLUDE_DIRS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	@$(c_compile)
	@$(CC) $(CFLAGS) $(MJJSON) -c $< -o $@ $(INCLUDE_DIRS)

#*************************** COMPILE_JSON ******************************#

$(CCJSON): $(OBJS) $(BUILD_DIR)/$(JASB_OUT)
	@$(ECHO_E) "$(PURPLE)Updating compile_commands.json..$(NC)"
	@$(BUILD_DIR)/$(JASB_OUT) $(JASB_CMD)

#*************************** CLEAN *************************************#

clean:
	@echo "$(RED)Deleting files..$(NC)"
	@rm -f $(BUILD_DIR)/$(OUTPUT)
	@rm -rf $(OBJ_DIR)
	@$(RM_EXTRA)
	@$(RM_EXTRA2)

fclean: clean
	rm -f compile_commands.json

fc: clean

re: clean all

re_fast: clean
	@make --no-print-directory -f $(FILE) -j24 all

.PHONY: all re clean fclean fc re_fast
