# windows Makefile


CXX := x86_64-w64-mingw32-g++
CC  := x86_64-w64-mingw32-gcc

TARGET  := manager.exe
OBJ_DIR := build/obj

RAYLIB_LIB := build/libs/raylib/raylib/libraylib.a

LIBS := $(RAYLIB_LIB) \
        -static \
        -lwinmm \
        -lgdi32 \
        -lopengl32 \
        -luser32 \
        -lshell32 \
        -lm \
        -lstdc++ \
        -lgcc \
        -lgcc_eh


# Sources

CXX_SRCS := $(shell find src -name "*.cpp") \
            libs/imgui/imgui.cpp \
            libs/imgui/imgui_draw.cpp \
            libs/imgui/imgui_tables.cpp \
            libs/imgui/imgui_widgets.cpp \
            libs/rlImGui/rlImGui.cpp 

C_SRCS := libs/cJSON/cJSON.c

CXX_OBJS := $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(CXX_SRCS))
C_OBJS   := $(patsubst %.c,$(OBJ_DIR)/%.o,$(C_SRCS))

# Includes

INCLUDES := \
    -Ilibs/raylib/src \
    -Ilibs/imgui \
    -Ilibs/rlImGui \
    -Ilibs/cJSON \
    -Isrc \
    -IOS_Dep

# Flags

CXXFLAGS_COMMON := \
    -std=c++17 \
    -Wall \
    -Wextra \
    $(INCLUDES)

CFLAGS_COMMON := \
    -std=c11 \
    -Wall \
    -Wextra \
    $(INCLUDES)

CXXFLAGS_DEBUG := \
    $(CXXFLAGS_COMMON) \
    -O0 \
    -g \
    -fno-omit-frame-pointer

CXXFLAGS_RELEASE := \
    $(CXXFLAGS_COMMON) \
    -O2 \
    -DNDEBUG \
    -flto \
    -fdata-sections \
    -ffunction-sections \
    -fomit-frame-pointer

CFLAGS_DEBUG := \
    $(CFLAGS_COMMON) \
    -O0 \
    -g \
    -fno-omit-frame-pointer

CFLAGS_RELEASE := \
    $(CFLAGS_COMMON) \
    -O2 \
    -DNDEBUG \
    -flto \
    -fdata-sections \
    -ffunction-sections \
    -fomit-frame-pointer

LDFLAGS_DEBUG :=

LDFLAGS_RELEASE := \
    -flto \
    -Wl,--gc-sections \
    -Wl,--strip-all


# Targets


.PHONY: all debug release clean clean-all raylib

all: release

debug: CXXFLAGS := $(CXXFLAGS_DEBUG)
debug: CFLAGS   := $(CFLAGS_DEBUG)
debug: LDFLAGS  := $(LDFLAGS_DEBUG)
debug: $(TARGET)

release: CXXFLAGS := $(CXXFLAGS_RELEASE)
release: CFLAGS   := $(CFLAGS_RELEASE)
release: LDFLAGS  := $(LDFLAGS_RELEASE)
release: $(TARGET)


# Raylib

$(RAYLIB_LIB):
	@echo "Building Raylib (MinGW cross)"

	@mkdir -p build/libs/raylib

	cmake -S libs/raylib \
	      -B build/libs/raylib \
	      -DCMAKE_SYSTEM_NAME=Windows \
	      -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
	      -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++ \
	      -DCMAKE_RC_COMPILER=x86_64-w64-mingw32-windres \
	      -DCMAKE_BUILD_TYPE=Release \
	      -DBUILD_SHARED_LIBS=OFF

	cmake --build build/libs/raylib -- -j$$(nproc)

raylib: $(RAYLIB_LIB)


$(TARGET): $(RAYLIB_LIB) $(CXX_OBJS) $(C_OBJS)
	@echo "Linking $@"
	$(CXX) $(LDFLAGS) $(CXX_OBJS) $(C_OBJS) -o $@ $(LIBS)

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(@D)
	@echo "Compiling $<"
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(@D)
	@echo "Compiling $<"
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	@echo "Cleaning objects"
	rm -rf $(OBJ_DIR)
	rm -f $(TARGET)

clean-all: clean
	@echo "Cleaning dependencies"
	rm -rf build/libs/raylib