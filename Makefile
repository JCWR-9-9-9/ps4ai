# PS4 Snake Game Makefile
# Uses OpenOrbis PS4 Toolchain natively

# Project metadata
TITLE       := Snake Game
VERSION     := 1.00
TITLE_ID    := BREW00001
CONTENT_ID  := IV0000-BREW00001_00-SNAKEGAME0000000

# Toolchain path
TOOLCHAIN   ?= $(OO_PS4_TOOLCHAIN)
ifeq ($(TOOLCHAIN),)
TOOLCHAIN   := /workspace/PS4Toolchain
endif

# Directories
INTDIR      := build

# Tools
CCX         := $(TOOLCHAIN)/bin/linux/openorbis-clang++
LD          := $(TOOLCHAIN)/bin/linux/ld.lld
PKGTOOL     := $(TOOLCHAIN)/bin/linux/PkgTool.Core
CREATEFSELF := $(TOOLCHAIN)/bin/linux/create-fself
CREATEGP4   := $(TOOLCHAIN)/bin/linux/create-gp4

# Compiler flags
CFLAGS      := --target=x86_64-pc-freebsd12-elf -fPIC -funwind-tables -c \
               -isysroot $(TOOLCHAIN) \
               -isystem $(TOOLCHAIN)/include \
               -isystem /usr/include \
               -isystem /usr/include/x86_64-linux-gnu

CXXFLAGS    := $(CFLAGS) \
               -isystem $(TOOLCHAIN)/include/c++/v1 \
               -DSDL_DISABLE_MMINTRIN_H \
               -DSDL_DISABLE_XMMINTRIN_H \
               -DSDL_DISABLE_EMMINTRIN_H \
               -DSDL_DISABLE_PMMINTRIN_H \
               -DSDL_DISABLE_IMMINTRIN_H \
               -DSDL_DISABLE_3DNOW_NOW \
               -DSDL_DISABLE_3DNOW \
               -DSDL_DISABLE_MMINTRIN_H \
               -DSDL_DISABLE_XMMINTRIN_H \
               -DSDL_DISABLE_EMMINTRIN_H \
               -DSDL_DISABLE_PMMINTRIN_H \
               -DSDL_DISABLE_IMMINTRIN_H \
               -DSDL_DISABLE_3DNOW_NOW \
               -DSDL_DISABLE_3DNOW

# Linker flags
LDFLAGS     := -m elf_x86_64 -pie \
               --script $(TOOLCHAIN)/link.x \
               --eh-frame-hdr \
               -z undefs \
               -L$(TOOLCHAIN)/lib \
               -lSDL2 -lkernel -lm -lpthread -lc++ -lc \
               $(TOOLCHAIN)/lib/crt1.o

# Environment for tools
export LD_LIBRARY_PATH := /workspace/clang-local/usr/lib/x86_64-linux-gnu:/workspace/ssl-local/usr/lib/x86_64-linux-gnu:$(LD_LIBRARY_PATH)
export OO_PS4_TOOLCHAIN := $(TOOLCHAIN)
export DOTNET_SYSTEM_GLOBALIZATION_INVARIANT := 1

# Object files
OBJS := $(INTDIR)/main.o $(INTDIR)/ps4_stubs.o

# SCE module directory (required by the PKG; holds system PRX modules).
SCE_MODULE_DIR := sce_module
SCE_MODULE_PRX := $(SCE_MODULE_DIR)/libc.prx $(SCE_MODULE_DIR)/libSceFios2.prx

# Copy required system modules from the toolchain into sce_module/.
$(SCE_MODULE_DIR)/libc.prx: | $(TOOLCHAIN)/src/modules/libc.prx
	@mkdir -p $(SCE_MODULE_DIR)
	@echo "Copying libc.prx into $(SCE_MODULE_DIR)..."
	@cp $(TOOLCHAIN)/src/modules/libc.prx $@

$(SCE_MODULE_DIR)/libSceFios2.prx: | $(TOOLCHAIN)/src/modules/libSceFios2.prx
	@mkdir -p $(SCE_MODULE_DIR)
	@echo "Copying libSceFios2.prx into $(SCE_MODULE_DIR)..."
	@cp $(TOOLCHAIN)/src/modules/libSceFios2.prx $@

.PHONY: all clean pkg

all: $(CONTENT_ID).pkg

pkg: $(CONTENT_ID).pkg

$(CONTENT_ID).pkg: pkg.gp4
	@echo "Building PKG..."
	$(PKGTOOL) pkg_build $< .

pkg.gp4: eboot.bin sce_sys/param.sfo sce_sys/icon0.png $(SCE_MODULE_PRX)
	@echo "Creating GP4 project..."
	$(CREATEGP4) -out $@ --content-id=$(CONTENT_ID) --files "$^"

sce_sys/param.sfo: Makefile
	@mkdir -p sce_sys
	@echo "Creating SFO..."
	$(PKGTOOL) sfo_new $@
	$(PKGTOOL) sfo_setentry $@ APP_TYPE --type Integer --maxsize 4 --value 1
	$(PKGTOOL) sfo_setentry $@ APP_VER --type Utf8 --maxsize 8 --value '$(VERSION)'
	$(PKGTOOL) sfo_setentry $@ ATTRIBUTE --type Integer --maxsize 4 --value 0
	$(PKGTOOL) sfo_setentry $@ CATEGORY --type Utf8 --maxsize 4 --value 'gd'
	$(PKGTOOL) sfo_setentry $@ CONTENT_ID --type Utf8 --maxsize 48 --value '$(CONTENT_ID)'
	$(PKGTOOL) sfo_setentry $@ DOWNLOAD_DATA_SIZE --type Integer --maxsize 4 --value 0
	$(PKGTOOL) sfo_setentry $@ SYSTEM_VER --type Integer --maxsize 4 --value 0
	$(PKGTOOL) sfo_setentry $@ TITLE --type Utf8 --maxsize 128 --value '$(TITLE)'
	$(PKGTOOL) sfo_setentry $@ TITLE_ID --type Utf8 --maxsize 12 --value '$(TITLE_ID)'
	$(PKGTOOL) sfo_setentry $@ VERSION --type Utf8 --maxsize 8 --value '$(VERSION)'

sce_sys/icon0.png: icon0.png
	@mkdir -p sce_sys
	cp $< $@

eboot.bin: $(INTDIR) $(OBJS)
	@echo "Linking ELF..."
	$(LD) $(INTDIR)/*.o -o $(INTDIR)/snake.elf $(LDFLAGS)
	@echo "Creating EBoot..."
	$(CREATEFSELF) -in=$(INTDIR)/snake.elf -out=$(INTDIR)/snake.oelf --eboot "eboot.bin" --paid 0x3800000000000011

$(INTDIR)/%.o: %.cpp
	@echo "Compiling $<..."
	$(CCX) $(CXXFLAGS) -o $@ $<

$(INTDIR)/%.o: %.c
	@echo "Compiling $<..."
	$(TOOLCHAIN)/bin/linux/openorbis-clang $(CFLAGS) -o $@ $<

$(INTDIR):
	mkdir -p $(INTDIR)

clean:
	rm -rf $(CONTENT_ID).pkg pkg.gp4 sce_sys eboot.bin $(INTDIR) $(SCE_MODULE_DIR)