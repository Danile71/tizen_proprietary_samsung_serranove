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

#ifndef __PROBEINFO_H__
#define __PROBEINFO_H__

#ifdef __cplusplus
extern "C"{
#endif

#include "app_protocol.h"

#define		VT_INT			'd'
#define		VT_UINT			'd'
#define		VT_LONG			'x'
#define		VT_ULONG		'x'
#define		VT_STR			's'
#define		VT_CHAR			'c'
#define		VT_PTR			'p'
#define		VT_NULL			'x'
#define		VT_OFF_T		'd'
#define		VT_SIZE_T		'x'
#define		VT_SSIZE_T		'x'
#define		VT_SOCKLEN_T		'd'
#define		VT_UINT16_T		'd'
#define		VT_UINT32_T		'd'
#define		VT_UINT64_T		'x'
#define		VT_MODE_T		'd'
#define		VT_DEV_T		'd'

/* #define		LC_MEMORY		1 */
/* #define		LC_UICREATE		2 */
/* #define		LC_UIEVENT		3 */
/* #define		LC_USERFUNC		4 */
/* #define		LC_RESOURCE		5 */
/* #define		LC_LIFECYCLE		6 */
/* #define		LC_SNAPSHOT		7 */
/* #define		LC_SCENE		8 */
/* #define		LC_DEVICE		9 */
/* #define		LC_ALLOCMEM		10 */
/* #define		LC_SAMPLE		11 */
/* #define		LC_THREAD		12 */
/* #define		LC_CUSTOM		13 */
/* #define		LC_SYNC			14 */
/* #define		LC_SOCKET		15 */
/* #define		LC_GLES20		16 */


#define		FD_API_OPEN			0
#define		FD_API_CLOSE			1
#define		FD_API_READ_START		2
#define		FD_API_READ_END 		3
#define		FD_API_WRITE_START		4
#define		FD_API_WRITE_END		5
#define		FD_API_DIRECTORY		6
#define		FD_API_PERMISSION		7
#define		FD_API_OTHER			8
#define		FD_API_SEND			9
#define		FD_API_RECEIVE			10
#define		FD_API_OPTION			11
#define		FD_API_MANAGE			12
#define		FD_API_LOCK_START		14
#define		FD_API_LOCK_END			15
#define		FD_API_LOCK_RELEASE		16


#define		SOCKET_API_FD_OPEN		0
#define		SOCKET_API_FD_CLOSE		1
#define		SOCKET_API_RECV_START		2
#define		SOCKET_API_RECV_END		3
#define		SOCKET_API_SEND_START		4
#define		SOCKET_API_SEND_END		5
#define		SOCKET_API_BIND			6
#define		SOCKET_API_LISTEN		7
#define		SOCKET_API_CONNECT		8
#define		SOCKET_API_ACCEPT_START		9
#define		SOCKET_API_ACCEPT_END		10
#define		SOCKET_API_EVENT_START		11
#define		SOCKET_API_EVENT_END		12
#define		SOCKET_API_OTHER		13
#define		HTTP_API_SESSION_CONSTRUCT	14
#define		HTTP_API_SESSION_CLOSE		15
#define		HTTP_API_TRANSACTION_OPEN	16
#define		HTTP_API_TRANSACTION_CLOSE	17
#define		HTTP_API_ALLOCATION		18
#define		HTTP_API_SUBMIT			19
#define		HTTP_API_REQUEST		20
#define		HTTP_API_RESPONSE		21
#define		HTTP_API_OTHER			22
#define		HTTP_API_CLOSE			23


#define		MEMORY_API_ALLOC	0
#define		MEMORY_API_FREE		1
#define		MEMORY_API_MANAGE	2

//#define		SNAPSHOT_API_WIN		0
//#define		SNAPSHOT_API_CONTROLBAR		1
//#define		SNAPSHOT_API_NAVIFRAME		2
//#define		SNAPSHOT_API_PAGER		3

#define 	EVENT_TYPE_DOWN		0
#define 	EVENT_TYPE_UP		1
#define 	EVENT_TYPE_MOVE		2

#define		USERFUNC_ENTER		0
#define		USERFUNC_EXIT		1

#define		THREAD_PTHREAD			0
#define		THREAD_TIZENTHREAD_WORKER		1
#define		THREAD_TIZENTHREAD_EVENTDRIVEN	2

#define THREAD_CLASS_BLANK ""

#define		THREAD_API_NEW				0
#define		THREAD_API_START			1
#define		THREAD_API_STOP				2
#define		THREAD_API_EXIT				3
#define		THREAD_API_WAIT_START			4
#define		THREAD_API_WAIT_END			5
#define		THREAD_API_INTERNAL_START		6
#define		THREAD_API_INTERNAL_STOP		7
#define		THREAD_API_OTHER			8

#define 	SYNC_TIZEN_MUTEX				0
#define		SYNC_TIZEN_MONITOR			1
#define		SYNC_TIZEN_SEMAPHORE			2
#define		SYNC_PTHREAD_MUTEX			3
#define		SYNC_PTHREAD_COND_VARIABLE		4
#define		SYNC_PTHREAD_RWLOCK			5
#define		SYNC_PTHREAD_SPINLOCK			6
#define		SYNC_PTHREAD_BARRIER			7

#define		SYNC_API_NEW				0
#define		SYNC_API_ACQUIRE_WAIT_START		1
#define		SYNC_API_ACQUIRE_WAIT_END		2
#define		SYNC_API_RELEASE			3
#define		SYNC_API_TRY_ACQUIRE			4
#define		SYNC_API_COND_WAIT_START		5
#define		SYNC_API_COND_WAIT_END			6
#define		SYNC_API_NOTIFY				7
#define		SYNC_API_NOTIFY_ALL			8
#define		SYNC_API_OTHER				9

enum DaOptions
{
	FL_MEMORY_ALLOC_PROBING		= 0x000000000008ULL, // memory allocation API (glibc)
	FL_FILE_API_PROBING		= 0x000000000010ULL, // file API (glibc, OSP)
	FL_THREAD_API_PROBING		= 0x000000000020ULL, // thread API (glibc, OSP)
	FL_OSP_UI_API_PROBING		= 0x000000000040ULL, // UI API (OSP)
	FL_SCREENSHOT			= 0x000000000080ULL, // Screenshot
	FL_USER_EVENT			= 0x000000000100ULL, // events of Touch, Gesture, Orientation, Key
	FL_RECORDING			= 0x000000000200ULL, // recording the user event
	FL_NETWORK_API_PROBING		= 0x000000020000ULL, // network API (glibc, OSP, libsoap, openssl)
	FL_OPENGL_API_PROBING		= 0x000000040000ULL, // openGL API

	FL_MEMORY_ALLOC_ALWAYS_PROBING	= 0x000010000000ULL, // all (include external) memory allocation API (glibc) always
	FL_FILE_API_ALWAYS_PROBING	= 0x000020000000ULL, // all (include external) file API (glibc, OSP) always
	FL_THREAD_API_ALWAYS_PROBING	= 0x000040000000ULL, // all (include external) thread API (glibc, OSP) always
	FL_OSP_UI_API_ALWAYS_PROBING	= 0x000080000000ULL, // all (include external) UI API (OSP) always
	FL_NETWORK_API_ALWAYS_PROBING	= 0x000100000000ULL, // all (include external) network API (glibc, OSP, libsoap, openssl) always
	FL_OPENGL_API_ALWAYS_PROBING	= 0x000200000000ULL, // all (include external) openGL API always

/* TODO replace all "OPT_*" in code by "FL_*" like in swap-manager */
	OPT_ALWAYSOFF		= 0x000000000,
	OPT_ALLOC		= FL_MEMORY_ALLOC_PROBING,
	OPT_FILE		= FL_FILE_API_PROBING,
	OPT_THREAD		= FL_THREAD_API_PROBING,
	OPT_UI			= FL_OSP_UI_API_PROBING,
	OPT_SNAPSHOT		= FL_SCREENSHOT,
	OPT_EVENT		= FL_USER_EVENT,
	OPT_RECORD		= FL_RECORDING,
	OPT_NETWORK		= FL_NETWORK_API_PROBING,
	OPT_GLES		= FL_OPENGL_API_PROBING,
	OPT_ALWAYSON		= 0xfffffffff,
	OPT_ALLOC_ALWAYS	= FL_MEMORY_ALLOC_ALWAYS_PROBING,
	OPT_FILE_ALWAYS		= FL_FILE_API_ALWAYS_PROBING,
	OPT_THREAD_ALWAYS	= FL_THREAD_API_ALWAYS_PROBING,
	OPT_UI_ALWAYS		= FL_OSP_UI_API_ALWAYS_PROBING,
	OPT_NETWORK_ALWAYS	= FL_NETWORK_API_ALWAYS_PROBING,
	OPT_GLES_ALWAYS		= FL_OPENGL_API_ALWAYS_PROBING
};


#ifdef __cplusplus
}
#endif

#endif
