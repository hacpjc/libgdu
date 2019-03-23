
DIR_PACK := .pack
DIR_PACK_LIB := $(DIR_PACK)/lib

include Makefile.mk

liblgu-shared := liblgu.so
liblgu-static := liblgu.a

CFLAGS := -I$(CURDIR) -fPIC
CFLAGS += -g
CFLAGS += -Wall
 
LDFLAGS := -shared
LDFLAGS += 

.PHONY: default
default: all

.PHONY: all
all: $(liblgu-static) $(liblgu-shared)

$(liblgu-static): $(obj-y)
	@mkdir -vp $(DIR_PACK_LIB)
	ar -rcs $(DIR_PACK_LIB)/$@ $(obj-y)

$(liblgu-shared): $(obj-y)
	@mkdir -vp $(DIR_PACK_LIB)
	$(CC) $(LDFLAGS) -Wl,-soname,$@ -o $(DIR_PACK_LIB)/$@ $<

$(obj-y):
	$(CC) -c $(patsubst %.o,%.c,$@) -o $@ $(CFLAGS)

.PHONY: clean
clean:
	-@rm -vf $(obj-y)
	-@rm -rvf $(DIR_PACK)

.PHONY: distclean
distclean: clean

#;
