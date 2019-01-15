INSTALLDIR = usr/lib
BIN_INSTALLDIR = /usr/local/bin
HEADER_INSTALLDIR = /usr/local/include/

## Since include directives do not impose additional dependencies, we can make
## Makefile more clear, simply putting all includes we ever need in single
## variable. Keep it alphabetic, please.
DEBUG_FLAGS=					\
#		-DSTDTOFILE				\#

INCLUDE_CPPFLAGS =				\
		-I./include			\
		-I./probe_thread		\
		-I./probe_event				\
		-I/usr/include/appfw		\
		-I/usr/include/dbus-1.0		\
		-I/usr/include/e_dbus-1		\
		-I/usr/include/ecore-1		\
		-I/usr/include/ecore-x-1	\
		-I/usr/include/ecore-input-1	\
		-I/usr/include/ecore-evas-1	\
		-I/usr/include/ecore-file-1	\
		-I/usr/include/ecore-imf-1	\
		-I/usr/include/ecore-con-1	\
		-I/usr/include/edje-1		\
		-I/usr/include/eet-1		\
		-I/usr/include/efreet-1		\
		-I/usr/include/eina-1		\
		-I/usr/include/eina-1/eina	\
		-I/usr/include/elementary-1	\
		-I/usr/include/eldbus-1		\
		-I/usr/include/ethumb-1		\
		-I/usr/include/ethumb-client-1	\
		-I/usr/include/evas-1		\
		-I/usr/include/pixman-1		\
		-I/usr/include/system		\
		-I/usr/include/capi-system-runtime-info	\
		-I/usr/include/vconf		\
		-I/usr/lib/dbus-1.0/include	\
		-I/usr/include/efl-1 		\
		-I/usr/include/eo-1			\

WARN_CFLAGS = -g			\
		-Wall			\
		-funwind-tables		\
		-fomit-frame-pointer	\
		-Xlinker		\
		--no-undefined		\
		-Werror			\
		-Wextra			\
		-O2			\
		-Wwrite-strings		\
		-Wlogical-op		\
		-Wpacked		\
		-Winline		\
		-Wno-psabi		\

## Since linking unneeded libraries bloats output of ldd(1), this variable
## holds search paths and common libraries.

LDFLAGS = -shared   	\
	-lpthread			\
	-lrt				\
	-ldl				\
	-Wl,-z,noexecstack

ASMFLAG = -O0 -g

## FIXME: Ideally, UTILITY_SRCS is sources for probe infrastructure and
## PROBE_SRCS is sources for actual replacement functions.  Unfortunatelly,
## it is not so easy and UTILITY_SRCS do not link alone.

COMMON_SRCS = $(UTILITY_SRCS) $(PROBE_SRCS)
UTILITY_SRCS =				\
	./helper/real_functions.c	\
	./helper/libdaprobe.c		\
	./helper/dahelper.c		\
	./helper/btsym.c		\
	./helper/dacollection.c		\
	./helper/dacapture.c		\
	./helper/daforkexec.c		\
	./helper/damaps.c			\
	./helper/dastdout.c			\
	./custom_chart/da_chart.c	\

PROBE_SRCS =	   				\
	./probe_memory/libdamemalloc.c		\
	./probe_memory/libdamemmanage.c		\
	./probe_socket/libdasocket.c		\
	./probe_event/gesture.c			\
	./probe_event/da_event.c		\
	./probe_event/keytouch.c		\
	./probe_event/orientation.c		\
	./probe_third/libdaemon.c		\
	./probe_thread/libdathread.c		\
	./probe_thread/libdasync.c		\
	./probe_file/da_io_posix.c		\
	./probe_file/da_io_stdc.c		\

DUMMY_SRCS = ./custom_chart/da_chart_dummy.c
CAPI_SRCS = 	$(COMMON_SRCS)			\
		./probe_capi/capi_appfw.c		\
		./probe_ui/capi_capture.c

TIZEN_SRCS =	$(COMMON_SRCS) $(CAPI_SRCS)\
		./helper/addr-tizen.c						\
		./helper/common_probe_init.c				\
		./probe_memory/libdanew.cpp					\
		./probe_graphics/da_evas_gl.c				\
		./probe_graphics/da_gl_api_init.c			\
		./probe_graphics/da_gles20_tizen.cpp			\
		./probe_graphics/da_gles20_native.cpp			\
		./probe_graphics/da_gles20_direct.cpp

ASM_SRC = ./helper/da_call_original.S

PARSE_ELF_LIB_SRC = ./elf_parsing/lib_parse_elf.c
PARSE_ELF_BIN_SRC = ./elf_parsing/parse_elf.c

## Totally brain-dead.
## FIXME: Rewrite this normally with eval.
ASM_OBJ = $(patsubst %.S,%.o, $(ASM_SRC))
CAPI_OBJS = $(patsubst %.c,%.o, $(CAPI_SRCS)) $(ASM_OBJ)
TIZEN_OBJS = $(patsubst %.cpp,%.o, $(patsubst %.c,%.o, $(TIZEN_SRCS))) $(ASM_OBJ)
DUMMY_OBJS = $(patsubst %.c,%.o, $(DUMMY_SRCS))
PARSE_ELF_LIB_OBJ = $(patsubst %.c,%.o, $(PARSE_ELF_LIB_SRC))
PARSE_ELF_BIN_OBJ = $(patsubst %.c,%.o, $(PARSE_ELF_BIN_SRC))

TIZEN_TARGET = da_probe_tizen.so
DUMMY_TARGET = libdaprobe.so
PARSE_ELF_LIB_TARGET = libparserelf.so
PARSE_ELF_BIN_TARGET = parse_elf

CPPFLAGS = $(INCLUDE_CPPFLAGS) -D_GNU_SOURCE -DSELF_LIB_NAME="\"/$(INSTALLDIR)/$(TIZEN_TARGET)\""
CFLAGS = $(WARN_CFLAGS) -fPIC
CXXFLAGS = $(WARN_CFLAGS) -fPIC

TIZEN_CPPFLAGS = -DTIZENAPP $(SWAP_PROBE_DEFS)
TIZEN_LDFLAGS = -lstdc++


all:		elflib capi tizen dummy elfparser
tizen:		headers $(TIZEN_TARGET)
dummy:		headers $(DUMMY_TARGET)
elflib:		$(PARSE_ELF_LIB_OBJ) $(PARSE_ELF_LIB_TARGET)
elfparser:	elflib $(PARSE_ELF_BIN_OBJ) $(PARSE_ELF_BIN_TARGET)

$(ASM_OBJ): $(ASM_SRC)
	$(CC) $(ASMFLAG) -c $^ -o $@

$(PARSE_ELF_LIB_OBJ): $(PARSE_ELF_LIB_SRC)
	$(CC) -fPIC -c $^ -o $@

$(PARSE_ELF_BIN_OBJ): $(PARSE_ELF_BIN_SRC)
	$(CC) -c $^ -o $@

API_NAME_LIST = scripts/api_names_all.txt
GENERATED_HEADERS = include/api_id_mapping.h include/x_define_api_id_list.h
SOURCE_HEADERS = include/api_ld_mapping.h

headers: $(API_NAME_LIST) $(GENERATED_HEADERS)
rmheaders:
	rm -f $(API_NAME_LIST) $(GENERATED_HEADERS) $(SOURCE_HEADERS)

$(API_NAME_LIST):
	if [ -f $@ ]; then rm $@;fi
	cat include/api_names_global.h > $@
	cat */api_names.txt >> $@

$(SOURCE_HEADERS): $(API_NAME_LIST)
$(SOURCE_HEADERS): ./scripts/gen_headers.py
	python $< $(API_NAME_LIST) $(TIZEN_TARGET) $(INSTALLDIR) > $@ >&2
	cat $@

include/api_id_mapping.h: ./scripts/gen_api_id_mapping_header.awk
include/x_define_api_id_list.h: ./scripts/gen_api_id_mapping_list.awk

da_api_map: $(GENERATED_HEADERS)

$(GENERATED_HEADERS):
	awk -f $< < $(API_NAME_LIST) > $@

$(TIZEN_TARGET): LDFLAGS+=$(TIZEN_LDFLAGS)
$(TIZEN_TARGET): CPPFLAGS+=$(TIZEN_CPPFLAGS)
$(TIZEN_TARGET): CPPFLAGS+=$(DEBUG_FLAGS)
$(TIZEN_TARGET): $(TIZEN_OBJS)
	$(CC) $(LDFLAGS) $^ -o $@

$(DUMMY_TARGET): $(DUMMY_OBJS)
	$(CC) $(LDFLAGS) $^ -o $@

$(PARSE_ELF_LIB_TARGET): $(PARSE_ELF_LIB_OBJ)
	$(CC) $^ -shared -o $@

$(PARSE_ELF_BIN_TARGET): $(PARSE_ELF_BIN_OBJ) $(PARSE_ELF_LIB_TARGET)
	$(CC) $^ -o $@ -L. -lparserelf

ldheader:	$(SOURCE_HEADERS)

install: install_da install_ld install_elf

install_da: all
	[ -d "$(DESTDIR)/$(INSTALLDIR)" ] || mkdir -p $(DESTDIR)/$(INSTALLDIR)
	install $(TIZEN_TARGET) $(DUMMY_TARGET) $(DESTDIR)/$(INSTALLDIR)/


install_ld: ldheader # var_addr
	install -m 644 include/ld_preload_probes.h $(DESTDIR)/$(HEADER_INSTALLDIR)/ld_preload_probes.h
	install -m 644 include/ld_preload_types.h $(DESTDIR)/$(HEADER_INSTALLDIR)/ld_preload_types.h
	install -m 644 include/ld_preload_probe_lib.h $(DESTDIR)/$(HEADER_INSTALLDIR)/ld_preload_probe_lib.h
	install -m 644 include/x_define_api_id_list.h $(DESTDIR)/$(HEADER_INSTALLDIR)/x_define_api_id_list.h
	install -m 644 include/app_protocol.h $(DESTDIR)/$(HEADER_INSTALLDIR)/app_protocol.h

install_elf: elflib elfparser
	install $(PARSE_ELF_LIB_TARGET) $(DESTDIR)/$(INSTALLDIR)/libparserelf.so
	install $(PARSE_ELF_BIN_TARGET) $(DESTDIR)/$(BIN_INSTALLDIR)/parse_elf
	install -m 644 elf_parsing/parse_elf.h $(DESTDIR)/$(HEADER_INSTALLDIR)/parse_elf.h


clean:
	rm -f *.so $(TIZEN_OBJS) $(PARSE_ELF_LIB_OBJ) $(PARSE_ELF_BIN_OBJ) $(GENERATED_HEADERS) $(API_NAME_LIST) $(SOURCE_HEADERS)

.PHONY: all capi tizen dummy clean install_ld install_da install_elf install headers
