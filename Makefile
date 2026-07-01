CONFIG   ?= Debug
PLATFORM := x64

CXX     := g++
LD      := g++
OBJCOPY := objcopy

CXXFLAGS := \
  -m64 -std=c++20 -masm=intel \
  -ffreestanding -fno-rtti -fno-exceptions \
  -fno-zero-initialized-in-bss -fno-common \
  -mno-sse -mno-sse2 -mno-mmx -mno-3dnow -mno-80387 \
  -msoft-float -mno-red-zone -g

ifeq ($(CONFIG),Release)
  CXXFLAGS += -O2
else
  CXXFLAGS += -O0
endif

LIBC     := ../mylibc/bin/$(PLATFORM)/$(CONFIG)/libc.a
INCLUDES := -I../mylibc/include
LDFLAGS  := -nostdlib -T linker.ld

ELFDIR   := bin/$(PLATFORM)/$(CONFIG)
OBJDIR   := $(ELFDIR)/obj
BOOTDIR  := ../_bootpartition/$(PLATFORM)/$(CONFIG)

SRCS  := $(wildcard src/*.cpp)
APPS  := $(patsubst src/%.cpp,%,$(SRCS))
BOOTS := $(addprefix $(BOOTDIR)/,$(addsuffix .o,$(APPS)))

.PHONY: all clean

all: $(BOOTS)

$(BOOTDIR)/%.o: $(ELFDIR)/%.efi
	@mkdir -p $(BOOTDIR)
	$(OBJCOPY) -O binary \
	  --only-section=.text --only-section=.rodata --only-section=.data \
	  $< $@
	@echo "[mytools] $(@F) done"

$(ELFDIR)/%.efi: $(OBJDIR)/%.o $(LIBC)
	@mkdir -p $(ELFDIR)
	$(LD) $(LDFLAGS) -o $@ $< $(LIBC)

$(OBJDIR)/%.o: src/%.cpp
	@mkdir -p $(OBJDIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf bin
