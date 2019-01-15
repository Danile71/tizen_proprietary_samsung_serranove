/*
 *  DA probe
 *
 * Copyright (c) 2000 - 2013 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 *
 * Jaewon Lim <jaewon81.lim@samsung.com>
 * Woojin Jung <woojin2.jung@samsung.com>
 * Juyoung Kim <j0.kim@samsung.com>
 * Anastasia Lyupa <a.lyupa@samsung.com>
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; either version 2.1 of the License, or (at your option)
 * any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * Contributors:
 * - S-Core Co., Ltd
 * - Samsung RnD Institute Russia
 *
 */

#ifndef __DAPROBE_H__
#define __DAPROBE_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <dlfcn.h>
#include <stdint.h>
#include "probeinfo.h"
#include "api_names_global.h"

#ifdef __cplusplus
extern "C"{
#endif

#define GET_VALUE_2(x,y) x##y
#define GET_VALUE_1(x,y) GET_VALUE_2(x, y)
#define PROBE_NAME(func) GET_VALUE_1(PROBE_NAME_PREFIX, func)

#ifndef likely
#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)
#endif
#define __unused __attribute__((unused))

#define NUM_OF_MONITOR		3
#define DA_LOG_MAX			4096
#define PERROR_MSG_MAX		128

#define DEFAULT_TOKEN	"`,"

unsigned long get_caller_addr(unsigned long caller_addr);
unsigned char get_call_type(unsigned char call_type);
void write_msg(unsigned long msg_buf, size_t len, unsigned long call_type_p,
	       unsigned long caller_p, unsigned long caller_address);

#define CALLER_ADDRESS	\
	((void*) get_caller_addr(0xbadbeef))

#define CALL_TYPE       \
	((uint32_t) get_call_type(0xff))

#define WRITE_MSG(msg_buf, len, call_type_p, caller_p, caller_address)		\
	do {									\
		write_msg((unsigned long)msg_buf, (size_t)len,			\
			  (unsigned long)call_type_p, (unsigned long)caller_p,	\
			  (unsigned long)caller_address);			\
	} while(0)

#define BACKTRACE_SYMBOLS cached_backtrace_symbols

typedef enum
{
	MT_MEMORY,
	MT_FILE,
	MT_SOCKET,
} MonitorType;

enum BlockLocation
{
	BL_PRE,
	BL_POST,
};

typedef struct
{
	int type;
	int length;
	char data[DA_LOG_MAX];
} log_t;

typedef struct
{
	int				eventIndex;
	int				pID;
	int				tID;
	unsigned long	currentTime;
	unsigned int	callDepth;
} probeInfo_t;

extern __thread int	gProbeDepth;

#define probingStart()		(gProbeDepth++)
#define probingEnd()		(gProbeDepth--)

/***************************************************************************
 * helper apis
 ***************************************************************************/

int preBlockBegin(void);
void postBlockEnd();

unsigned int getCurrentEventIndex();
unsigned long getCurrentTime();
uint64_t get_current_nsec(void);
bool setProbePoint(probeInfo_t * iProbe);
int update_heap_memory_size(bool isAdd, size_t size);

bool print_log_fmt(int msgType, const char *func_name, int line, ...);
bool print_log_str(int msgType, char *st);
bool printLog(log_t* log, int msgType);

int __appendTypeLog(log_t* log, int nInput, char* token, ...);
int getBacktraceString(log_t* log, int bufsize);

void *rtdl_next(const char *symname);
void *rtld_default(const char *symname);

//char* captureScreenShotX(int* width, int* height);
//void releaseScreenShotX();

//int printSamplingLog(void);

//bool captureScreen(unsigned int logNum);
//void captureProbe();
//int registeScreenChange(int renderID);
//void detectTouchEvent(int touchID);
//int getTouchState();
//int isPossibleCapture();


/***************************************************************************
 * helper macros
 ***************************************************************************/

// ============================ rtdl ===============================

#define rtdl_next_set_once(symbol, sname)		\
	do {						\
		if (symbol == NULL)			\
			symbol = rtdl_next(sname);	\
	} while (0)

#define rtld_default_set_once(symbol, sname)    \
	do {					\
		if (symbol == NULL)		     \
			symbol = rtld_default(sname);       \
	} while (0)

// ========================= print log =====================================
#define PRINTMSG(...)	print_log_fmt(APP_MSG_MSG, __FUNCTION__, __LINE__, __VA_ARGS__)
#define PRINTWRN(...)	print_log_fmt(APP_MSG_WARNING, __FUNCTION__, __LINE__, __VA_ARGS__)
#define PRINTERR(...)	print_log_fmt(APP_MSG_ERROR, __FUNCTION__, __LINE__, __VA_ARGS__)

#define INIT_INFO						\
		info.host_ip = 0;				\
		info.host_port = 0;				\
		info.msg_total_size = 0;			\
		info.msg_pack_size = 0;				\
		info.sock = NULL;				\
		info.msg_buf = (char *)""

typedef struct {
	uint32_t host_port;
	uint32_t host_ip;
	struct sockaddr *sock;

	uint64_t msg_total_size;
	uint32_t msg_pack_size;
	char *msg_buf;

} info_t;

// =========================== declare variables ===========================
// local variable is faster than heap allocated variable
// array variable initialization with declare is expensive than memset
#define DECLARE_ERRNO_VARS		\
	int olderrno = 0;		\
	int newerrno = 0;

// declare variable for standard api (not tizen related api)
#define DECLARE_VARIABLE_STANDARD	\
	probeInfo_t probeInfo;		\
	int blockresult = 0;		\
	DECLARE_ERRNO_VARS;		\
	int __attribute__((unused)) ret = 0

// declare variable for standard api (not tizen related api) without ret
#define DECLARE_VARIABLE_STANDARD_NORET		\
		probeInfo_t	probeInfo; 			\
		int blockresult;				\
		int olderrno, newerrno;			\

// ========================== get function pointer =========================

#define GET_REAL_FUNCP(FUNCNAME, SONAME, FUNCTIONPOINTER)			\
	GET_REAL_FUNCP_STR(#FUNCNAME, SONAME, FUNCTIONPOINTER)

#define GET_REAL_FUNCP_STR(FUNCNAME, SONAME, FUNCTIONPOINTER)			\
		do {														\
			if (!FUNCTIONPOINTER) {									\
				if (lib_handle[SONAME] == NULL) {					\
					lib_handle[SONAME] = dlopen(lib_string[SONAME], RTLD_LAZY);			\
					if (lib_handle[SONAME] == NULL) {				\
						fprintf(stderr, "dlopen failed : %s\n", lib_string[SONAME]);	\
						PRINTERR("dlopen failed : %s\n", lib_string[SONAME]);	\
						exit(0);									\
					} \
				}													\
				FUNCTIONPOINTER = dlsym(lib_handle[SONAME], FUNCNAME);		\
				if (FUNCTIONPOINTER == NULL || dlerror() != NULL) {		\
					fprintf(stderr, "dlsym failed : <" FUNCNAME ">\n");			\
					PRINTERR("dlsym failed : <" FUNCNAME ">\n");			\
					exit(0);										\
				}													\
			}														\
		} while(0)

#define GET_REAL_FUNC(FUNCNAME, SONAME)		\
	GET_REAL_FUNCP(FUNCNAME, SONAME, FUNCNAME##p)

#define GET_REAL_FUNCP_RTLD_NEXT(FUNCNAME, FUNCTIONPOINTER)			\
		do {														\
			if(!FUNCTIONPOINTER) {									\
				FUNCTIONPOINTER = dlsym(RTLD_NEXT, #FUNCNAME);		\
				if(FUNCTIONPOINTER == NULL || dlerror() != NULL)	\
					probe_terminate_with_err("function not found", #FUNCNAME, LIB_NO);		\
			}														\
		} while(0)

#define GET_REAL_FUNC_RTLD_NEXT(FUNCNAME)	\
		GET_REAL_FUNCP_RTLD_NEXT(FUNCNAME, FUNCNAME##p)

#define GET_REAL_FUNCP_RTLD_NEXT_CPP(FUNCNAME, FUNCTIONPOINTER)		\
		do {														\
			if(!FUNCTIONPOINTER) {									\
				void* funcp = dlsym(RTLD_NEXT, #FUNCNAME);			\
				if(funcp == NULL || dlerror() != NULL) {			\
					fprintf(stderr, "dlsym failed <%s>\n", #FUNCNAME);							\
					exit(0);										\
				}													\
				memcpy(&FUNCTIONPOINTER, &funcp, sizeof(void*));	\
			}														\
		} while(0)

#define GET_REAL_FUNC_RTLD_NEXT_CPP(FUNCNAME)	\
		GET_REAL_FUNCP_RTLD_NEXT_CPP(FUNCNAME, FUNCNAME##p)

#define GET_REAL_FUNCP_RTLD_DEFAULT(FUNCNAME, FUNCTIONPOINTER)		\
		do {														\
			if(!FUNCTIONPOINTER) {									\
				FUNCTIONPOINTER = dlsym(RTLD_DEFAULT, #FUNCNAME);	\
				if(FUNCTIONPOINTER == NULL || dlerror() != NULL)	\
					probe_terminate_with_err("function not found", #FUNCNAME, LIB_NO);		\
			}														\
		} while(0)

// ======================= pre block macro ================================

#define PRE_PROBEBLOCK_BEGIN()					\
	if((blockresult = preBlockBegin()) != 0) {	\
		setProbePoint(&probeInfo)

#define PRE_PROBEBLOCK_END()							\
	}									\
	olderrno = errno;							\
	errno = 0

#define PRE_PROBEBLOCK() \
		PRE_PROBEBLOCK_BEGIN(); \
		PRE_PROBEBLOCK_END()

// ===================== unconditional probe block ========================

#define PRE_UNCONDITIONAL_BLOCK_BEGIN()	\
	do {								\
		setProbePoint(&probeInfo)

#define PRE_UNCONDITIONAL_BLOCK_END()	\
	} while(0)

// =========================== post block macro ===========================

#define POST_PROBEBLOCK_BEGIN(LCTYPE, RETTYPE, RETVALUE, INPUTFORMAT, ...)	\
	newerrno = errno;														\
	do {																	\
		PREPARE_LOCAL_BUF(); \
		PACK_COMMON_BEGIN(MSG_PROBE_NETWORK, vAPI_ID, INPUTFORMAT, __VA_ARGS__);\
		PACK_COMMON_END(RETTYPE, RETVALUE, errno, blockresult);

#define POST_PROBEBLOCK_END() 						\
		FLUSH_LOCAL_BUF();						\
		postBlockEnd();								\
	} while(0);												\
	errno = (newerrno != 0) ? newerrno : olderrno

#define POST_PROBEBLOCK_FUNC_START_END()					\
		postBlockEnd();							\
	} while(0);								\
	olderrno = errno;							\
	errno = 0

// ========================= simplify macro ================================

#define BEFORE_ORIGINAL(FUNCNAME, LIBNAME)	\
	DECLARE_VARIABLE_STANDARD;				\
	GET_REAL_FUNC(FUNCNAME, LIBNAME);		\
	PRE_PROBEBLOCK()

#define BEFORE_ORIGINAL_NOFILTER(FUNCNAME, LIBNAME)	\
	DECLARE_VARIABLE_STANDARD;						\
	GET_REAL_FUNC(FUNCNAME, LIBNAME);				\
	PRE_PROBEBLOCK()

#ifdef __cplusplus
}
#endif

#endif
