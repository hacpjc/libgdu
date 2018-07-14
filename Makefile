
DIR_PACK := .pack
DIR_PACK_LIB := $(DIR_PACK)/lib

include Makefile.mk

libgdu-shared := libgdu.so
libgdu-static := libgdu.a

CFLAGS := -I$(CURDIR) -fPIC
CFLAGS += -g
 
LDFLAGS := -shared
LDFLAGS += 

.PHONY: default
default: all

.PHONY: all
all: $(libgdu-static) $(libgdu-shared)

$(libgdu-static): $(obj-y)
	@mkdir -vp $(DIR_PACK_LIB)
	ar -rcs $(DIR_PACK_LIB)/$@ $(obj-y)

$(libgdu-shared): $(obj-y)
	@mkdir -vp $(DIR_PACK_LIB)
	$(CC) $(LDFLAGS) -Wl,-soname,$@ -o $(DIR_PACK_LIB)/$@ $<

$(obj-y):
	$(CC) -c $(patsubst %.o,%.c,$@) -o $@ $(CFLAGS)

.PHONY: clean
clean:
	-@rm -vf $(obj-y)
	-@rm -rvf $(DIR_PACK)

#;