/*
 * Copyright (C) 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2007-2009 Torch Mobile, Inc.
 * Copyright (C) 2010, 2011 Research In Motion Limited. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WTF_Platform_h
#define WTF_Platform_h

/* Include compiler specific macros */
#include <wtf/Compiler.h>

/* ==== PLATFORM handles OS, operating environment, graphics API, and
   CPU. This macro will be phased out in favor of platform adaptation
   macros, policy decision macros, and top-level port definitions. ==== */
#define PLATFORM(WTF_FEATURE) (defined WTF_PLATFORM_##WTF_FEATURE  && WTF_PLATFORM_##WTF_FEATURE)


/* ==== Platform adaptation macros: these describe properties of the target environment. ==== */

/* CPU() - the target CPU architecture */
#define CPU(WTF_FEATURE) (defined WTF_CPU_##WTF_FEATURE  && WTF_CPU_##WTF_FEATURE)
/* HAVE() - specific system features (headers, functions or similar) that are present or not */
#define HAVE(WTF_FEATURE) (defined HAVE_##WTF_FEATURE  && HAVE_##WTF_FEATURE)
/* OS() - underlying operating system; only to be used for mandated low-level services like
   virtual memory, not to choose a GUI toolkit */
#define OS(WTF_FEATURE) (defined WTF_OS_##WTF_FEATURE  && WTF_OS_##WTF_FEATURE)


/* ==== Policy decision macros: these define policy choices for a particular port. ==== */

/* USE() - use a particular third-party library or optional OS service */
#define USE(WTF_FEATURE) (defined WTF_USE_##WTF_FEATURE  && WTF_USE_##WTF_FEATURE)
/* ENABLE() - turn on a specific feature of WebKit */
#define ENABLE(WTF_FEATURE) (defined ENABLE_##WTF_FEATURE  && ENABLE_##WTF_FEATURE)

/* ==== CPU() - the target CPU architecture ==== */

/* This also defines CPU(BIG_ENDIAN) or CPU(MIDDLE_ENDIAN) or neither, as appropriate. */

/* CPU(ALPHA) - DEC Alpha */
#if defined(__alpha__)
#define WTF_CPU_ALPHA 1
#endif

/* CPU(IA64) - Itanium / IA-64 */
#if defined(__ia64__)
#define WTF_CPU_IA64 1
/* 32-bit mode on Itanium */
#if !defined(__LP64__)
#define WTF_CPU_IA64_32 1
#endif
#endif

/* CPU(MIPS) - MIPS 32-bit */
/* Note: Only O32 ABI is tested, so we enable it for O32 ABI for now.  */
#if (defined(mips) || defined(__mips__) || defined(MIPS) || defined(_MIPS_)) \
    && defined(_ABIO32)
#define WTF_CPU_MIPS 1
#if defined(__MIPSEB__)
#define WTF_CPU_BIG_ENDIAN 1
#endif
#define WTF_MIPS_PIC (defined __PIC__)
#define WTF_MIPS_ARCH __mips
#define WTF_MIPS_ISA(v) (defined WTF_MIPS_ARCH && WTF_MIPS_ARCH == v)
#define WTF_MIPS_ISA_AT_LEAST(v) (defined WTF_MIPS_ARCH && WTF_MIPS_ARCH >= v)
#define WTF_MIPS_ARCH_REV __mips_isa_rev
#define WTF_MIPS_ISA_REV(v) (defined WTF_MIPS_ARCH_REV && WTF_MIPS_ARCH_REV == v)
#define WTF_MIPS_DOUBLE_FLOAT (defined __mips_hard_float && !defined __mips_single_float)
#define WTF_MIPS_FP64 (defined __mips_fpr && __mips_fpr == 64)
/* MIPS requires allocators to use aligned memory */
#define WTF_USE_ARENA_ALLOC_ALIGNMENT_INTEGER 1
#endif /* MIPS */

/* CPU(PPC) - PowerPC 32-bit */
#if   defined(__ppc__)     \
    || defined(__PPC__)     \
    || defined(__powerpc__) \
    || defined(__powerpc)   \
    || defined(__POWERPC__) \
    || defined(_M_PPC)      \
    || defined(__PPC)
#define WTF_CPU_PPC 1
#define WTF_CPU_BIG_ENDIAN 1
#endif

/* CPU(PPC64) - PowerPC 64-bit */
#if   defined(__ppc64__) \
    || defined(__PPC64__)
#define WTF_CPU_PPC64 1
#define WTF_CPU_BIG_ENDIAN 1
#endif

/* CPU(SH4) - SuperH SH-4 */
#if defined(__SH4__)
#define WTF_CPU_SH4 1
#endif

/* CPU(SPARC32) - SPARC 32-bit */
#if defined(__sparc) && !defined(__arch64__) || defined(__sparcv8)
#define WTF_CPU_SPARC32 1
#define WTF_CPU_BIG_ENDIAN 1
#endif

/* CPU(SPARC64) - SPARC 64-bit */
#if defined(__sparc__) && defined(__arch64__) || defined (__sparcv9)
#define WTF_CPU_SPARC64 1
#define WTF_CPU_BIG_ENDIAN 1
#endif

/* CPU(SPARC) - any SPARC, true for CPU(SPARC32) and CPU(SPARC64) */
#if CPU(SPARC32) || CPU(SPARC64)
#define WTF_CPU_SPARC 1
#endif

/* CPU(S390X) - S390 64-bit */
#if defined(__s390x__)
#define WTF_CPU_S390X 1
#define WTF_CPU_BIG_ENDIAN 1
#endif

/* CPU(S390) - S390 32-bit */
#if defined(__s390__)
#define WTF_CPU_S390 1
#define WTF_CPU_BIG_ENDIAN 1
#endif

/* CPU(X86) - i386 / x86 32-bit */
#if   defined(__i386__) \
    || defined(i386)     \
    || defined(_M_IX86)  \
    || defined(_X86_)    \
    || defined(__THW_INTEL)
#define WTF_CPU_X86 1
#endif

/* CPU(X86_64) - AMD64 / Intel64 / x86_64 64-bit */
#if   defined(__x86_64__) \
    || defined(_M_X64)
#define WTF_CPU_X86_64 1
#endif

/* CPU(ARM) - ARM, any version*/
#if   defined(arm) \
    || defined(__arm__) \
    || defined(ARM) \
    || defined(_ARM_)
#define WTF_CPU_ARM 1

#if defined(__ARMEB__) || (COMPILER(RVCT) && defined(__BIG_ENDIAN))
#define WTF_CPU_BIG_ENDIAN 1

#elif !defined(__ARM_EABI__) \
    && !defined(__EABI__) \
    && !defined(__VFP_FP__) \
    && !defined(_WIN32_WCE) \
    && !defined(ANDROID)
#define WTF_CPU_MIDDLE_ENDIAN 1

#endif

#define WTF_ARM_ARCH_AT_LEAST(N) (CPU(ARM) && WTF_ARM_ARCH_VERSION >= N)

/* Set WTF_ARM_ARCH_VERSION */
#if   defined(__ARM_ARCH_4__) \
    || defined(__ARM_ARCH_4T__) \
    || defined(__MARM_ARMV4__) \
    || defined(_ARMV4I_)
#define WTF_ARM_ARCH_VERSION 4

#elif defined(__ARM_ARCH_5__) \
    || defined(__ARM_ARCH_5T__) \
    || defined(__MARM_ARMV5__)
#define WTF_ARM_ARCH_VERSION 5

#elif defined(__ARM_ARCH_5E__) \
    || defined(__ARM_ARCH_5TE__) \
    || defined(__ARM_ARCH_5TEJ__)
#define WTF_ARM_ARCH_VERSION 5
/*ARMv5TE requires allocators to use aligned memory*/
#define WTF_USE_ARENA_ALLOC_ALIGNMENT_INTEGER 1

#elif defined(__ARM_ARCH_6__) \
    || defined(__ARM_ARCH_6J__) \
    || defined(__ARM_ARCH_6K__) \
    || defined(__ARM_ARCH_6Z__) \
    || defined(__ARM_ARCH_6ZK__) \
    || defined(__ARM_ARCH_6T2__) \
    || defined(__ARMV6__)
#define WTF_ARM_ARCH_VERSION 6

#elif defined(__ARM_ARCH_7A__) \
    || defined(__ARM_ARCH_7R__)
#define WTF_ARM_ARCH_VERSION 7

/* RVCT sets _TARGET_ARCH_ARM */
#elif defined(__TARGET_ARCH_ARM)
#define WTF_ARM_ARCH_VERSION __TARGET_ARCH_ARM

#if defined(__TARGET_ARCH_5E) \
    || defined(__TARGET_ARCH_5TE) \
    || defined(__TARGET_ARCH_5TEJ)
/*ARMv5TE requires allocators to use aligned memory*/
#define WTF_USE_ARENA_ALLOC_ALIGNMENT_INTEGER 1
#endif

#else
#define WTF_ARM_ARCH_VERSION 0

#endif

/* Set WTF_THUMB_ARCH_VERSION */
#if   defined(__ARM_ARCH_4T__)
#define WTF_THUMB_ARCH_VERSION 1

#elif defined(__ARM_ARCH_5T__) \
    || defined(__ARM_ARCH_5TE__) \
    || defined(__ARM_ARCH_5TEJ__)
#define WTF_THUMB_ARCH_VERSION 2

#elif defined(__ARM_ARCH_6J__) \
    || defined(__ARM_ARCH_6K__) \
    || defined(__ARM_ARCH_6Z__) \
    || defined(__ARM_ARCH_6ZK__) \
    || defined(__ARM_ARCH_6M__)
#define WTF_THUMB_ARCH_VERSION 3

#elif defined(__ARM_ARCH_6T2__) \
    || defined(__ARM_ARCH_7__) \
    || defined(__ARM_ARCH_7A__) \
    || defined(__ARM_ARCH_7R__) \
    || defined(__ARM_ARCH_7M__)
#define WTF_THUMB_ARCH_VERSION 4

/* RVCT sets __TARGET_ARCH_THUMB */
#elif defined(__TARGET_ARCH_THUMB)
#define WTF_THUMB_ARCH_VERSION __TARGET_ARCH_THUMB

#else
#define WTF_THUMB_ARCH_VERSION 0
#endif


/* CPU(ARMV5_OR_LOWER) - ARM instruction set v5 or earlier */
/* On ARMv5 and below the natural alignment is required.
   And there are some other differences for v5 or earlier. */
#if !defined(ARMV5_OR_LOWER) && !WTF_ARM_ARCH_AT_LEAST(6)
#define WTF_CPU_ARMV5_OR_LOWER 1
#endif


/* CPU(ARM_TRADITIONAL) - Thumb2 is not available, only traditional ARM (v4 or greater) */
/* CPU(ARM_THUMB2) - Thumb2 instruction set is available */
/* Only one of these will be defined. */
#if !defined(WTF_CPU_ARM_TRADITIONAL) && !defined(WTF_CPU_ARM_THUMB2)
#  if defined(thumb2) || defined(__thumb2__) \
    || ((defined(__thumb) || defined(__thumb__)) && WTF_THUMB_ARCH_VERSION == 4)
#    define WTF_CPU_ARM_TRADITIONAL 0
#    define WTF_CPU_ARM_THUMB2 1
#  elif WTF_ARM_ARCH_AT_LEAST(4)
#    define WTF_CPU_ARM_TRADITIONAL 1
#    define WTF_CPU_ARM_THUMB2 0
#  else
#    error "Not supported ARM architecture"
#  endif
#elif CPU(ARM_TRADITIONAL) && CPU(ARM_THUMB2) /* Sanity Check */
#  error "Cannot use both of WTF_CPU_ARM_TRADITIONAL and WTF_CPU_ARM_THUMB2 platforms"
#endif /* !defined(WTF_CPU_ARM_TRADITIONAL) && !defined(WTF_CPU_ARM_THUMB2) */

#if defined(__ARM_NEON__) && !defined(WTF_CPU_ARM_NEON)
#define WTF_CPU_ARM_NEON 1
#endif

#if defined(__ARM_NEON__) && PLATFORM(TIZEN)

#if defined (WTF_CPU_ARM_NEON)
// All NEON intrinsics usage enabled for Tizen by this macro.
#define HAVE_ARM_NEON_INTRINSICS 1
#endif

#else

#if CPU(ARM_NEON) && (!COMPILER(GCC) || GCC_VERSION_AT_LEAST(4, 7, 0))
// All NEON intrinsics usage can be disabled by this macro.
#define HAVE_ARM_NEON_INTRINSICS 1
#endif  /* CPU(ARM_NEON) && (!COMPILER(GCC) || GCC_VERSION_AT_LEAST(4, 7, 0)) */

#endif /* defined(__ARM_NEON__) && PLATFORM(TIZEN) */

#endif /* ARM */

#if CPU(ARM) || CPU(MIPS) || CPU(SH4) || CPU(SPARC)
#define WTF_CPU_NEEDS_ALIGNED_ACCESS 1
#endif

/* Samsung Platforms */
#if PLATFORM(TIZEN)

/* webkit extension library */
#if ENABLE(TIZEN_PROD_WEBKIT_EXTENSION)
#define ENABLE_TIZEN_PROD_DETECT_CONTENTS 1 /*Gurpreet Kaur(k.gurpreet@samsung.com) : Detect email/phone when user tap on email/phone without
link property*/
#define ENABLE_TIZEN_PROD_CHANNEL_SCROLL 1 /* Siddharth Bhardwaj(siddharth.bh@samsung.com): Add channel scroll */
#endif

#define ENABLE_LEGACY_WEB_AUDIO 1 /* Praveen Jadhav(praveen.j@samsung.com) : Support Legacy APIs in WebAudio*/
#define ENABLE_TIZEN_WEBKIT2_UNIT_TESTS 1 /* g.czajkowski@samsung.com, ryuan.choi@samsung.com : Make it possible to change resource path of unit test */

/* API extensions */
#define ENABLE_TIZEN_JS_EXT_API 1 /* Gyuyoung Kim(gyuyoung.kim@samsung.com): Add new APIs for JS */

/* EWK patches */
#define ENABLE_TIZEN_ISF_PORT 1 /* Michal Pakula(m.pakula@samsung.com) : feature for integrating the EditorClient Port with TIZEN2.0 ISF */
#define ENABLE_TIZEN_MULTIPLE_SELECT 1 /*Santosh Mahto (santosh.ma@samsung.com) : enable the feature of handling select element multple selection */
#define ENABLE_TIZEN_POPUP_PICKER_WITH_BUTTONS 1 /* Divakar (divakar.a@samsung.com) : for Popup picker with radio buttons support. */
#define ENABLE_TIZEN_PASTEBOARD 1 /* Michal Pakula(m.pakula@samsung.com), Eunmi Lee(eunmi15.lee@samsung.com) : Pasteboard implementation for WK1 and WK2. */
#define ENABLE_TIZEN_TEXT_CARET_HANDLING_WK2 1 /* Piotr Roguski(p.roguski@samsung.com) : enables two methods for getting and setting text caret position in editable fields */
#define ENABLE_TIZEN_ENLARGE_CARET_WIDTH 1 /* Jinwoo Song(jinwoo7.song@samsung.com): enlarge default caret width(size) - Email team is demanding this function. When composing, it is difficult to know where caret is.*/
#define ENABLE_TIZEN_CARET_INTERVAL 1 /* Jinwoo Song(jinwoo7.song@samsung.com): Set caret interval as 0.6 to align with EFL entry's caret interval. */
#define ENABLE_TIZEN_CUSTOM_HEADERS 1 /* Who is owner ? */
#define ENABLE_TIZEN_ICON_DATABASE 1 /* Nikhil Bansal(n.bansal@samsung.com), Changhyup Jwa(ch.jwa@samsung.com) : Tizen feature for icon database */
#define ENABLE_TIZEN_FAVICON 1 /* Albert Malewski(a.malewski@samsung.com) : Enable favicon in WebKit */
#define ENABLE_TIZEN_DBSPACE_PATH 1 /* Jaehun Lim(ljaehun.lim@samsung.com) : Set the TIZEN's default database directory */
#define ENABLE_TIZEN_USING_EXTERNAL_DOWNLOAD_CLIENT 1 /* Keunsoon Lee(keunsoon.lee@samsung.com) : not to use WebKit's internal download module, but notify the download starting to client to give a chance to use external Download Client. */
#define ENABLE_TIZEN_DOM_TIMER_MIN_INTERVAL_SET 1 /* Gyuyoung Kim(gyuyoung.kim@samsung.com) : Set minimum interval for DOMTimer */
#define ENABLE_TIZEN_CACHE_CONTROL 1 /*Sungman Kim(ssungmai.kim@samsung.com) : Control cache enable or disable mode*/

#if ENABLE(TIZEN_ISF_PORT)
#define ENABLE_TIZEN_KEYPRESS_KEYCODE_FIX 0 /* Bartlomiej Gajda (b.gajda@samsung.com) : Ensures that keypress events have correct keycode for korean characters. */
#endif

/* This patch should be removed if multiple UIProcess can exist concurrently.
   This will be removed when libsoup can handle multiple process file concurrency problem. */
#define ENABLE_TIZEN_CACHE_DUMP_SYNC 1 /* Keunsoon Lee(keunsoon.lee@samsung.com) : UIProcess send synchronous API to WebProcess to dump soup cache. */

/* ewk_frame_script_execute should return the string even even if the result is the object.
     because the object can be converted to the string using toString function.
     ex) When the result is DIV Element, it is converted "[object HTMLDivElement]" by toString function.
 */
#if ENABLE(SQL_DATABASE)
#define ENABLE_TIZEN_SQL_DATABASE 1 /* Jiyeon Kim(jiyeon0422.kim@samsung.com) : Enable Web SQL */
#define ENABLE_TIZEN_SQL_EXTENDED_ERROR_HANDLING 1 /* Pramod B S(pramod.bs@samsung.com) : to handle sql extended error code */
#endif

#define ENABLE_TIZEN_SUPPORT_BEFORE_UNLOAD_CONFIRM_PANEL 1 /* Jaehun Lim(ljaehun.lim@samsung.com) : Show confirm window when beforeUnload event is fired */

/* WebCore's patches */
#define ENABLE_TIZEN_MALLOC_BACKTRACE 1 /* Ryuan Choi(ryuan.choi@samsung.com) : It's just for debugging before crashed */
#define ENABLE_TIZEN_DISABLE_CUSTOM_SCROLLBAR 1 /* Ryuan Choi(ryuan.choi@samsung.com) : disable custom scrollbar (modified from r134878) */
#define ENABLE_TIZEN_SYSTEM_FONT 1 /* Ryuan Choi(ryuan.choi@samsung.com) : Bug fix, default width of input field is too big. */
#define ENABLE_TIZEN_MEDIAQUERY_WORKAROUND 1 /* Ryuan Choi(ryuan.choi@samsung.com) : device_aspect_ratio should be viewport dependent until we have better solution.*/
#define ENABLE_TIZEN_INJECTED_BUNDLE_CRASH_WORKAROUND 1 /* Ryuan Choi(ryuan.choi@samsung.com) : This is workaround to fix unexpected WRT crash */
#define ENABLE_TIZEN_CLICK_IMAGE_WORKAROUND 1 /* Ryuan Choi(ryuan.choi@samsung.com) : clicking image(with fitted) is a feature of browser but buggy on WK2 */
#define ENABLE_TIZEN_THEME_SCALE_SUPPORT 1 /* Ryuan Choi(ryuan.choi@samsung.com) : scaled theme support.*/
#define ENABLE_TIZEN_VIEWPORT_META_TAG 1 /* Gyuyoung Kim(gyuyoung.kim@samsung.com) : Support viewport meta tag */
#define ENABLE_TIZEN_CERTIFICATE_HANDLING 1 /* Krzysztof Czech(k.czech@samsung.com) : support for certificate callback function set by UI */ /* DongJae KIM : Enable Patch*/
#define ENABLE_TIZEN_ELEMENT_CREATED_BY_PARSER_INFO_STORE 0 /* Who is owner ? : support storing info about element's creation: from parser or JS */
#define ENABLE_TIZEN_DOWNLOAD_LINK_FILTER 1 /* Jaehun Lim(ljaehun.lim@samsung.com) : show download context menus for only http(s) and ftp(s) link */
#define ENABLE_TIZEN_ROOT_INLINE_BOX_SELECTION_TOP 1 /* Kamil Blank(k.blank@samsung.com) : Fix for too high selection on bbc.co.uk, https://bugs.webkit.org/show_bug.cgi?id=65307 */
#define ENABLE_TIZEN_PREVENT_CRASH_OF_TOUCH_EVENTS 1 /* Who is owner ? : https://bugs.webkit.org/show_bug.cgi?id=40163 */
#define ENABLE_TIZEN_INPUT_TAG_SUPPORT 1 /* Eunjoong Jeon : Support input tag type - 1. Hint the input tag type to apps 2. Define appearence of each input tag type */
#define ENABLE_TIZEN_INPUT_TAG_EXTENSION 1 /* Yuni Jeong(yhnet.jung@samsung.com) : For setting & getting focused input element value */
#define ENABLE_TIZEN_INPUT_COLOR_PICKER 1 /* KwangYong Choi(ky0.choi@samsung.com) : Interface for input color picker on Tizen */
#define ENABLE_TIZEN_DATALIST_ELEMENT 1 /* KwangYong Choi(ky0.choi@samsung.com) : Support datalist feature of text input field */
#define ENABLE_TIZEN_CONTEXT_MENU_TEMPORARY_FIX 1 /* Michal Pakula(m.pakula@samsung.com) : Temporary hack to prevent from crash when calling context menu on editable fiedld */
#define ENABLE_TIZEN_CONTEXT_MENU_SELECT 1 /* Michal Pakula(m.pakula@samsung.com) : Adds Select All and Select options to context menu */
#define ENABLE_TIZEN_MEDIA_CONTROL_USER_AGENT_SHEET 1 /* Gyuyoung Kim(gyuyoung.kim@samsung.com) Add media control ui sheet for TIZEN */
#define ENABLE_TIZEN_LINK_EFFECT 1 /* Sanghyup Lee(sh53.lee@samsung.com) To beep when click link*/
#define ENABLE_TIZEN_SELECTION_BACKGROUND_COLOR 1 /* Sanghyup Lee(sh53.lee@samsung.com) : for opaque selection background color to follow TIZEN UX guide */
#define ENABLE_TIZEN_CODEBASE_ATTRIBUTE_SUPPORT 1 /* Gyuyoung Kim (gyuyoung.kim@samsung.com) : Support codebase attribute in HTML object element */
#define ENABLE_TIZEN_COMMENT_SIZE_LIMIT_PATCH 1 /* Rashmi Kulakarni(rashmi.vijay@samsung.com) : Set a limit for the comment length in HTML content */
#define ENABLE_TIZEN_CACHE_IMAGE_GET 1 /* Raghu Ram Nagaraj(r.nagaraj@samsung.com) : Checking and retrieving cached images */
#define ENABLE_TIZEN_CHILD_NODE_IMAGE_URL 1 /* Raghu Ram Nagaraj(r.nagaraj@samsung.com) : Checking and retrieving image URL for child node of container/parent node  */
#define ENABLE_TIZEN_WEBSOCKET_READ_BUFFER_CRASH_PATCH 1 /* Rashmi Kulakarni(rashmi.vijay@samsung.com) : Added Null checks to prevent crash during socket connection close */
#define ENABLE_TIZEN_EWK_CONTEXT_CRASH_PATCH 1 /* Rashmi Kulakarni(rashmi.vijay@samsung.com) : Added Null check to prevent crash during ewk_context destruction */
#define ENABLE_TIZEN_PATCH_FOR_PAGECACHE_CRASH 1 /* Rashmi Kulakarni(rashmi.vijay@samsung.com): Fix for preventing page cache crash */
#define ENABLE_TIZEN_DOCUMENT_LOADER_NULL_CHECK 1 /* Shreeram Kushwaha(shreeram.k@samsung.com): Added Null check to prevent crash when DocumentLoader becomes null*/
#define ENABLE_TIZEN_DAILY_UPVERSIONING 1 /* Krzysztof Czech(k.czech@samsung.com) : Used for fixing building errors while doing daily upversioning, should be deleted before merging to master*/

#define ENABLE_TIZEN_GRAPHICSCONTEXT_COLLECT_REGION 1 /*KyungTae Kim(ktf.kim@samsung.com) : Collect clip regions in GraphicsContext for clipping in endTransparencyLayer */
#define ENABLE_TIZEN_SEARCH_FIELD_STYLE 1 /* Jaehun Lim(ljaehun.lim@samsung.com) : making search fields style-able */
#define ENABLE_TIZEN_RELOAD_CACHE_POLICY_PATCH 1 /*Sungman Kim(ssungmai.kim@samsung.com) : Set cache policy of initialRequest to ReloadIgnoringCacheData from browser reload*/
#define ENABLE_TIZEN_WEBGL_ANGLE_INTEGRATION 1 /*Shanmuga Pandi (shanmuga.m@samsung.com) : For ANGLE Integration for WEBGL*/
#define ENABLE_TIZEN_CONTEXT_CLICK 1 /*Wojciech Bielawski (w.bielawski@samsung.com) : Enable contextClick() functionality*/
#define ENABLE_TIZEN_CAIRO_RGBA_SWIZZLING 1 /*Shanmuga Pandi (shanmuga.m@samsung.com) : To use Cairo Based RGBA Swizzling for BitmapTexture update to avoid GraphicsContext3D extractImageData */
#define ENABLE_TIZEN_OVERLAP_REGION_PAINT_OPTIMIZATION 1 /*Shanmuga Pandi (shanmuga.m@samsung.com) : Optimize performance of painting of using overlap regions*/
#define ENABLE_TIZEN_AVOID_ARM_NEON_INTRINSICS_FOR_FE_BLEND 1 /* Shanmuga Pandi (shanmuga.m@samsung.com) : Avoid ARM_NEON_INTRINSIC code for FEBlend, since there is some compiler bug that incorrectly caused calls to ARM NEON store intrinsics (such as vst1_u8) to be optimized away. */
#define ENABLE_TIZEN_RESET_FIXED_LAYER_POSITION_ON_REMOVE 1 /* Shanmuga Pandi (shanmuga.m@samsung.com) :  Reset Composited layer's position, if position:fixed element property removed.*/
#define ENABLE_TIZEN_CANVAS2D_FILL_IMAGE_PATTERN 1 /* Shanmuga Pandi (shanmuga.m@samsung.com) : Fix for the Flickering issue while drawing image pattern with Alpha value. */
#define ENABLE_TIZEN_FIRST_LINE_IMAGE_PAINTING 1 /* Sangyong Park (sy302@samsung.com) : Fix image painting issue of first line pseudo style */

#define ENABLE_TIZEN_RUNLOOP_WAKEUP_ERROR_WORKAROUND 1 /* Seokju Kwon : Fix S1-6807 issue temporarily */
#define ENABLE_TIZEN_WEBSOCKET_TLS_SUPPORT  1 /* Basavaraj P S : Handling TLS connection for for secure websocket requests */
#define ENABLE_TIZEN_CHECK_DID_PERFORM_FIRST_NAVIGATION 1 /* Sungman Kim(ssungmai.kim@samsung.com) : To skip sendSync message from WebProcess to UIProcess, doesn't use itemAtIndex() on BackForwardController */
#define ENABLE_TIZEN_CSS_FILTER_OPTIMIZATION 1 /* Hurnjoo Lee(hurnjoo.lee@samsung.com : Optimize performance of css filters */

#define ENABLE_TIZEN_BACKGROUND_DISK_CACHE 0 /* Byeongha.cho(byeongha.cho@samsung.com), Sungman Kim(ssungmai.kim@samsung.com) : Cache encoded image data to disk on background state. */
#define ENABLE_TIZEN_TAP_ON_LINK_INVOKES_HAND_CURSOR 1 /* Manoj Modak(manoj.modak@partner.samsung.com): Avoid applying focus to inappropriate targets such as large divs with event handlers, test candidate targets and only focus them if they display a hand cursor*/
#define ENABLE_TIZEN_FILTER_SCALE_BUG_FIX 1 /* Hurnjoo Lee(hurnjoo.lee@samsung.com : Fix filter bug that elements were blurred when contents scale was not 1. */
#define ENABLE_TIZEN_ADJUST_IMG_WIDTH_WITH_PERCENT 1 /* Jaehun Lim(ljaehun.lim@smasung.com) : Re-calculate <img> width with % width */

#define ENABLE_TIZEN_OPTIMIZE_CONTENT_WIDTH_HEIGHT_CALCULATION 1 /* Jinwoo Song(jinwoo7.song@samsung.com) : Optimize performance of content width/height calculation. */
#define ENABLE_TIZEN_TIMER_THROTTLE 1 /* Sangyong Park (sy302@samsung.com) : Adjust the order or interval of timer execution */

#if ENABLE(NOTIFICATIONS)
#define ENABLE_TIZEN_NOTIFICATIONS 1 /* Kihong Kwon(kihong.kwon@samsung.com) : Temp patch for notification score in the html5test.com */
#endif

#define ENABLE_TIZEN_ADJUST_PATTERN_MATRIX 1 /*KyungTae Kim(ktf.kim@samsung.com) : Adjust pattern matrix of repeated images to align to dest pixel grid for fixing seam issues */

#if ENABLE(INDEXED_DATABASE)
#define ENABLE_TIZEN_INDEXED_DATABASE 1 /* DongGwan Kim(donggwan.kim@samsung.com) : Support IndexedDB for JS bindings */
#endif

#if ENABLE(OFFLINE_WEB_APPLICATIONS)
#define ENABLE_TIZEN_APPLICATION_CACHE 1 /* Seonae Kim(sunaeluv.kim@samsung.com) : Support Application Cache */
#endif

#define ENABLE_TIZEN_WEB_STORAGE 1 /* Seonae Kim(sunaeluv.kim@samsung.com) : Support Web Storage */

#if ENABLE(MEDIA_STREAM)
#define ENABLE_TIZEN_MEDIA_STREAM 1 /* DongGwan Kim(donggwan.kim@samsung.com) : Support Media Stream */
#endif

#define ENABLE_TIZEN_ATLAS_IMAGE_BUG_FIX 1 /*KyungTae Kim(ktf.kim@samsung.com) : For fixing atlas image bug(when draw part of image, surrounding pixels were shown), using subSurface when draw part of image*/

#define ENABLE_TIZEN_INFLATE_NONE_BLUR_SHADOW_AREA 1 /*KyungTae Kim(ktf.kim@samsung.com) : Enlarge the clipping area 1 pixel so that the fill does not bleed (due to antialiasing) if the context is transformed*/
#define ENABLE_TIZEN_NOT_INITIAIZE_BACKGROUND_SIZE_IN_SHORTHAND_WORKAROUND 1 /*KyungTae Kim(ktf.kim@samsung.com) : Do not initialize background-size when the background shorthand doesn't include it(Bug 97761)*/
#define ENABLE_TIZEN_SET_CARET_HEIGHT_TO_OBJECT_HEIGHT 1 /*KyungTae Kim(ktf.kim@samsung.com) : Set caret height from selection height to object height for fix the case that image and text are in the same line (Bug 116424)*/
#define ENABLE_TIZEN_END_OF_LINE_SELECT_LEFTWORD 1 /*KyungTae Kim(ktf.kim@samsung.com) : Select left word when selectWord at the end of line in the editable contents(same with <input> and Firefox)*/
#define ENABLE_TIZEN_INFLATE_REPAINT_FOR_NO_OUTLINE_WORKAROUND 1 /*KyungTae Kim(ktf.kim@samsung.com) : Inflate repaint rect when there is no outline value, to repaint correctly texts like 'j'*/
#if USE(FREETYPE)
#define ENABLE_TIZEN_FT_EMBOLDEN_FOR_SYNTHETIC_BOLD 1 /*Younghwan Cho(yhwan.cho@samsung.com) : Use freetype's 'embolden' instead of drawing twice for synthetic bold*/
#endif
#define ENABLE_TIZEN_CHECK_SPACE_FOR_AVG_CHAR_WIDTH 1 /* Himanshu Joshi (h.joshi@samsung.com : Added check to consider Space as possible way to calculate Average Char Width if '0' and 'x' are not present*/
#define ENABLE_TIZEN_CHECK_FOR_FONT_LINEGAP 1 /* Himanshu Joshi (h.joshi@samsung.com : Added check to change font line gap value if calculated lineGap value from font returns as zero. */
#define ENABLE_TIZEN_MIRROR_CHECK_FOR_RTL 0 /* Rachit Puri(rachit.puri@samsung.com) : Added mirror check for RTL */
#define ENABLE_TIZEN_SYNTHETIC_BOLD_OFFSET 1 /* Hyeonji Kim(hyeonji.kim@samsung.com) : interpret syntheticBoldOffset as a length in device pixels */

#define ENABLE_TIZEN_ADD_AA_CONDITIONS_FOR_NINE_PATCH 1 /*Younghwan Cho(yhwan.cho@samsung.com) : Add conditions of antialias for fixing 9patch-problem */
#define ENABLE_TIZEN_WEBKIT_OWN_FONT_FILES 0 /*Younghwan Cho(yhwan.cho@samsung.com) : webkit's font-config is seperated from the system font's*/
#define ENABLE_TIZEN_NOT_CHECK_ROUNDED_BORDER_ALL_CLIPPED_OUT 1 /*KyungTae Kim(ktf.kim@samsung.com) : For fixing rounded rect border bug(borders on different tiles looks different), remove code for checking rounded rect border all clipped out and setting round to 0*/
#define ENABLE_TIZEN_ROUNDED_BORDER_CLIP_ANTIALIAS_ON 1 /*KyungTae Kim(ktf.kim@samsung.com) : On the antialias setting for a  convex polygon clip for a rounded rect. The antialias setting for rounded rects' borders must be ON because rounded rects painted with antialias on*/
#define ENABLE_TIZEN_NAVIGATOR_LANGUAGE_STR 1 /* Kihong Kwon(kihong.kwon@samsung.com) : Modify return value from like a "ko-KR.UTF8" to "ko-KR" for "navigator.language". */
#define ENABLE_TIZEN_CONSIDER_COOKIE_DISABLED_IF_SOUP_COOKIE_JAR_ACCEPT_NEVER_IS_SET 1 /* Raveendra Karu(r.karu@samsung.com) : Gets cookies enabled/disabled status from browser settings and returns the same to the Navigator Object */
#define ENABLE_TIZEN_REDIRECTED_LOCATION_IS_NOT_UTF_8 1 /* Raveendra Karu(r.karu@samsung.com) : If redirected url is not utf-8 encoded, String:fromUTF8 (url) returns NULL. This patch is to avoid this problem*/
#define ENABLE_TIZEN_CLEAR_HTTPBODY_IN_POST_TO_GET_REDIRECTION 1 /* Raveendra Karu(r.karu@samsung.com) : Clear request http body if redirection happens from POST/PUT to GET */
#define ENABLE_TIZEN_AUTHENTICATION_CHALLENGE_ENABLED_IN_ALL_FRAMES 1 /* Raveendra Karu(r.karu@samsung.com) : Authentication challenge is enabled even if the frame is not a main frame*/
#define ENABLE_TIZEN_UNIVERSAL_FILE_READ_ACCESS 1 /* Raveendra Karu(r.karu@samsung.com) : Enabling universal file read access for file: type urls */
#define ENABLE_TIZEN_DESTROY_TIMEOUT_SOURCE 1 /* Raveendra Karu(r.karu@samsung.com) : Destroying timeoutSource (timer) of a request after processing of the request is done */
#define ENABLE_TIZEN_INTERCEPT_REQUEST 1 /*Raveendra Karu (r.karu@samsung.com) : Intercepting network requests to load resources from operator application */
#define ENABLE_TIZEN_ALLOW_STALE_CACHE_IN_BACKFORWARD 1 /* Raveendra Karu (r.karu@samsung.com) : loading resources from cache in case of back forward navigation and session restore case */
#define ENABLE_TIZEN_DISABLE_PAGECACHE_FOR_HTTPS_SITES 1 /*Raveendra Karu (r.karu@samsung.com) : Disabling page cache for secured (https) sites as it is not sending certificate to browser app */
#define ENABLE_TIZEN_FIX_SHOULD_AA_LINES_CONDITION 1 /*Younghwan Cho(yhwan.cho@samsung.com) : Add some conditions for AA to draw box-lines */
#define ENABLE_TIZEN_JPEGIMAGE_DECODING_THREAD 0 /* Keunyong Lee(ky07.lee@samsung.com) : Make new thread for Jpeg image decoding */
#define ENABLE_TIZEN_CLEAR_MEMORY_CACHE_BUG_FIX 1 /* Keunyong Lee(ky07.lee@samsung.com) : Fix decoded data contolling problem after memory cache clearing */
#define ENABLE_TIZEN_BLOCK_SENDING_RESIZE_EVENT_WHILE_LOADING 0 /* Jaehun Lim(ljaehun.lim@samsung.com) : Fix the infinite layout when frame flattening is enabled */
#define ENABLE_TIZEN_REMOVE_IMG_SRC_ATTRIBUTE 1 /* Jaehun Lim(ljaehun.lim@samsung.com) : Don't paint when <img>'s src attribute is removed */
#define ENABLE_TIZEN_EMOJI_FONT 0 /* Hyeonji Kim(hyeonji.kim@samsung.com) : webkit's emoji font */
#define ENABLE_TIZEN_USER_SETTING_FONT 1 /* Hyeonji Kim(hyeonji.kim@samsung.com) : If we don't find glyphs for characters, we can use SLP fallback font that is selected by user in Setting */
#define ENABLE_TIZEN_WRT_APP_URI_SCHEME 1   /*Sungman Kim(ssungmai.kim@samsung.com) : WRT data encryption support*/
#define ENABLE_TIZEN_SUPPORT_COMPLEX_FONTS_SHAPING 1 /* Hyeonji Kim(hyeonji.kim@samsung.com) : Support complex fonts shaping */
#define ENABLE_TIZEN_FALLBACK_FONTDATA 1 /* Hyeonji Kim(hyeonji.kim@samsung.com) : Add the fallback fontData to FontFallbackList */
#define ENABLE_TIZEN_FAKE_ITALIC_SMALLCAPS_COMBINATION 1 /* Tanvir Rizvi(tanvir.rizvi@samsung.com) Synthesize the small-caps and Italic combination */
#define ENABLE_TIZEN_COLLECT_HARFBUZZRUN 1 /* Rashmi Shyamasundar(rashmi.s2@samsung.com) Create proper harfBuzzRun for a word which follows a space */
#define ENABLE_TIZEN_CANVAS_2D_LINE_JOIN 1 /* Rashmi Shyamasundar(rashmi.s2@samsung.com) Save the value of lineJoin in GraphicsContextState */
#define ENABLE_TIZEN_CHECK_FONTCONFIG_PATTERN 1 /* Hyeonji Kim(hyeonji.kim@samsung.com) : Check fontconfig pattern validation */
#define ENABLE_TIZEN_FONT_SCALABLE_PRIORITY 0 /* Pramod B S(pramod.bs@samsung.com) disabling the scalable font ttf priority */
#define ENABLE_TIZEN_FONT_GLYPH_FOR_32BIT_UNICODE 1 /* Hyeonji Kim(hyeonji.kim@samsung.com) : Fill glyph page for 32bit unicode */
#define ENABLE_TIZEN_CHECK_UNICODE_SCRIPT_FOR_HARFBUZZRUN 1 /* Hyeonji Kim(hyeonji.kim@samsung.com) : Prevent that Arabic text is split into a separate run */
#define ENABLE_TIZEN_FIX_COMPLEX_TEXT_INITIAL_ADVANCE 1 /* Insoon Kim(is46.kim@samsung.com) : Adjust start position for complex text, Fill glyphbuffer in DrawRange  */

#define ENABLE_TIZEN_ON_AUTHENTICATION_REQUESTED 1 /* Sungman Kim(ssungmai.kim@samsung.com) : Implement to requested authentication signal handling method */
#define ENABLE_TIZEN_ON_REDIRECTION_REQUESTED 1 /* Sungman Kim(ssungmai.kim@samsung.com) : Feature that offer the redirection header by callback set function */
#define ENABLE_TIZEN_BASE64 1 /* Sanghyun Park(sh919.park@samsung.com) : Fix the bugs about atob(Base64) */
#define ENABLE_TIZEN_LAYOUTSTATE_NULL_CHECKING_WORKAROUND 1 /* Jaehun Lim(ljaehun.lim@samsung.com) : Add null checking for LayoutState, LayoutState can be null while push and pop */
#define ENABLE_TIZEN_DONT_DISPATCH_FAKEMOUSEMOVEEVENT_WHEN_SCROLLING_ON_TOUCH_DEVICES 1 /* Jaehun Lim (ljaehun.lim@samsung.com) : Don't dispatch fake mouse move events on touch devices for fixing hover problem */
#define ENABLE_TIZEN_LOAD_HTML_STRING_AS_UTF8 1 /* KwangYong Choi (ky0.choi@samsung.com) : Use UTF-8 instead of UTF-16 when the page is loaded by WebPage::loadHTMLString() */
#define ENABLE_TIZEN_DRAW_SCALED_PATTERN 0 /* Kyungjin Kim(gen.kim@samsung.com) : Scale image prior to draw it's pattern to enhance performance */
#define ENABLE_TIZEN_PAINT_SELECTION_ANTIALIAS_NONE 1 /* Hyeonji Kim(hyeonji.kim@samsung.com) : Remove the line between the preceding text block and the next text block which are selected */
#define ENABLE_TIZEN_EDITING_BACKSPACE 1 /* Krzysztof Wolanski (k.wolanski@samsung.com) : Fix the wrong backspace b */
#define ENABLE_TIZEN_PRESERVE_PROPERTIES_WHEN_PARAGRAPH_MERGE 1 /* Grzegorz Czajkowski (g.czajkowski@samsung.com): This is a quick fix for background-color and text-decoration defined in class are not copied when paragraph is merged.  We erroneously copy only CSS inheritable properties via removeNonEditingProperties(). */
#define ENABLE_TIZEN_EDITING_NO_LAYOUT_DURING_REPLACE_WORKAROUND 1 /* KyungTae Kim(ktf.kim@samsung.com) : While typing, text node is replaced with remove&insert. Layout isn't needed between them*/
#define ENABLE_TIZEN_USE_SETTINGS_FONT 1 /* Hyeonji Kim(hyeonji.kim@samsung.com) : When font-family is "Tizen", use system's setting font as default font-family */
#define ENABLE_TIZEN_CANVAS_TODATAURL_USING_IMAGE_ENCODER 1 /* Kyungjin Kim(gen.kim@samsung.com) : Implement canvas.toDataURL using PNGEncoder and JPEGEncoder */
#define ENABLE_TIZEN_PARAGRAPH_SEPARATOR_FIX 1 /* Author: Arpita Bahuguna (a.bah@samsung.com), Commiter: Michal Pakula (m.pakula@samsung.com) : This is a quick fix for a bug where new paragraph was not created on contenteditable area with image and text nodes inside span tag */
#define ENABLE_TIZEN_GENERIC_FONT_FAMILY 1 /* Hyeonji Kim(hyeonji.kim@samsung.com) : Tizen Generic font families */
#define ENABLE_TIZEN_FALLBACK_FONT_WEIGHT 1 /* Hyeonji Kim(hyeonji.kim@samsung.com) : Check fontDescription's weight to find the bold fallbackfont when font-weight is "bold" */
#define ENABLE_TIZEN_FONT_SIZE_CSS_STYLING 1 /* Shreeram Kushwaha(shreeram.k@samsung.com) : Add function to enable setting font size in css style */
#define ENABLE_TIZEN_CHECK_FOR_ZERO_WIDTH_JOINER 1 /* Shreeram Kushwaha (shreeram.k@samsung.com : Added check for Zero Width Joiner when creating harfbuzz runs. */
#define ENABLE_TIZEN_HARFBUZZ_UPGRADE 1 /* Shreeram Kushwaha (shreeram.k@samsung.com) : For fixing issues related to harfbuzz library 0.9.40 upgrade*/
#define ENABLE_TIZEN_SPEECH_SYNTHESIS 1 /* Wojciech Bielawski (w.bielawski) : W3C WebSpeech API implementation */
#define ENABLE_TIZEN_POLICY_DECISION_HTTP_METHOD_GET 1/* Sungman Kim(ssungmai.kim@samsung.com) : Add function to get http method from navigation policy decision*/
#define ENABLE_TIZEN_POLICY_DECISION_HTTP_BODY_GET 1/* Changhyup Jwa(ch.jwa@samsung.com) : Add function to get http body from navigation policy decision*/
#define ENABLE_TIZEN_BROWSER_FONT 1 /* Hyeonji Kim(hyeonji.kim@samsung.com) : use Tizen_Browser font-family for browser font visibility */
#define ENABLE_TIZEN_PREVENT_COMPILER_OPTIMIZATION_FOR_PIXELDATA 1 /* Tanvir Rizvi(tanvir.rizvi) : Use Volatile qualifier for pixelData to prevent compiler from optimizing the Pixel Data */
#define ENABLE_TIZEN_PASTE_IMAGE_URI 1 /* Shreeram Kushwaha (shreeram.k@samsung.com) : Add function to paste image as URI for Email application*/
#define ENABLE_TIZEN_FONT_SIZE_CSS_BACKGROUND_WORKAROUND 1 /* KyungTae Kim(ktf.kim@samsung.com) : When setting font-size to the texts with background, split & reapply the background style*/
#define ENABLE_TIZEN_DATAURL_SIZE_LIMIT 1 /* Divakar (divakar.a@samsung.com) : To limit data:image url size, in order to avoid low-memory condition. */
#define ENABLE_TIZEN_DISABLE_WEBKIT_TEXT_SIZE_ADJUST 1 /* Jaehun Lim (ljaehun.lim@samsung.com : Disable -webkit-text-size-adjust on easy mode. */
#define ENABLE_TIZEN_PREVENT_SAME_DOCEMENT_STOP_LOADING_FOR_YOUTUBE 1 /*Sungman Kim (ssungmai.kim@samsung.com) : Add condition statement for stop loading historyitem*/
#define ENABLE_TIZEN_PAINT_SHADOWS_FOR_ELLIPSIS 1 /* Hyeonji Kim(hyeonji.kim@samsung.com) : Paint one or more shadows for ellipsis */
#define ENABLE_TIZEN_HONOUR_IMAGE_ORIENTATION 0 /* Tanvir Rizvi(tanvir.rizvi@samsung.com) : Enable shouldRespectImageOrientationKey through webpage settings */
#if CPU(ARM_NEON)
#define ENABLE_TIZEN_MATRIX_MULTIPLY_NEON 1 /* Kyungjin Kim(gen.kim@samsung.com) : Inline assembly code for TransformationMatrix::multiply from SRUK */
#endif
#define ENABLE_TIZEN_FLOAT_TYPE_TRANSFORM_MATRIX 1 /* Soon-Young Lee(sy5002.lee@samsung.com) : Use float type matrix instead of double type from SRR */
#define ENABLE_TIZEN_TEXTUREMAPPERLAYER_TREE_UPDATE_WHEN_CHILD_CHANGED 1 /* Tanvir Rizvi(tanvir.rizvi@samsung.com) : Enable flag to prevent the texturemapperlayer tree parents become zero if children changed request is recieved */
#define ENABLE_TIZEN_CAIRO_PATTERN_DESTROY 1 /* Hyeonji Kim(hyeonji.kim@samsung.com) : Fix crash when the cairo pattern is destroyed */
#define ENABLE_TIZEN_SVG_SKIP_REPAINT_TRACKING 1 /* Hyeonji Kim(hyeonji.kim@samsung.com) : Skip SVG repaint tracking when parent container transforms */
#define ENABLE_TIZEN_IMAGEBUFFER_FAILURE_CHECK 1 /* Tanvir Rizvi(tanvir.rizvi@samsung.com) : Fix for crash happening when image buffer is NILL */
#define ENABLE_TIZEN_BLOCK_SELECTION_GAP 1 /* Hyeonji Kim(hyeonji.kim@samsung.com) : Fixed logicalTop and logicalHeight of block selection gap */
#define ENABLE_TIZEN_PREVENT_RESIZE_WHILE_ROTATING_WITH_SAME_WIDTH 1 /* Tanvir Rizvi(tanvir.rizvi@samsung.com) : Prevent View Resize when device is rotating, and view width is same as previous  */
#define ENABLE_TIZEN_RESTART_HIGH_QUALITY_TIMER_WHEN_NOT_ACTIVE 1 /* Tanvir Rizvi(tanvir.rizvi@samsung.com): Shot the High Quality Image Timer only when the timer is Inactive,else timer accumulates */
#if ENABLE(TEXT_AUTOSIZING)
#define ENABLE_TIZEN_TEXT_AUTOSIZING 1 /* Jaehun Lim(ljaehun.lim@samsung.com) : for Tizen environment */
#define ENABLE_TIZEN_TEXT_AUTOSIZING_NUMBER_OF_LINKS_FIX 1 /* Manoj Modak (manoj.modak@partner.samsung.com) : Device with smaller form factor, we need to check for 2 links in a row/line */
#endif

#define ENABLE_TIZEN_CUSTOM_CARET_COLOR 1 /* Divakar(divakar.a@samsung.com) : for Blue Colored Caret on email composer and other form controls. */
#define ENABLE_TIZEN_JPG_MIMETYPE_FIX 1  /* Divakar(divakar.a@samsung.com) : for adding 'image/jpg' in supported MIME types. */
#define ENABLE_TIZEN_ENCODING_VERIFICATION 1 /* Grzegorz Czajkowski (g.czajkowski@samsung.com) : new Ewk's API to check whether the encoding is supported by WebKit. This could be done while settings the encoding (in ewk_settings_default_encoding_set). However, the WRT Team prefers the separated API. */
#define ENABLE_TIZEN_XSSAUDITOR_CRASH_FIX 1 /* Divakar (divakar.a@samsung.com) This is a fix for PLM: P131107-02987. */
#define ENABLE_TIZEN_POLICY_FOR_NEW_WINDOW_ACTION_SYNC 1 /* Robert Plociennik (r.plociennik@samsung.com) : Implementation of synchronous new window action policy check; also a fix for LINUXWRT-1093. */
#define ENABLE_TIZEN_PAINT_MASK_IMAGE_ANTIALIAS_NONE 1 /* Hyeonji Kim(hyeonji.kim@samsung.com) : Remove the white lines at the boundary between the mask images */

/* Download Patches */
#define ENABLE_TIZEN_DOWNLOAD 1 /* Keunsoon Lee(keunsoon.lee@samsung.com) : */

/* Plugin Patches */
#define ENABLE_TIZEN_SUPPORT_PLUGINS 1 /* Mariusz Grzegorczyk(mariusz.g@samsung.com) : */
#define ENABLE_TIZEN_PLUGIN_SHOW_MISSING_PLUGIN_STATEMENT_EVEN_IF_PLUGIN_IS_DISABLED_ON_PREFERENCE 1 /* Keunsoon Lee(keunsoon.lee@samsung.com): show "missing plugin" even if preference setting for plugin is disabled. */
#define ENABLE_TIZEN_PLUGIN_WAITING_FOR_UPSTREAM 1 /* Keunsoon Lee(keunsoon.lee@samsung.com): codes which are waiting for landed on upstream */
#define ENABLE_TIZEN_SCAN_PLUGIN 1 /* KwangYong Choi(ky0.choi@samsung.com) : Implemented scan plugin feature for EFL */
#define ENABLE_TIZEN_PLUGIN_PREVENT_AUTOMATIC_FLASH_DOWNLOAD 1 /* KwangYong Choi(ky0.choi@samsung.com) : Prevent plugin auto-download */
#define ENABLE_TIZEN_PLUGIN_SCALE_FACTOR_FIX 1 /* KwangYong Choi(ky0.choi@samsung.com) : Use fixed device scale factor to fix drawing */
#define ENABLE_TIZEN_PAINT_MISSING_PLUGIN_RECT 1 /* KwangYong Choi(ky0.choi@samsung.com) : Paint missing plugin rect with gray */
#define ENABLE_TIZEN_PLUGIN_DESTROY_CRASH_FIX 1 /* Kwangyong Choi(ky0.choi@samsung.com) : Fix crash when the PluginControllerProxy is remvoed */
#define ENABLE_TIZEN_CACHE_RESOURCE_DELETE_WITH_ITERATOR 1 /*Mayur Kankanwadi(mayurk.vk@samsung.com) Deleting CachedResource based on iterator instead of URL key. Fix for P140604-02280.*/
#define ENABLE_TIZEN_FIX_TCT_FOR_CANVAS 1 /* Tanvir Rizvi(tanvir.rizvi): Modified the code to fix the tct test of canvas. WebKit didn't make the exception of InvalidStateError */
#define ENABLE_TIZEN_HIGH_QUALITY_IMAGE_FOR_COMPOSITE_SOURCE_OVER_OPERATION 1 /* Tanvir Rizvi(tanvir.rizvi@samsung.com) Enable High Quality image filters for sourceOver composite operation */
/*
 * Extend standard npapi NPNVariable.
 * Replace XEvents with XTNPEvents for tizen platform needs.
 */
#define ENABLE_TIZEN_NPAPI 1 /* Mariusz Grzegorczyk(mariusz.g@samsung.com) : */
#if ENABLE(TIZEN_NPAPI)
#define ENABLE_TIZEN_PLUGIN_SUSPEND_RESUME 1 /* KwangYong Choi(ky0.choi@samsung.com) : Suspend/Resume notification to the plugin */
#define ENABLE_TIZEN_PLUGIN_CUSTOM_REQUEST 1 /* Bang Kwang Min(justine.bang@samsung.com) : support the special call which can be reached to UIProcess */
#if ENABLE(TIZEN_PLUGIN_CUSTOM_REQUEST)
#define ENABLE_TIZEN_JSBRIDGE_PLUGIN 1 /* Bang Kwang Min(justine.bang@samsung.com) : support the special call which JS send some data to Native App through Plugin */
#endif
#define ENABLE_TIZEN_PLUGIN_KEYBOARD_EVENT 1 /* KwangYong Choi(ky0.choi@samsung.com) : Support TNP soft key event */
#define ENABLE_TIZEN_PLUGIN_TOUCH_EVENT 1 /* KwangYong Choi(ky0.choi@samsung.com) : Support TNP touch event */
#define ENABLE_TIZEN_PLUGIN_SILENT_CRASH 1 /* KwangYong Choi(ky0.choi@samsung.com) : Disable crash popup for PluginProcess */
#endif

/* Debugging Patch */
#define ENABLE_TIZEN_LOG 1 /* Gyuyoung Kim(gyuyoung.kim@samsung.com) : */
#if ENABLE(TIZEN_LOG)
#define ERROR_DISABLED 0
#define LOG_DISABLED 0
#endif
#define ENABLE_TIZEN_DISPLAY_MESSAGE_TO_CONSOLE 1 /* Seokju Kwon(seokju.kwon@samsung.com) : Display console-messages for WebApps, Browser ... */


/* Network Patches*/
/* macro which solve network delay when some of contents was blocked for a while
 - example) m.daum.net, m.paran.com
 - When one of request was blocked, progress looks stopped at 50~60%(but almost was loaded) */
#define ENABLE_TIZEN_NETWORK_DELAYED_RESOURCE_SOLVE 1 /* Łukasz Ślachciak(l.slachciak@samsung.com) : */
/* need to reset session when network device is changed.(3G->wifi, wifi->3G)
 * simple websites try to use previous socket which is binded previous network
 * device.
 */
#define ENABLE_TIZEN_SESSION_REQUEST_CANCEL 1 /* Who is owner ? : */

#if USE(SOUP)
/* enables ewk part of webkit soup cache */
#define ENABLE_TIZEN_SOUP_CACHE 1 /* Kwangtae Ko(kwangtae.ko@samsung.com : */
/* handle status code 5xx after receiving HTTP body */
#define ENABLE_TIZEN_SOUP_HANDLE_STATUS_CODE_5XX_AFTER_RECEIVING_HTTP_BODY 1 /* Changhyup Jwa(ch.jwa@samsung.com) : */
#define ENABLE_TIZEN_SOUP_NOT_ALLOW_BROWSE_LOCAL_DIRECTORY 1 /* Jongseok Yang(js45.yang@samsung.com) */
/* Workaround Patches */
#define ENABLE_TIZEN_ALLOW_CERTIFICATE_TRUSTED_URI  0 /*Sungman Kim(ssungmai.kim@samsung.com : Allow the specific uri to continue without certificate error. This is workaround. */
#define ENABLE_TIZEN_SET_AUTHORIZATION_TO_RESOURCE_REQUEST_FROM_SOUP_MESSAGE 1 /* Keunsoon Lee(keunsoon.lee@samsung.com) : Read Authorization header value from libsoup request header, and set it to ResourceRequest to pass it to ewk layer. This will be used by ewk_policy_decision. */
#endif
#define ENABLE_TIZEN_RESTARTED_CALLBACK_BUG_FIX 1 /*Sungman Kim(ssungmai.kim@samsung.com) : Resource handle ref count bug fix when restarted callback call*/
#define ENABLE_TIZEN_SOUP_CACHE_DIRECTORY_SET 1 /*Sungman Kim(ssungmai.kim@samsung.com) : Set the Soup, Cache directory path of SoupDataDirectory*/
#define ENABLE_TIZEN_SOUP_CACHE_DIRECTORY_PATH_SET 1 /* Raveendra Karu (r.karu@samsung.com) : Modified SoupCache directory path to "/opt/usr/apps/<package id>/cache" */
#define ENABLE_TIZEN_MALICIOUS_CONTENTS_SCAN 1 /*Daehyun Yoo(daehyun81.yoo@samsung.com) : Web contents scanning feature with McAfee*/
#define ENABLE_TIZEN_USER_AGENT_WHITELIST 1 /*Daehyun Yoo(daehyun81.yoo@samsung.com) : Whitelist feature for changing UA to custom UA, such as Android keyword added UA*/

#define ENABLE_TIZEN_PERSISTENT_COOKIES_SQL 1 /* Kwangtae Ko(kwangtae.ko@samsung.com : */

#define ENABLE_TIZEN_PAUSE_NETWORK 1 /* Gyuyoung Kim(gyuyoung.kim@samsung.com) : */
#define ENABLE_TIZEN_PAUSE_REFRESH 1 /* Divakar (divakar.a@samsung.com) : For pausing/resuming refresh timers on WebView's suspend and resume. */
#define ENABLE_TIZEN_HTTP_REQUEST_HEADER_APPEND 1 /* Kwangtae Ko(kwangtae.ko@samsung.com : Append request headers for  accept-charset and x-wap-proxy-cookie */
#define ENABLE_TIZEN_EXPONENTIAL_BUFFER_SIZE 1 /* Jie Chen(jie.a.chen@intel.com) */

/* Workaround Patches */
#define ENABLE_TIZEN_PREVENT_INCREMENTAL_IMAGE_RENDERING_WORKAROUND 1 /* Ryuan Choi(ryuan.choi@samsung.com) : */
/* fix for showing callendar on expedia site when frame flattening enabled */
#define ENABLE_TIZEN_EXPEDIA_CALENDAR_FIX 1
#define ENABLE_TIZEN_POPUP_FIX 1
/* fix for connection timeout */
#define ENABLE_TIZEN_TIMEOUT_FIX 1 /* Łukasz Ślachciak(l.slachciak@samsung.com) : */
/* Workaround for character interval defect */
#define ENABLE_TIZEN_FONT_HINT_NONE 1 /* Jaesick Chang(jaesik.chang@samsung.com) : */
/* Fix for continues layout when element having width given in percent is nested in flattened frame
   https://bugs.webkit.org/show_bug.cgi?id=54284
*/
#define ENABLE_TIZEN_ELEMENTS_NESTED_IN_FLATTENED_FRAME_FIX 1 /* Piotr Roguski(p.roguski@samsung.com) : */
#define ENABLE_TIZEN_FULLSCREEN_VIDEO_WORKAROUD 1 /* YongGeol Jung(yg48.jung@samsung.com) */
/* fix for wrong calculation of backing store's zoom center position. */
#define ENABLE_TIZEN_FIX_BUILD_BREAK_GCC 1 /* Sanggyu Lee(sg5.lee@samsung.com) : Fixed a fastMalloc compile error for Thumb2 with GCC */
/* View mode patches */
#define ENABLE_TIZEN_VIEW_MODE 1 /* Who is ownver ? : */
#define ENABLE_TIZEN_PREVENT_CRASH_VIMEO_SITE 1 /* Jaehun Lim(ljaehun.lim@samsung.com) : Add NULL check to prevent crash in vimeo.com (for i386 or non-flash environment) */
/* Workaround for using the defaut encoding set */
#define ENABLE_TIZEN_FIX_CLIP_PATTERN_RECT 1 /* Kyungjin Kim(gen.kim@samsung.com) : adjust clip rect to the min of pattern and clip for repeatX, repeatY */
#define ENABLE_TIZEN_2D_CANVAS_ZERO_GRADIENT 1 /* Rashmi Shyamasundar (rashmi.s2@samsung.com) : If the gradient size is zero, then the functions fillXXX/strokeXXX should paint nothing */
#define ENABLE_TIZEN_CANVAS2D_FILLMAXWIDTH 1 /* Rashmi Shyamasundar(rashmi.s2@samsung.com) : Do not paint if maxwidth is zero */

#define ENABLE_TIZEN_INPUT_PICKER_MAX_MIN 1 /* Shreeram Kushwaha (shreeram.k@samsung.com) : To enhance Input picker so that user won't be able to select out of range values*/
#define ENABLE_TIZEN_IFRAME_FLATTENING_WORKAROUND 1 /* Dong-Gwan Kim(donggwan.kim@samsung.com : workaround patch of iframe flattening related issue(DCM-2237) */
#define ENABLE_TIZEN_DISPLAY_MISSING_IMAGES 1 /* Dong-Gwan Kim(donggwan.kim@samsung.com : display missing images when auto load image is disabled(PLM130730-4828) */
#define ENABLE_TIZEN_DISPLAY_IMAGE_ALT_TEXT 1 /* Divakar (divakar.a@samsung.com) : Display alternate text when image is not available and alt text is present. */
#define ENABLE_TIZEN_VALIDATE_DOCUMENT_VARIABLE 1 /* Kyungjin Kim(gen.kim@samsung.com) : Workaround of checking m_document validation for VPSS-1204 */
#define ENABLE_TIZEN_SOUP_MESSAGE_PAUSE_SET_FLAG 1 /*Sungman : workaround patch to allow SoupMessage pause only in case of set specific flag */
#define ENABLE_TIZEN_RESET_MATRIX_AFTER_CLEAR_PATH 1 /* Kyungjin Kim(gen.kim@samsung.com : Reset current matrix when clear path */

/* JavaScript JIT */
#if ENABLE(JIT)
#define ENABLE_YARR 1
#define ENABLE_YARR_JIT 1
#endif

#define ENABLE_TIZEN_BUILD_THUMB_GCC 1
#define ENABLE_TIZEN_USE_STACKPOINTER_AT_WEBPROCESS_STARTUP 1

/* Original Webkit Features */
#if ENABLE(INSPECTOR)
#define ENABLE_TIZEN_REMOTE_WEB_INSPECTOR 1 /* Seokju Kwon(seokju.kwon@samsung.com) */
#endif

#define ENABLE_MAC_JAVA_BRIDGE 0
#define ENABLE_IMAGE_DECODER_DOWN_SAMPLING 0
#if ENABLE(IMAGE_DECODER_DOWN_SAMPLING)
#define ENABLE_TIZEN_IMAGE_DECODER_DOWN_SAMPLING 0 /* Jaehun Lim(ljaehun.lim@samsung.com) : Enabling image DOWN SAMPLING feature only for JPEG format */
#endif
#define ENABLE_TIZEN_JPEG_IMAGE_SCALE_DECODING 1 /* Keunyong Lee(ky07.lee@samsung.com) : Scaled decoding Feature for Jpeg Image. Becuase this Feature replace Down Sampling, IMAGE_DECODER_DOWN_SAMPLING should be false when you want to use it. */
#define ENABLE_TIZEN_REMOVE_DRAWLINE_ANTIALIAS_NONE 1 /* Keunyong Lee(ky07.lee@samsung.com) : Dash/Dot Box border and Text UnderLine display Bug Fix*/

#if ENABLE(TIZEN_WEBKIT2)

#define ENABLE_TIZEN_CONTEXT_MENU_WEBKIT_2 1 /* Gyuyoung Kim(gyuyoung.kim@samsung.com), Eunmi Lee(eunmi15.lee@samsung.com) : Support Context Menu for EFL WebKit2 */

#if USE(ACCELERATED_COMPOSITING) && USE(TEXTURE_MAPPER)
#define ENABLE_TIZEN_WEBKIT2_DEBUG_BORDERS 1 /* Hyowon Kim(hw1008.kim@samsung.com) : Renders a border around composited Render Layers to help debug and study layer compositing. */
#define ENABLE_TIZEN_WEBKIT2_TILED_AC 1 /* Youngtaeck Song(youngtaeck.song@samsung.com) : Tiling with Acceletated compositing for Tizen */
#define ENABLE_TIZEN_ONESHOT_DRAWING_SYNCHRONIZATION 1 /* Hyowon Kim(hw1008.kim@samsung.com) : Fix layer flickering */
#define ENABLE_TIZEN_DONT_USE_TEXTURE_MAPPER_TEXTURE_POOL 1 /* Seojin Kim(seojin.kim@samsung.com) : TextureMapper texture pool has no shrink mechanism currently, and leads to OOM finally */
#define ENABLE_TIZEN_INHERIT_REF_COUNT_TO_TEXTURE_MAPPER_LAYER 1 /* JungJik Lee (jungjik.lee@samsung.com) : Inherit the reference count to TextureMapperLayer */


#define ENABLE_TIZEN_TEXTURE_MAPPER_CULLER 1 /* Hurnjoo Lee(hurnjoo.lee@samsung.com) : Avoid rendring of layers that occluded by opaque rects */
#define ENABLE_TIZEN_RUNTIME_BACKEND_SELECTION 1 /* Hyowon Kim(hw1008.kim@samsung.com) : Allow selecting the backend for Texture Mapper at run-time */
#define ENABLE_TIZEN_UI_SIDE_ANIMATION_SYNC 1 /* Hyowon Kim(hw1008.kim@samsung.com) : keep last frame of an animation until the animated layer is removed */
#define ENABLE_TIZEN_FIX_WRONG_NORMALIZED_ANIMATION 1 /* Younghwan Cho (yhwan.cho@samsung.com) : fix wrong normalized value for end of animation with alternative */
#define ENABLE_TIZEN_CORRECT_CLIPPING_WITH_INTERMEDIATE_SURFACE 1 /* Hyowon Kim(hw1008.kim@samsung.com) : compute the correct clipping rect when using an intermediate surface */
#define ENABLE_TIZEN_PREVENT_CRASH_OF_UI_SIDE_ANIMATION 1 /* Hurnjoo Lee(hurnjoo.lee@samsung.com) : Prevent crashes of UI side animation */
#define ENABLE_TIZEN_WEBKIT2_PRE_RENDERING_WITH_DIRECTIVITY 1 /*JungJik Lee(jungjik.lee@samsung.com : Calculates cover-rect with trajectory vector scalar value to consider directivity. */
#define ENABLE_TIZEN_USE_FIXED_SCALE_ANIMATION 1 /* JungJik Lee(jungjik.lee@samsung.com) : use fixed scale if the layer is animating */
#define ENABLE_TIZEN_UPDATE_ONLY_VISIBLE_AREA_STATE 1 /* JungJik Lee(jungjik.lee@samsung.com) : Update tiles in only visible area */
#define ENABLE_TIZEN_DISABLE_DIRECTLY_COMPOSITED_IMAGE 1 /* Hurnjoo Lee(hurnjoo.lee@samsung.com) : Disable directly composited image */
#define ENABLE_TIZEN_ACCELERATED_ROTATE_TRANSFORM 0 /* Hurnjoo Lee(hurnjoo.lee@samsung.com) : Accelerated rotate transform */
#if ENABLE(TIZEN_RUNTIME_BACKEND_SELECTION)
#define TIZEN_VIRTUAL virtual
#else
#define TIZEN_VIRTUAL
#endif

#define ENABLE_TIZEN_LAYER_FLUSH_THROTTLING 1 /* JungJik Lee(jungjik.lee@samsung.com) : Give 0.5 sec delay to layer flush scheduler while loading the page */
#define ENABLE_TIZEN_COVERAREA_BY_MEMORY_USAGE 1 /* JungJik Lee(jungjik.lee@samsung.com) : calculate the cover area by memory usage */
#define WTF_USE_TEXTURE_MAPPER_GL 1 /* Youngtaeck Song(youngtaeck.song@samsung.com) : use TextureMapperGL for Tizen */
#define WTF_USE_UI_SIDE_COMPOSITING 1 /* Youngtaeck Song(youngtaeck.song@samsung.com) : Compositing on the UI-process in WebKit2 */
#define ENABLE_TIZEN_CANVAS_GRAPHICS_SURFACE 1 /* Heejin Chung(heejin.r.chung@samsung.com) : WebGL Based on GraphicsSurface */
#define ENABLE_TIZEN_SET_INITIAL_COLOR_OF_WEBVIEW_EVAS_IMAGE 1 /* Youngtaeck Song(youngtaeck.song@samsung.com) : Set initial color of webview evas image */
#define ENABLE_TIZEN_USE_CONTENTS_SCALE 1 /* Hyeonji Kim(hyeonji.kim@samsung.com) : use contents scale even if the layer's transform is not affine */
#endif
#define ENABLE_TIZEN_WEBKIT2_DDK_CHECK 0 /* JungJik Lee(jungjik.lee@samsung.com) : Fail in initialzing view when No DDK installed */

#if USE(TILED_BACKING_STORE)
#define ENABLE_TIZEN_WEBKIT2_TILED_BACKING_STORE 1 /* Youngtaeck Song(youngtaeck.song@samsung.com) : Tiled backing store for Tizen */
#define ENABLE_TIZEN_WEBKIT2_TILED_SCROLLBAR 1 /* Ryuan Choi(ryuan.choi@samsung.com) : Scrollbar implementation for Tiled DrawingArea */
#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
#define ENABLE_TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE_BACKUP_IMAGE 1 /* Eunsol Park(eunsol47.park@samsung.com), Hyowon Kim(hw1008.kim@samsung.com) : Display Backup Image when there is no painted tile*/
#define ENABLE_TIZEN_UV_MAPPING 1 /* Hyowon Kim(hw1008.kim@samsung.com) : UV mapping to use the platform-surface whose width should be multiple of 8 */
#define ENABLE_TIZEN_LOW_SCALE_LAYER 1 /* Eunsol Park(eunsol47.park@samsung.com), JungJik Lee(jungjik.lee@samsung.com) : Compositing low scale layer to avoid displaying empty tile*/
#if ENABLE(TIZEN_LOW_SCALE_LAYER)
#define ENABLE_TIZEN_DEFER_CLEARING_LOW_SCALE_LAYER_AT_ZOOM 1 /* Youngtaeck Song(youngtaeck.song@samsung.com) : Defer clearing low scale layer when zooming */
#endif
#define ENABLE_TIZEN_WEBGL_ANIMATION_SNAPSHOT_FIX 1 /* Shanmuga Pandi (shanmuga.m@samsung.com) : Fix for webgl animation site's snapshot issue */
#endif
#define ENABLE_TIZEN_PRERENDERING_FOR_ROTATION 1 /* Youngtaeck Song(youngtaeck.song@samsung.com) : Prerender contents so that we display whole image immediately when rotating */
#define ENABLE_TIZEN_FIX_PRERENDERING_FOR_ROTATION_SNAPSHOT_BUG 1 /* Tanvir Rizvi(tanvir.rizvi@samsung.com) : Fix for white patches when prerendering for rotation*/
#endif
#define ENABLE_TIZEN_WEBKIT2_TILED_AC_DONT_ADJUST_COVER_RECT 1 /* Eunsol Park(eunsol47.park@samsung.
com) : Patch to do not adjust cover rect as fixed pixel size*/
#define ENABLE_TIZEN_SUSPEND_PAINTING_DURING_ZOOMING 1 /* Sanghyup Lee(sh53.lee@samsung.com) : Suspend painting during zooming to improve perfomance. */
#define ENABLE_TIZEN_CHANGE_TO_MEDIUM_SCALE_IMAGE_AFTER_ZOOM_START 1/* Youngtaeck Song(youngtaeck.s
ong@samsung.com) : Change to medium scale image after zoom start for showing clear image at high sca
le. */
#define ENABLE_TIZEN_GSTREAMER_VIDEO 1 /* Kwangyong Choi : Fixed media control display when using gstreamer video */
#define ENABLE_TIZEN_MEDIA_PLAYER 1 /* Michal Poteralski : Enable Tizen Platform Media Player */

#if USE(TILED_BACKING_STORE) && ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
#define ENABLE_TIZEN_RECORDING_SURFACE_SET 0 /* Hyunki Baik(hyunki.baik@samsung.com) : recording surface features for WK2 */
#if ENABLE(TIZEN_RECORDING_SURFACE_SET)
#define ENABLE_TIZEN_RECORDING_SURFACE_PAINT_THREAD 0 /* Hyunki Baik(hyunki.baik@samsung.com), Hyeonji Kim(hyeonji.kim@samsung.com) : enable paint thread for parallelization in WK2 */
#endif
#else
#define ENABLE_TIZEN_RECORDING_SURFACE_SET 0 /* Hyunki Baik(hyunki.baik@samsung.com) : recording surface features for WK2 */
#define ENABLE_TIZEN_RECORDING_SURFACE_PAINT_THREAD 0 /* Hyunki Baik(hyunki.baik@samsung.com), Hyeonji Kim(hyeonji.kim@samsung.com) : enable paint thread for parallelization in WK2 */
#endif

#define ENABLE_TIZEN_PREFERENCE 1 /* Eunmi Lee(eunmi15.lee@samsung.com) : for slp specific preferences */
#define ENABLE_TIZEN_LOAD_REMOTE_IMAGES 1 /* Dongjae Kim(dongjae1.kim@samsung.com) : for tizen remode image load setting */

#define ENABLE_TIZEN_SOUP_COOKIE_CACHE_FOR_WEBKIT2 1 /* Jongseok Yang(js45.yang@samsung.com) : Implement soup cache, cookie */
#define ENABLE_TIZEN_WEBKIT2_THEME_SET_INTERNAL 1 /* Ryuan Choi(ryuan.choi@samsung.com) : Set to default theme for WK2/tizen */
#define ENABLE_TIZEN_INPUTTYPE_NUMBER 1 /* Eunsol Cha(e.cha@samsung.com) : customize InnerSpinButtonStyle */
#define ENABLE_TIZEN_WEBKIT2_PROXY 1 /* Ryuan Choi(ryuan.choi@samsung.com) : Provide API to set proxy */
#define ENABLE_TIZEN_WEBKIT2_POPUP_INTERNAL 1 /* Ryuan Choi(ryuan.choi@samsung.com) : popup implementation for WK2/tizen internal */
#define ENABLE_TIZEN_WEBKIT2_NOTIFY_POPUP_REPLY_STATUS 1 /* Byungwoo Lee(bw80.lee@samsung.com) : Notify popup reply status through the smart callback */
#define ENABLE_TIZEN_SCREEN_ORIENTATION_SUPPORT_INTERNAL ENABLE_TIZEN_SCREEN_ORIENTATION_SUPPORT /* Ryuan choi9ryuan.choi@samsung.com) : temporal implementation for screen orientation lock callback */
#define ENABLE_TIZEN_POPUP_BLOCKED_NOTIFICATION 1 /* Divakar (divakar.a@samsung.com) : Notifying UIProcess when Popup is blocked */
#define ENABLE_TIZEN_WEBKIT2_FORM_NAVIGATION 1 /*ByungJun Kim(bj1987.kim@samsung.com) : Add form_navigation to combo_box */

#define ENABLE_TIZEN_WEBKIT2_FOCUS_RING 1 /* Yuni Jeong(yhnet.jung@samsung.com) : Focus ring implementation for WK2 */
#define ENABLE_TIZEN_WEBKIT2_VIEW_VISIBILITY 1 /* Eunsol Park(eunsol47.park@samsung.com) : add API for setting the webPage's visibility */
#if ENABLE(TIZEN_WEBKIT2_VIEW_VISIBILITY)
#define ENABLE_TIZEN_CACHE_MEMORY_OPTIMIZATION 1 /* Keunyong Lee(ky07.lee@samsung.com) : The Patches that can reduce memory usage */
#endif
#define ENABLE_TIZEN_WEBKIT2_NUMBER_TYPE_SUPPORT 1 /* Seonae Kim(sunaeluv.kim@samsung.com) : add number types for switching WK number types */
#if ENABLE(INSPECTOR)
#define ENABLE_TIZEN_WEBKIT2_REMOTE_WEB_INSPECTOR 1 /* Seokju Kwon(seokju.kwon@samsung.com) : Remote Web Inspector implementation for WK2 */
#define ENABLE_TIZEN_WEBKIT2_NOTIFY_SUSPEND_BY_REMOTE_WEB_INSPECTOR 1 /* Bunam Jeon(bunam.jeon@samsung.com) : Notify to UI process that the content has been suspended by inspector */
#endif
#define ENABLE_TIZEN_WEBKIT2_VISITED_LINKS 1 /* Jaehun Lim(ljaehun.lim@samsung.com) : workaround patch for coloring visited links.
                                              * this patch should be removed after WK2 HistoryClient is implemented.
                                              */

#define ENABLE_TIZEN_WEBKIT2_HIT_TEST 1 /* Yuni Jeong(yhnet.jung@samsung.com) : Hit Test API implementation for WK2 */
#define ENABLE_TIZEN_WEBKIT2_PATCH_FOR_TC 1 /* Changhyup Jwa(ch.jwa@samsung.com) : Patchs to pass TC */
#define ENABLE_TIZEN_WEBKIT2_GET_TEXT_STYLE_FOR_SELECTION 1 /* Yuni Jeong(yhnet.jung@samsung.com) : Patchs to get text style for supporting email requirement */

#if ENABLE(TIZEN_PREFERENCE)
#define ENABLE_TIZEN_WEBKIT2_TEXT_ZOOM 1 /* Changhyup Jwa(ch.jwa@samsung.com) : Enable/Disable text zoom on user's pinch zoom */
#endif
#define ENABLE_TIZEN_WEBKIT2_CONTEXT_MENU_TEXT_SELECTION_MODE 1 /* Suhas Nagarmutt Kamath(suhas.kn@samsung.com) : Patchs to add "Text selection mode" options to context menu */
#define ENABLE_TIZEN_WEBKIT2_CREATE_VIEW_WITH_CREATED_PAGE_GROUP_WITH_IDENTIFIER 1 /* Yuni Jeong(yhnet.jung@samsung.com) : Patchs to create view with created page group with identifier */
#define ENABLE_TIZEN_WEBKIT2_SAME_PAGE_GROUP_FOR_CREATE_WINDOW_OPERATION 1 /* Jongseok Yang (js45.yang@samsung.com) : The page frome create window operation has same page group as the caller page */
#define ENABLE_TIZEN_WEBKIT2_SEPERATE_LOAD_PROGRESS 1 /* Yuni Jeong(yhnet.jung@samsung.com) : Patchs to seperate load progress callback for supporting OSP requirement  - "load,progress,started", "load,progress", "load,progress,finished" */
#define ENABLE_TIZEN_WEBKIT2_CONTEXT_MENU_ICON_TYPE_SUPPORT 0 /* Yuni Jeong(yhnet.jung@samsung.com) : Support icon type's option to context menu */
#define ENABLE_TIZEN_WEBKIT2_LOCAL_IMPLEMENTATION_FOR_ERROR 1 /* Jongseok Yang(js45.yang@samsung.com) : temporary pathes to maintain local implementation for error operation */
#define ENABLE_TIZEN_WEBKIT2_LOCAL_IMPLEMETATION_FOR_NAVIGATION_POLICY 1 /* Jongseok Yang(js45.yang@samsung.com) : temporary pathes to maintain local implementation for navigation policy operation */
#define ENABLE_TIZEN_WEBKIT2_LOCAL_IMPLEMETATION_FOR_FORM 1 /* Sangyong Park(sy302.park@samsung.com) : temporary pathes to maintain local implementation for form operation */
#define ENABLE_TIZEN_WEBKIT2_ABOUT_MEMORY 1 /* Jinwoo Song(jinwoo7.song@samsung.com) : Implement 'About:Memory' feature */

#if ENABLE(INSPECTOR)
#define ENABLE_TIZEN_NATIVE_MEMORY_SNAPSHOT 1 /*KyungTae Kim(ktf.kim@samsung.com) : Native memory snapshot is a functionality for measuring native memory on WebCore*/
#endif

#define ENABLE_TIZEN_DOWNLOAD_ATTRIBUTE 1 /*KyungTae Kim(ktf.kim@samsung.com) : To support download attribute on Webkit2. It'll be removed when webkit.org/b/102914 is merged*/
#define ENABLE_TIZEN_WEBKIT2_TEXT_TRANSLATION 1 /* Bunam Jeon(bunam.jeon@samsung.com) : Support the text translation of webkit2 */
#define ENABLE_TIZEN_WEBKIT2_CLIPBOARD_HELPER 1 /* Jongseok Yang(js45.yang@samsung.com) : Module in UIProcess to support clipboard operation */
#define ENABLE_TIZEN_WEBKIT2_CONTEXT_MENU_CLIPBOARD 1 /* Taeyun An(ty.an@samsung.com) : Patches to display clipboard option in contextmenu */

#if ENABLE(TIZEN_WEBKIT2_TILED_BACKING_STORE)
#define ENABLE_TIZEN_WEBKIT2_HISTORICAL_RESTORE_VISIBLE_CONTENT_RECT 1 /* Changhyup Jwa(ch.jwa@samsung.com) : Restore visible content rect on going back and forward */
#define ENABLE_TIZEN_WEBKIT2_BEFORE_PAGE_RENDERED_SCROLL_POSITION 1 /*Santosh Mahto(santosh.ma@samsung.com) : save and restore  the scrollpostion before page is not rendered fully. */
#define ENABLE_TIZEN_WEBKIT2_RESTORE_SCROLLPOINT_ON_FRAME_LOAD_FINISH 1 /*Deepak Mittal(deepak.m1@samsung.com) : restoring scroll points on FrameLoadFinish. */
#endif
#define ENABLE_TIZEN_WEBKIT2_MEMORY_SAVING_MODE 1 /*Eunsol Park(eunsol47.park@samsung.com) : Used for lower memory usage profiling*/
#define ENABLE_TIZEN_WEBKIT2_CONTEXT_X_WINDOW 1 /* Changhyup Jwa(ch.jwa@samsung.com) : WebProcess cannot access to evas, so it needs to obtain window id to get window's orientation. Default root window has sync issue. */
#define ENABLE_TIZEN_WEBKIT2_FOR_MOVING_TEXT_SELECTION_HANDLE_FROM_OSP 1 /* Yuni Jeong(yhnet.jung@samsung.com) : Patchs for moving text selection handle from OSP */
#define ENABLE_TIZEN_WEBKIT2_FIX_INVAlID_SCROLL_FOR_NEW_PAGE 1 /* Jongseok Yang (js45.yang@samsung.com) : Patch to fix the invalid scroll position by open source patch */
#define ENABLE_TIZEN_WEBKIT2_TEXT_SELECTION_NOT_PAINT_SELECTION_FOR_INPUTBOX 1 /* Prathmesh Manurkar(prathmesh.m@samsung.com) : Patch for not painting the selection for Contents Inside the input.Selection is not painted when the focus in outside the input box */
#define ENABLE_TIZEN_WEBKIT2_INPUT_FIELD_ZOOM 1 /* Bunam Jeon(bunam.jeon@samsung.com) : Implement input field zoom animation */
#define ENABLE_TIZEN_WEBKIT2_COPY_MENU_FOR_SPECIFIC_SCHEME_LINK 1 /* Jongseok Yang(js45.yang@samsung.com) : Support copy menu for specific scheme link */
#define ENABLE_TIZEN_WEBKIT2_CONTEXT_MENU_WEB_SEARCH 1 /* Taeyun An (ty.an@samsung.com) : Support tizen dedicated web search operation */
#define ENABLE_TIZEN_WEBKIT2_CONTROL_EFL_FOCUS 1 /* Jongseok Yang(js45.yang@samsung.com), Taeyun An(ty.an@samsung.com) : the specific operation for EFL focus is required because webview feature like IME and selection depends on EFL focus */
#define ENABLE_TIZEN_WEBKIT2_PREVENT_LONG_PRESS 1 /* Yuni Jeong (yhnet.jung@samsung.com) : Prevent long press operation */
#define ENABLE_TIZEN_WEBKIT2_PREVENT_DOUBLE_TAP 1 /* Yuni Jeong (yhnet.jung@samsung.com) : Prevent double tap operation */
#define ENABLE_TIZEN_WEBKIT2_PREVENT_ZOOM 1 /* Hyunsoo Shim (hyunsoo.shim@samsung.com) : Prevent zoom operation */
#define ENABLE_TIZEN_WEBKIT2_SET_HOME_DIRECTORY 1 /* Jongseok Yang(js45.yang@samsung.com), Taeyun An(ty.an@samsung.com) : Request from Browser */
#define ENABLE_TIZEN_WEBKIT2_TOUCH_EVENT_TIMER 1 /* Hyunsoo Shim (hyunsoo.shim@samsung.com : Set touch event timer for touch performance*/
#define ENABLE_TIZEN_WEBKIT2_TOUCH_CANCEL 1 /* Sanghyup Lee (sh53.lee@samsung.com) : Support touch cancel event */
#define ENABLE_TIZEN_WEBKIT2_HIDE_URL_BAR 1 /* Hyunsoo Shim (hyunsoo.shim@samsung.com : Support hide url bar */
#define ENABLE_TIZEN_WEBKIT2_BROWSER_TOOLBAR_OVERLAP_EXCEPTION 1 /* Yuni Jeong (yhnet.jung@samsung.com) : Prevent to overlap with browser toolbar */
#define ENABLE_TIZEN_WEBKIT2_CHANGEABLE_UI_FOR_FOCUS_RING 1 /* Yuni Jeong (yhnet.jung@samsung.com) : Support tizen changeable ui for focus ring */
#define ENABLE_TIZEN_WEBKIT2_CHANGEABLE_UI_FOR_TEXT_SELECTION 1 /* Bhagirathi Satpathy (bhagirathi.s@samsung.com) : Support tizen changeable ui for Text Selection */
#define ENABLE_TIZEN_WEBKIT2_CHANGEABLE_UI_FOR_CARET 1 /* Bhagirathi Satpathy (bhagirathi.s@samsung.com) : Support tizen changeable ui for Caret */
#define ENABLE_TIZEN_WEBKIT2_TEXT_SELECTION_TEXT_CURSOR 0 /* Suhas Nagarmutt Kamath(suhas.kn@samsung.com) : To support text cursor ui on the left and right sides of selection area */
#define ENABLE_TIZEN_WEBKIT2_PREVENT_TEXT_SELECTION_MAGNIFIER 1  /* Suhas Nagarmutt Kamath (suhas.kn@samsung.com) : Enable/Disabling Text Selection magnifier */
#define ENABLE_TIZEN_WEBKIT2_SCROLL_POSITION_BUG_FIX 1 /* Dariusz Frankiewicz (d.frankiewic@samsung.com), Sanghyup Lee (sh53.lee@samsung.com : Fix wrong scroll position when user enter some text on input field. */
#endif /* ENABLE(TIZEN_WEBKIT2) */

/* When displaying menu list using menu icon, a additional scrollbar is displayed in the screen center
   So, this patch is to remove the logic for a additional scrollbar */
#define ENABLE_TIZEN_FIX_DRAWING_ADDITIONAL_SCROLLBAR 1 /* Jongseok Yang(js45.yang@samsung.com) */

#define ENABLE_TIZEN_FORCE_LAYOUT_FOR_SETTINGS_PAGE 1 /*Mayur Kankanwadi (mayurk.vk@samsung.com) Fix for P140625-06060 : Force a layout to make postlayout tasks to complete*/

/*
  * Problem : An additional scrollbar is displayed in the screen center when loading is finished as auto fitting.
  * Solution : Remove the Widget::frameRectsChanged for EFL port. */
#define ENABLE_TIZEN_FIX_SCROLLBAR_FOR_AUTO_FITTING 1 /* Jongseok Yang(js45.yang@samsung.com) */

/* The scrollbar just for ScrollView is disaplyed.
   The scrollbar for other object is not displayed. (ex. DIV scrollbar) */
#define ENABLE_TIZEN_DISPLAY_SCROLLBAR_JUST_FOR_SCROLLVIEW 1 /* Jongseok Yang(js45.yang@samsung.com) */

/*
  * Problem : Multiple History Items are getting created in History Item Tree for a error page, when you press Back or Forward Key,intead of reading present history.
  * Becuse error page is loaded with Type standard evnethough we press back/forward.
  * Solution : FrameLoader::Load is called with Back/Forward Load type when press back/forward. */
#define ENABLE_TIZEN_BACKFORWARD_LIST_ERRORPAGE 1 /* Basavaraj P S(basavaraj.ps@samsung.com) */
#define ENABLE_TIZEN_BACKFORWARD_LIST_CLEAR 1 /* Basavaraj P S(basavaraj.ps@samsung.com),  Exposing API to clear back forward list of a page*/
#define ENABLE_TIZEN_EXTERNAL_URL_CONTAINING_USERNAME_PASSWORD 1 /* Basavaraj P S(basavaraj.ps@samsung.com),  Support to authenticate external URL containing username and password*/

#if USE(ACCELERATED_COMPOSITING)
#define ENABLE_TIZEN_ACCELERATED_COMPOSITING 1 /* Hyowon Kim(hw1008.kim@samsung.com) */
#define ENABLE_TIZEN_FIX_REPAINTING_BUG_OF_COMPOSITED_LAYER 1 /* Hurnjoo Lee(hurnjoo.lee@samsung.com) : Fix a bug of rendering of Accelerated Compositing */
#define ENABLE_TIZEN_FIX_DEPTH_TEST_BUG_OF_WEBGL 1 /* YongGeol Jung(yg48.jung@samsung.com) : Fix webgl bug related to depth-test */
#if USE(TEXTURE_MAPPER)
#define WTF_USE_TIZEN_TEXTURE_MAPPER 1
#endif

#if ENABLE(TIZEN_ACCELERATED_2D_CANVAS) && ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
#define ENABLE_TIZEN_ACCELERATED_2D_CANVAS_EFL 1 /* Kyungjin Kim(gen.kim@samsung.com), Hyunki Baik(hyunki.baik@samsung.com) : Accelerated 2D Canvas */
#endif
#if ENABLE(TIZEN_ACCELERATED_2D_CANVAS_EFL)
#if CPU(ARM)
#define ENABLE_TIZEN_CANVAS_CAIRO_GLES_RENDERING 1 /* Kyungjin Kim(gen.kim@samsung.com), Hyunki Baik(hyunki.baik@samsung.com) : canvas cairo/GLES rendering */
#else
#define ENABLE_TIZEN_CANVAS_SURFACE_LOCKING 1 /* Christophe Dumez(christophe.dumez@intel.com) : Accelerated 2D Canvas using EGL surface locking */
#endif
#endif
#if ENABLE(TIZEN_CANVAS_CAIRO_GLES_RENDERING)
#define ENABLE_TIZEN_USE_XPIXMAP_DECODED_IMAGESOURCE 1 /* Byeongha Cho(byeongha.cho@samsung.com) : Use pixmap surface to decode image and share the buffer between CPU and GPU */
#define ENABLE_TIZEN_CAIROGLES_MULTISAMPLE_WORKAROUND 1
#define ENABLE_TIZEN_2D_CANVAS_JS_SUSPEND 0 /* Hyunki Baik(hyunki.baik@samsung.com):  Suspend JavasScript during UIProcess access the shared platformSurface for 2D Canvas */
#define ENABLE_TIZEN_CANVAS_CAIRO_GLES_SURFACE_CREATION_FIX 0 /* Insoon Kim(is46.kim@samsung.com): cairo_gl_surface_set_binding_texture() is deprecated. Changed to use cairo_gl_surface_create_for_texture() for Tizen2.4 */
#define ENABLE_TIZEN_CANVAS_CAIRO_GLES_OFFSCREEN_BUFFER 0 /* Insoon Kim(is46.kim@samsung.com) : Use offscreen buffer for rendering */
#if ENABLE(TIZEN_CANVAS_CAIRO_GLES_RENDERING)
#define ENABLE_TIZEN_CANVAS_CAIRO_GLES_OFFSCREEN_BUFFER_GL_SURFACE 0 /* Insoon Kim(is46.kim@samsung.com) : Use Cairo GL Surface for offscreen buffer [0:similar surface, 1:gl surface]*/
#endif
#endif

#if ENABLE(TIZEN_WEBKIT2)
#define ENABLE_TIZEN_MOBILE_WEB_PRINT 1 /* Hyunsoo Shim(hyunsoo.shim@samsung.com) : Mobile Web Print for AC layer */
#endif
#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
#define ENABLE_TIZEN_ACCELERATED_COMPOSITING_PLUGIN_LAYER_EFL 1 /* Hyunsoo Shim(hyunsoo.shim@samsung.com) : Accelerated Compositing plugin layer */
#if ENABLE(REQUEST_ANIMATION_FRAME)
#define ENABLE_TIZEN_SYNC_REQUEST_ANIMATION_FRAME 1 /* YongGeol Jung(yg48.jung@samsung.com) : Synchronize callback function of ScriptedAnimationController between UI Process and Web Process to 1 : 1 */
#endif
#define ENABLE_TIZEN_FIX_FOR_PIXMAN_SIZE_LIMITATION 1 /* YongGeol Jung(yg48.jung@samsung.com) : Fixed for size limitation of pixman */
#define ENABLE_TIZEN_SUPPORT_WEBPROVIDER_AC 1
#endif
#define ENABLE_TIZEN_CSS_FIXED_ACCELERATION 1 /* Jaehun Lim(ljaehun.lim@samsung.com) : Accelerated CSS FIXED elements */
#define ENABLE_TIZEN_EVAS_GL_DIRECT_RENDERING 1 /* Hyowon Kim(hw1008.kim@samsung.com) */
#if ENABLE(OVERFLOW_SCROLLING)
#define ENABLE_TIZEN_CSS_OVERFLOW_SCROLL_ACCELERATION 1 /* ChangSeok Oh(midow.oh@samsung.com) : Accelerated CSS overflow:scroll elements */
#define ENABLE_TIZEN_CSS_OVERFLOW_SCROLL_SPLIT 1 /* Jaehun Lim (ljaehun.lim@samsung.com) : Split x and y offsets */
#if ENABLE (TIZEN_CSS_OVERFLOW_SCROLL_ACCELERATION)
#define ENABLE_TIZEN_CSS_OVERFLOW_SCROLL_FIX_WRONG_CLIPPING 1 /* Younghwan Cho (yhwan.cho@samsung.com) : patch for wrong clipping from 'Bug 91117 - Add support for compositing the contents of overflow:scroll areas', test-code */
#endif
#define ENABLE_TIZEN_INPUT_BOX_SCROLL 1 /* Prathmesh Manurkar(prathmesh.m@samsung.com) : Added for scrolling contents of input box */
#define ENABLE_TIZEN_CSS_OVERFLOW_SCROLL_ALLOW_SCROLLING 1 /* Hyeonji Kim(hyeonji.kim@samsung.com) : Allow the box's scrolling in a given direction only when it overflows */
#endif
#if ENABLE(TIZEN_WEBKIT2)
#if ENABLE(OVERFLOW_SCROLLING)
#define ENABLE_TIZEN_CSS_OVERFLOW_SCROLL_ACCELERATION_ON_UI_SIDE 1 /* ChangSeok Oh(midow.oh@samsung.com) : Accelerated scrolling CSS overflow:scroll elements on UI Process. */
#if ENABLE(TIZEN_CSS_OVERFLOW_SCROLL_ACCELERATION_ON_UI_SIDE)
#define ENABLE_TIZEN_CSS_OVERFLOW_SCROLL_SCROLLBAR 1 /* Jaehun Lim(ljaehun.lim@samsung.com : paint scrollbars for overflow areas */
#endif
#endif
#else
#undef ENABLE_TIZEN_ACCELERATED_COMPOSITING_PLUGIN_LAYER_EFL /* Hurnjoo Lee(hurnjoo.lee@samsung.com) : Disable the feature for WebKit1 */
#endif
#endif

#if ENABLE(PAGE_VISIBILITY_API)
#define ENABLE_TIZEN_PAGE_VISIBILITY_API 1 /* Kihong Kwon(kihong.kwon@samsung.com) : Support the Page Visibility API */
#endif

#if ENABLE(SPELLCHECKING)
#define ENABLE_TIZEN_WEBKIT2_ASYNCHRONOUS_SPELLCHECKING 1 /* Grzegorz Czajkowski: WebKitTestRunner needs it to run tests from editing/spelling directory, imported from https://bugs.webkit.org/show_bug.cgi?id=81042. */
#endif

#define ENABLE_TIZEN_META_CHARSET_PARSER 1 /* Kangil Han(kangil.han@samsung.com) : Parser fix for <http://www.onnuritour.com>. Working on <https://bugs.webkit.org/show_bug.cgi?id=102677>. */
#define ENABLE_TIZEN_CSS_INSET_BORDER_BUG 1 /* Byeonga Cho(byeongha.cho@samsung.com : Fix CSS inset border bug in x86. This is workaround. FIX ME */
#define ENABLE_TIZEN_SKIP_DRAWING_SHADOW_BLUR_SCRATCH_BUFFER 1 /* YongGeol Jung(yg48.jung@samsung.com) */
#define ENABLE_TIZEN_PERFORMANCE_CANVAS_SCALED_SHADOW_BLUR 1 /* YongGeol Jung(yg48.jung@samsung.com) : Performance improvement for canvas shadow blur. */
#define ENABLE_TIZEN_DEFER_FIXED_LAYER_UPDATE 1 /* YongGeol Jung(yg48.jung@samsung.com) */
#define ENABLE_TIZEN_USE_SW_PATH_FOR_BLUR_FILTER 1 /* YongGeol Jung(yg48.jung@samsung.com) */

#define ENABLE_TIZEN_DFG_UPDATE_TAG_FOR_REGEXP_TEST 1 /* Hojong Han(hojong.han@samsung.com) : Fix crash while using Naver map. Working on <https://bugs.webkit.org/show_bug.cgi?id=105756> */
#define ENABLE_TIZEN_DFG_STORE_CELL_TAG_FOR_ARGUMENTS 1 /* Hojong Han(hojong.han@samsung.com) : Store cell tag for arguments */
#define ENABLE_TIZEN_JSC_CACHEFLUSH_PAGE_BY_PAGE 1 /* Hojong Han(hojong.han@samsung.com) : Fix YarrJIT crash */
#define ENABLE_TIZEN_SET_PROPERTY_ATTRIBUTE_CORRECTLY 1 /* Hojong Han(hojong.han@samsung.com) : Fix overwritting Read-only property, set from WRT */
#define ENABLE_TIZEN_JSC_SKIP_PROPERTY_TABLE_ERROR 1 /* Hojong Han(hojong.han@samsung.com) : Skip property table error */
#define ENABLE_TIZEN_GENERATE_COMBINED_UTC_LOCAL 1 /* Hojong Han(hojong.han@samsung.com) : Simplify date handling code */
#define ENABLE_TIZEN_JSC_PARALLEL_GC_THREAD_CREATION_CHECK 1 /* SangGyu Lee (sg5.lee@samsung.com) */
#define ENABLE_TIZEN_JSC_BLOCK_FREEING_THREAD_CREATION_CHECK 1 /* SangGyu Lee (sg5.lee@samsung.com) */
#define ENABLE_TIZEN_WTF_GIVING_MORE_CHANCES_TO_URANDOM 1 /* SangGyu Lee (sg5.lee@samsung.com) */
#define ENABLE_TIZEN_UPDATE_TIMEZONE_INFO 1 /* Hojong Han(hojong.han@samsung.com) : Sync timezone before getting local time */
#define ENABLE_TIZEN_JSC_REGEXP_TEST_CACHE 1 /* Hojong Han(hojong.han@samsung.com) : Cache results of regexp test */
#if CPU(ARM_THUMB2)
#define ENABLE_TIZEN_JSC_USE_SDIV_UDIV_ARITHDIV_ARMV7_THUNKS 1 /* VenkatNJ(venkat.nj@samsung.com) : Use sdiv and udiv for integer divide and modulo in armv7 Use special thunks for ARMv7 for ArithDiv operations */
#define ENABLE_TIZEN_JSC_INLINE_STRING_EQUALITY 1 /* VenkatNJ(venkat.nj@samsung.com) : Inline string comparision in DFG */
#endif
#define ENABLE_TIZEN_JSC_VALUE_PROFILE_CACHE 1 /* Hojong Han(hojong.han@samsung.com) : Use value profile cache to compile DFG JIT earlier, This should be used with ENABLE_VALUE_PROFILER */
#define ENABLE_TIZEN_JSC_WRITE_VALID_CALLERFRAME_TAG 1 /* Eunji Jeong(eun-ji.jeong@samsung.com): Write the valid tag to callerframe in callframe header */
#define ENABLE_TIZEN_JSC_SEED_GC_ACTIVITY_CALLBACK 1 /* VenkatNJ(venkat.nj@samsung.com) : Seed timer for GC Activity callback if no collection is performed */

#define ENABLE_TIZEN_WRT_LAUNCHING_PERFORMANCE 1 /* Byungwoo Lee(bw80.lee@samsung.com) : Local patches to enhance web app launching performance */
#define ENABLE_TIZEN_PROCESS_PERMISSION_CONTROL 1 /* Yunchan Cho(yunchan.cho@samsung.com), Ryuan Choi(ryuan.choi@samsung.com) : Change smack label of launched webkit processes */
#define ENABLE_TIZEN_TEXT_CODEC_MEMORY_REDUCTION 1 /*KyungTae Kim(ktf.kim@samsung.com) : Share Encode & Decode buffer for TextCodecUTF8 for memory reduction */
#define ENABLE_TIZEN_ADJUST_CONTENTS_SIZE_FOR_MINUS_X_WORKAROUND 1 /*KyungTae Kim(ktf.kim@samsung.com) : Workaround patch that adjusts contents size of minus x position contents */
#define ENABLE_TIZEN_EWK_CONTEXT_CACHE_MANAGER_NULL_CHECK_WORKAROUND 1 /* KyungTae Kim(ktf.kim@samsung.com) : Add null check to fix crash issue. */
#define ENABLE_TIZEN_FRAMEVIEW_NULL_CHECK_WORKAROUND 1 /* KyungTae Kim(ktf.kim@samsung.com) : Add null check to fix crash issue. */
#define ENABLE_TIZEN_EDITING_IGNORE_NONVISIBLE 1 /*KyungTae Kim(ktf.kim@samsung.com) : When checking styles for editing, ignore the element without renderer, to prevent to get 'mixed' although all the visible elements are 'true'*/
#define ENABLE_TIZEN_SQLITE_CRASH_FIX_WORKAROUND 1 /*KyungTae Kim(ktf.kim@samsung.com) : To Fix the crash occured in SQLiteStatement.cpp - inprofer use CString in LOG. For now, fix the log not to use the String, then after further investigation, if this bug is fixed correctly, this patch is needed to remove*/
#define ENABLE_TIZEN_FASTPATH_FOR_KNOWN_REGEXP 1 /*KyungTae Kim(ktf.kim@samsung.com) : Make a fast path for common regular expression patterns used in common js libraries*/


#define ENABLE_TIZEN_DO_NOT_APPLY_SCROLLOFFSET_FOR_DELEGATESSCROLLING 1 /* Eunmi Lee(eunmi15.lee@samsung.com) : Fix the wrong position of hitTest result when we do hit test in the subFrame (It should be contributed to the opensource) */

#define ENABLE_TIZEN_WEBKIT2_FORM_DATABASE 1 /* Changhyup Jwa(ch.jwa@samsung.com) : Form database support */

#if ENABLE(TIZEN_WEBKIT2_FORM_DATABASE)
#define ENABLE_TIZEN_WEBKIT2_AUTOFILL_PROFILE_FORM 1 /* Prathmesh Manurkar(prathmesh.m@samsung.com) : Profile Form support added */
#endif

#define ENABLE_TIZEN_RESET_CACHEDFRAME_CONTENTSIZE_ON_NAVIGATION 1 /* Divakar.A (divakar.a@samsung.com) : Resetting contentSize of CachedFrameView on navigation. This flag will be removed once latest opensource merge is done. */

#define ENABLE_TIZEN_CSP 1 /* Seonae Kim(sunaeluv.kim@samsung.com) : Support CSP to provide EWK API to web applicatoin */
#define ENABLE_TIZEN_SUPPORT_MHTML 1 /* KwangYong Choi(ky0.choi@samsung.com) : Add support for exporting MHTML data */

#define ENABLE_TIZEN_MHTML_CSS_IMPORT_URL_RELATIVE_TO_PARENT 1 /*Santosh Mahto(santosh.ma@samsung.com) : URL of stylesheet specified in @import should be relative to parent Stylesheet  in page Serializing */

#define ENABLE_TIZEN_MHTML_CSS_ADD_BACKGROUND_PROPERTY_TO_SHORT_HAND 1 /* Devdatta Deshpande (d.deshpande@partner.samsung.com): The case for background short hand properties was missing in StylePropertySet::asText */
#define ENABLE_TIZEN_MHTML_IGNORE_ADDING_IMAGE_TO_RESOURCE_ON_ERROR 1 /* Devdatta Deshpande (d.deshpande@partner.samsung.com): The Base64 encoded broken image was added when there is an error in the page, this should be avoided. */
#define ENABLE_TIZEN_MHTML_PREPEND_DOCTYPE_TO_HTML_MARKUP 1 /* Devdatta Deshpande (d.deshpande@partner.samsung.com): Added missing DocType in saved page markup. */
#define ENABLE_TIZEN_MHTML_CLOSE_META_TAG_FOR_XHTML_DOCUMENT 1 /* Devdatta Deshpande (d.deshpande@partner.samsung.com): Close empty meta tag in case of XHTML pages see: http://www.w3.org/TR/xhtml1/#h-4.6 and http://www.w3.org/TR/xhtml1/#C_2 */
#define ENABLE_TIZEN_RANGE_OFFSET_FIX_FOR_SAME_LENGTH_INSERTION 1 /* Devdatta Deshpande (d.deshpande@partner.samsung.com): Corrected the condition to >= as it was causing a issue when a text is replaced with another text of same length for the node on any border of the selection */

#define ENABLE_TIZEN_MHTML_STYLESHEET_NULL_CHECK 1 /* Santosh Mahto(santosh.ma@samsung.com) : NULL check for Empty styleSheet in Pageserializer.cpp */
#define ENABLE_TIZEN_OFFLINE_PAGE_SAVE 1 /* Nikhil Bansal (n.bansal@samsung.com) : Tizen feature for offline page save */

#define ENABLE_TIZEN_FIND_STRING 1 /* Jinwoo Song(jinwoo7.song@samsung.com) : Fix the bug to enable searching the web page which has the 'webkit-user-select: none' CSS property and value. */

#define ENABLE_TIZEN_GET_EXTERNAL_RESOURCES_IN_MHTML_FROM_NETWORK 1 /* Praveen(praveen.ks@samsung.com) : Allow external resources in MHTML file to be fetched from network rather than failing them */

#define ENABLE_TIZEN_MAIN_THREAD_SCHEDULE_DISCARD_DUPLICATE_REQUEST 1 /* Jihye Kang(jye.kang@samsung.com) : Fix lockup while doing stress test for filewriter */

#define ENABLE_TIZEN_MHTML_CSS_MEDIA_RULE_RESOURCE 1 /* Santosh Mahto(santosh.ma@samsung.com) : Collect the subresource specified in css media rule in mhtml PageSerializing */
#define ENABLE_TIZEN_ANIMATION_TIME_FROM_START 1 /* Nikhil Sahni(nikhil.sahni@samsung.com) : Correcting the css animation to use start time rather than using offset */
#define ENABLE_TIZEN_ANIMATION_DELAY_START 1 /* Kyungjin Kim(gen.kim@samsung.com) : give delay before starting animation to reduce frame skip */
#define ENABLE_TIZEN_FULLSCREEN_API 1 /* Jongseok Yang(js45.yang@samsung.com) : Implement the smart function for fullscreen API */
#define ENABLE_TIZEN_CSS_THEME_COLOR 1 /* Ankit PAndey(ankt.pandey@samsung.com) : Implement Theme Color Meta Tag */

#if ENABLE(TOUCH_EVENT_TRACKING)
#define ENABLE_TIZEN_TOUCH_EVENT_TRACKING 1 /* Eunmi Lee (eunmi15.lee@samsung.com) : Share the touch event rects with ui process. */
#endif

#define ENABLE_TIZEN_WEBSOCKET_ADD_CANCELLABLE 1 /* Praveen (praveen.ks@samsung.com) : Add cancellable to input stream and cancel the cancellable when we close the platform */

#if ENABLE(WORKERS)
#define ENABLE_TIZEN_WORKERS 1 /* Jihye Kang(jye.kang@samsung.com) : Use allowUniversalAccessFromFileURLs setting for workers */
#endif

#define ENABLE_TIZEN_GET_COOKIES_FOR_URL 1 /*Praveen (praveen.ks@samsung.com) : Add an API to get cookies for a particular URL */

#define ENABLE_TIZEN_HW_MORE_BACK_KEY 1 /* Jongseok Yang(js45.yang@samsung.com), Kwangyong Choi (ky0.choi@samsung.net) : Add support for HW more/back keys */

#define ENABLE_TIZEN_AVOID_NORMALIZATION_FOR_FILE 1 /* Loader Team */

#if ENABLE(TIZEN_GSTREAMER_VIDEO)
#define ENABLE_APP_LOGGING_FOR_MEDIA 1 /* Mahesha N(mahesha.n@samsung.com) : Use App logging for HTML Audio and Video Features. */
#if CPU(X86) || CPU(X86_64)
#define ENABLE_TIZEN_USE_HW_VIDEO_OVERLAY_IN_FULLSCREEN 0 /* Eojin ham(eojin.ham@samsung.com) : Use HW video overlay when play video in fullscreen. */
#else
#define ENABLE_TIZEN_USE_HW_VIDEO_OVERLAY_IN_FULLSCREEN 1 /* Eojin ham(eojin.ham@samsung.com) : Use HW video overlay when play video in fullscreen. */
#endif
#endif

#if ENABLE(TIZEN_USE_HW_VIDEO_OVERLAY_IN_FULLSCREEN) || ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
#if !USE(ACCELERATED_VIDEO_VAAPI)
#define ENABLE_TIZEN_SKIP_COMPOSITING_FOR_FULL_SCREEN_VIDEO 1 /* YongGeol Jung(yg48.jung@samsung.com) : Composite full screen layer only for native full screen video play-back */
#endif
#endif

#if USE(GRAPHICS_SURFACE) || ENABLE(TIZEN_CANVAS_GRAPHICS_SURFACE)
#if !ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
#define ENABLE_TIZEN_CHECK_XIMAGE 1 /* Hyeonji Kim(hyeonji.kim@samsung.com) : Check XImage validation */
#endif
#endif

#define ENABLE_TIZEN_UPDATE_CHARSET_WHEN_REUSING_XHR_OBJECT 1 /* Praveen(praveen.ks@samsung.com) : Update xhr object's charset with new response's charset */

#if ENABLE(VIBRATION)
#define ENABLE_TIZEN_VIBRATION 1 /* Kamil Lysik (k.lysik@partner.samsung.com) : Vibration API */
#endif

#if ENABLE(POINTER_LOCK)
#define ENABLE_TIZEN_POINTER_LOCK 1 /* Jiyeon Kim (jiyeon0402.kim@samsung.com) : Enable pointer lock feature */
#endif

#if ENABLE(DIALOG_ELEMENT)
#define ENABLE_TIZEN_HTML5_DIALOG_ELEMENT 1 /* Shreeram Kushwaha (shreeram.k@samsung.com) : Enable dialog element */
#endif

#define ENABLE_TIZEN_SOUP_FEATURES 1 /* Santosh Patil (ss.patil@partner.samsung.com) : On/Off libSoup Features */
#define ENABLE_TIZEN_BACK_FORWARD_LIST_STORE_RESTORE 1 /*Santosh Patil (ss.patil@partner.samsung.com): Added support for BackForward List Store and Restore*/

#define ENABLE_TIZEN_WAKEUP_GPU_ON_MOUSE_DOWN 1 /* Eunmi Lee (eunmi15.lee@samsung.com) : Wake up GPU on mouse down to improve touch responsiveness. */
#define ENABLE_TIZEN_ENABLE_MINIMUM_2_CPU_ON_MOUSE_DOWN 1 /* Eunmi Lee (eunmi15.lee@samsung.com) : Enable minimum 2 CPU on mouse down to improve touch responsiveness. */
#define ENABLE_TIZEN_BROWSER_DASH_MODE 1 /* Kyungjin Kim (gen.kim@samsung.com) : Enable dash mode interface */
#if ENABLE(TIZEN_BROWSER_DASH_MODE)
#define ENABLE_TIZEN_BROWSER_WEBGL_DASH_MODE 0 /* Prabhavathi (prabha.p@samsung.com) : Seperate dashmode flag for webgl*/
#endif

#define ENABLE_TIZEN_MALLOC_TRIM_EXPLICITLY 1 /* Dong-Gwan Kim (donggwan.kim@samsung.com) : free memory allocated by system explicitly by calling malloc_trim */

#define ENABLE_TIZEN_SET_SOUP_MESSAGE_USING_SYNC_CONTEXT 1 /* Keunsoon Lee (keunsoon.lee@samsung.com) : set SYNC CONTEXT true on soup_message for handling sync requests in libsoup  */

#define ENABLE_TIZEN_DISABLE_MEMORY_CACHE_FOR_FILE_SCHEME 1 /* Praveen (praveen.ks@samsung.com) : Don't use memory cache for resources with file scheme URLs */

#define ENABLE_TIZEN_CONSTRUCT_TAG_NODE_LIST_WITH_SNAPSHOT 1 /* Dong-Gwan Kim (donggwan.kim@samsung.com) : create snapshot with TagNodeList in order to speed up DOM Search */
#define ENABLE_TIZEN_CACHE_LAST_RESULT_OF_CSS_SELECTOR_QUERY 1 /* Dong-Gwan Kim (donggwan.kim@samsung.com) : Cache the last result of each CSS selector query for performance. */
#define ENABLE_TIZEN_PRIVATE_BROWSING 1 /*Sungman Kim (ssungmai.kim@samsung.com) : Support private browsing mode to Tizen */
#define ENABLE_TIZEN_JAVASCRIPT_POPUP_REPLY_HANDLING 1 /* Bhanukrushana Rout (b.rout@samsung.com) : to match send and reply message count for javascript popup */
#define ENABLE_TIZEN_CSS_SELECTOR_READ_ONLY_READ_WRITE 1 /* Jihye Kang (jye.kang@samsung.com) : Apply read-only read-write selector for editable element http://www.w3.org/TR/html5/disabled-elements.html#selector-read-write */
#define ENABLE_TIZEN_LIMIT_WEBPOPUPMENU_UPDATE 1 /*Mayur Kankanwadi (mayurk.vk@samsung.com) : Fix for P140707-04790 - Limit webpopupmenu updates on recalcstyle.*/
#define ENABLE_TIZEN_ALWAYS_TOUCH_EVENT 1 /* Jongseok Yang (js45.yang@samsung.com) : touch event should be generated always for CSS active */

#define ENABLE_TIZEN_BACKGROUND_COLOR_API 1 /* Hunseop Jeong (hs85.jeong@samsung.com) : Implement the ewk_view_bg_color_set() */

/* Tizen Lite */
#define ENABLE_TIZEN_LITE_PAGECACHE_OPTIMIZATION 1 /* Divakar (divakar.a@samsung.com) : adjust page cache size for Tizen lite */
#define ENABLE_TIZEN_LITE_VIEW_IMAGE_IN_SAME_WINDOW 1 /* Gurpreet Kaur k.gurpreet@samsung.com) : For viewing image in same window, accroding Kiran UX guidelines. */


#ifdef ENABLE_TIZEN_WAKEUP_GPU_ON_MOUSE_DOWN
#undef ENABLE_TIZEN_WAKEUP_GPU_ON_MOUSE_DOWN
#endif

#ifdef ENABLE_TIZEN_JSC_VALUE_PROFILE_CACHE
#undef ENABLE_TIZEN_JSC_VALUE_PROFILE_CACHE
#endif
#ifdef ENABLE_TIZEN_JSC_REGEXP_TEST_CACHE
#undef ENABLE_TIZEN_JSC_REGEXP_TEST_CACHE
#endif

#define ENABLE_TIZEN_JSC_EAGER_COMPILATION 1 /* Eunji Jeong(eun-ji.jeong@samsung.com) : Fully generate the code before benchmark execution */

#define ENABLE_TIZEN_OPEN_IMAGE_IN_CURRENT_TAB 1 /* Karol Pawlowski (k.pawlowski@samsung.com): Fix opening image in current tab issue */
#define ENABLE_TIZEN_LITE_MEMORYCACHE_OPTIMIZATION 1 /* Byeongha Cho (byeongha.cho@samsung.com): Memory Cache size optimiztion for lite device */

#define ENABLE_TIZEN_BASELINE_POSITION_FIX 1 /* Insoon Kim (is46.kim@samsung.com) : Fix the wrong baseline position for g character. (P150613-04028) */
#define ENABLE_TIZEN_LITE_TEXTURE_RESIZE_OPTIMIZATION 1 /* Srikanth Chevalam (chevalam.s@samsung.com) : Texture resize optimization for Tizen lite for WebGL */
#define ENABLE_TIZEN_STOP_SENDING_IPCMSG_WHEN_PROCESS_NOT_EXIST 1 /* Bhanukrushana Rout (b.rout@samsung.com : Fix to browser crash when IPC message sent during no-existence of web process*/

#endif /* PLATFORM(TIZEN) */
/* ==== OS() - underlying operating system; only to be used for mandated low-level services like 
   virtual memory, not to choose a GUI toolkit ==== */

/* OS(ANDROID) - Android */
#ifdef ANDROID
#define WTF_OS_ANDROID 1
#endif

/* OS(AIX) - AIX */
#ifdef _AIX
#define WTF_OS_AIX 1
#endif

/* OS(DARWIN) - Any Darwin-based OS, including Mac OS X and iPhone OS */
#ifdef __APPLE__
#define WTF_OS_DARWIN 1

#include <Availability.h>
#include <AvailabilityMacros.h>
#include <TargetConditionals.h>
#endif

/* OS(IOS) - iOS */
/* OS(MAC_OS_X) - Mac OS X (not including iOS) */
#if OS(DARWIN) && ((defined(TARGET_OS_EMBEDDED) && TARGET_OS_EMBEDDED) \
    || (defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE)                 \
    || (defined(TARGET_IPHONE_SIMULATOR) && TARGET_IPHONE_SIMULATOR))
#define WTF_OS_IOS 1
#elif OS(DARWIN) && defined(TARGET_OS_MAC) && TARGET_OS_MAC
#define WTF_OS_MAC_OS_X 1

/* FIXME: These can be removed after sufficient time has passed since the removal of BUILDING_ON / TARGETING macros. */

#define ERROR_PLEASE_COMPARE_WITH_MAC_OS_X_VERSION_MIN_REQUIRED 0 / 0
#define ERROR_PLEASE_COMPARE_WITH_MAC_OS_X_VERSION_MAX_ALLOWED 0 / 0

#define BUILDING_ON_LEOPARD ERROR_PLEASE_COMPARE_WITH_MAC_OS_X_VERSION_MIN_REQUIRED
#define BUILDING_ON_SNOW_LEOPARD ERROR_PLEASE_COMPARE_WITH_MAC_OS_X_VERSION_MIN_REQUIRED
#define BUILDING_ON_LION ERROR_PLEASE_COMPARE_WITH_MAC_OS_X_VERSION_MIN_REQUIRED

#define TARGETING_LEOPARD ERROR_PLEASE_COMPARE_WITH_MAC_OS_X_VERSION_MAX_ALLOWED
#define TARGETING_SNOW_LEOPARD ERROR_PLEASE_COMPARE_WITH_MAC_OS_X_VERSION_MAX_ALLOWED
#define TARGETING_LION ERROR_PLEASE_COMPARE_WITH_MAC_OS_X_VERSION_MAX_ALLOWED
#endif

/* OS(FREEBSD) - FreeBSD */
#if defined(__FreeBSD__) || defined(__DragonFly__) || defined(__FreeBSD_kernel__)
#define WTF_OS_FREEBSD 1
#endif

/* OS(HURD) - GNU/Hurd */
#ifdef __GNU__
#define WTF_OS_HURD 1
#endif

/* OS(LINUX) - Linux */
#ifdef __linux__
#define WTF_OS_LINUX 1
#endif

/* OS(NETBSD) - NetBSD */
#if defined(__NetBSD__)
#define WTF_OS_NETBSD 1
#endif

/* OS(OPENBSD) - OpenBSD */
#ifdef __OpenBSD__
#define WTF_OS_OPENBSD 1
#endif

/* OS(QNX) - QNX */
#if defined(__QNXNTO__)
#define WTF_OS_QNX 1
#endif

/* OS(SOLARIS) - Solaris */
#if defined(sun) || defined(__sun)
#define WTF_OS_SOLARIS 1
#endif

/* OS(WINCE) - Windows CE; note that for this platform OS(WINDOWS) is also defined */
#if defined(_WIN32_WCE)
#define WTF_OS_WINCE 1
#endif

/* OS(WINDOWS) - Any version of Windows */
#if defined(WIN32) || defined(_WIN32)
#define WTF_OS_WINDOWS 1
#endif

#define WTF_OS_WIN ERROR "USE WINDOWS WITH OS NOT WIN"
#define WTF_OS_MAC ERROR "USE MAC_OS_X WITH OS NOT MAC"

/* OS(UNIX) - Any Unix-like system */
#if   OS(AIX)              \
    || OS(ANDROID)          \
    || OS(DARWIN)           \
    || OS(FREEBSD)          \
    || OS(HURD)             \
    || OS(LINUX)            \
    || OS(NETBSD)           \
    || OS(OPENBSD)          \
    || OS(QNX)              \
    || OS(SOLARIS)          \
    || defined(unix)        \
    || defined(__unix)      \
    || defined(__unix__)
#define WTF_OS_UNIX 1
#endif

/* Operating environments */

/* FIXME: these are all mixes of OS, operating environment and policy choices. */
/* PLATFORM(CHROMIUM) */
/* PLATFORM(QT) */
/* PLATFORM(WX) */
/* PLATFORM(GTK) */
/* PLATFORM(BLACKBERRY) */
/* PLATFORM(MAC) */
/* PLATFORM(WIN) */
#if defined(BUILDING_CHROMIUM__)
#define WTF_PLATFORM_CHROMIUM 1
#elif defined(BUILDING_QT__)
#define WTF_PLATFORM_QT 1
#elif defined(BUILDING_WX__)
#define WTF_PLATFORM_WX 1
#elif defined(BUILDING_GTK__)
#define WTF_PLATFORM_GTK 1
#elif defined(BUILDING_BLACKBERRY__)
#define WTF_PLATFORM_BLACKBERRY 1
#elif OS(DARWIN)
#define WTF_PLATFORM_MAC 1
#elif OS(WINDOWS)
#define WTF_PLATFORM_WIN 1
#endif

/* PLATFORM(IOS) */
/* FIXME: this is sometimes used as an OS switch and sometimes for higher-level things */
#if (defined(TARGET_OS_EMBEDDED) && TARGET_OS_EMBEDDED) || (defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE)
#define WTF_PLATFORM_IOS 1
#endif

/* PLATFORM(IOS_SIMULATOR) */
#if defined(TARGET_IPHONE_SIMULATOR) && TARGET_IPHONE_SIMULATOR
#define WTF_PLATFORM_IOS 1
#define WTF_PLATFORM_IOS_SIMULATOR 1
#endif

/* Graphics engines */

/* USE(CG) and PLATFORM(CI) */
#if PLATFORM(MAC) || PLATFORM(IOS)
#define WTF_USE_CG 1
#endif
#if PLATFORM(MAC) || PLATFORM(IOS) || (PLATFORM(WIN) && USE(CG))
#define WTF_USE_CA 1
#endif

/* USE(SKIA) for Win/Linux/Mac/Android */
#if PLATFORM(CHROMIUM)
#if OS(DARWIN)
#define WTF_USE_SKIA 1
#define WTF_USE_ICCJPEG 1
#define WTF_USE_QCMSLIB 1
#elif OS(ANDROID)
#define WTF_USE_SKIA 1
#define WTF_USE_LOW_QUALITY_IMAGE_INTERPOLATION 1
#define WTF_USE_LOW_QUALITY_IMAGE_NO_JPEG_DITHERING 1
#define WTF_USE_LOW_QUALITY_IMAGE_NO_JPEG_FANCY_UPSAMPLING 1
#else
#define WTF_USE_SKIA 1
#define WTF_USE_ICCJPEG 1
#define WTF_USE_QCMSLIB 1
#endif
#endif

#if PLATFORM(BLACKBERRY)
#define USE_SYSTEM_MALLOC 1
#define WTF_USE_MERSENNE_TWISTER_19937 1
#define WTF_USE_SKIA 1
#endif

#if PLATFORM(GTK)
#define WTF_USE_CAIRO 1
#define ENABLE_GLOBAL_FASTMALLOC_NEW 0
#endif


#if OS(WINCE)
#define WTF_USE_MERSENNE_TWISTER_19937 1
#endif

/* On Windows, use QueryPerformanceCounter by default */
#if OS(WINDOWS)
#define WTF_USE_QUERY_PERFORMANCE_COUNTER  1
#endif

#if OS(WINCE) && !PLATFORM(QT)
#define NOSHLWAPI      /* shlwapi.h not available on WinCe */

/* MSDN documentation says these functions are provided with uspce.lib.  But we cannot find this file. */
#define __usp10__      /* disable "usp10.h" */

#define _INC_ASSERT    /* disable "assert.h" */
#define assert(x)

#endif  /* OS(WINCE) && !PLATFORM(QT) */

#if PLATFORM(QT)
#ifndef WTF_USE_ICU_UNICODE
#define WTF_USE_QT4_UNICODE 1
#endif
#elif OS(WINCE)
#define WTF_USE_WINCE_UNICODE 1
#elif PLATFORM(GTK)
/* The GTK+ Unicode backend is configurable */
#else
#define WTF_USE_ICU_UNICODE 1
#endif

#if PLATFORM(MAC) && !PLATFORM(IOS)
#if CPU(X86_64)
#define WTF_USE_PLUGIN_HOST_PROCESS 1
#endif
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1070
#define ENABLE_GESTURE_EVENTS 1
#define ENABLE_RUBBER_BANDING 1
#define WTF_USE_SCROLLBAR_PAINTER 1
#endif
#if !defined(ENABLE_JAVA_BRIDGE)
#define ENABLE_JAVA_BRIDGE 1
#endif
#if !defined(ENABLE_DASHBOARD_SUPPORT)
#define ENABLE_DASHBOARD_SUPPORT 1
#endif
#define WTF_USE_CF 1
#define WTF_USE_PTHREADS 1
#define HAVE_PTHREAD_RWLOCK 1
#define HAVE_READLINE 1
#define HAVE_RUNLOOP_TIMER 1
#define ENABLE_FULLSCREEN_API 1
#define ENABLE_SMOOTH_SCROLLING 1
#define ENABLE_WEB_ARCHIVE 1
#define ENABLE_WEB_AUDIO 1
#define ENABLE_CANVAS_PATH 1

#if defined(ENABLE_VIDEO)
#define ENABLE_VIDEO_TRACK 1
#endif
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1080
#define HAVE_LAYER_HOSTING_IN_WINDOW_SERVER 1
#endif
#define WTF_USE_APPKIT 1
#define WTF_USE_SECURITY_FRAMEWORK 1
#endif /* PLATFORM(MAC) && !PLATFORM(IOS) */

#if PLATFORM(CHROMIUM) && OS(DARWIN)
#define WTF_USE_CF 1
#define WTF_USE_PTHREADS 1
#define HAVE_PTHREAD_RWLOCK 1

#define WTF_USE_WK_SCROLLBAR_PAINTER 1
#endif

#if PLATFORM(IOS)
#define DONT_FINALIZE_ON_MAIN_THREAD 1
#endif

#if PLATFORM(QT) && OS(DARWIN)
#define WTF_USE_CF 1
#endif

#if OS(DARWIN) && !PLATFORM(GTK) && !PLATFORM(QT)
#define ENABLE_PURGEABLE_MEMORY 1
#endif

#if PLATFORM(IOS)
#define ENABLE_CONTEXT_MENUS 0
#define ENABLE_DRAG_SUPPORT 0
#define ENABLE_GEOLOCATION 1
#define ENABLE_ICONDATABASE 0
#define ENABLE_INSPECTOR 1
#define ENABLE_JAVA_BRIDGE 0
#define ENABLE_NETSCAPE_PLUGIN_API 0
#define ENABLE_ORIENTATION_EVENTS 1
#define ENABLE_REPAINT_THROTTLING 1
#define ENABLE_WEB_ARCHIVE 1
#define HAVE_NETWORK_CFDATA_ARRAY_CALLBACK 1
#define HAVE_PTHREAD_RWLOCK 1
#define HAVE_READLINE 1
#define WTF_USE_CF 1
#define WTF_USE_CFNETWORK 1
#define WTF_USE_PTHREADS 1

#if PLATFORM(IOS_SIMULATOR)
    #define ENABLE_CLASSIC_INTERPRETER 1
    #define ENABLE_JIT 0
    #define ENABLE_YARR_JIT 0
#else
    #define ENABLE_CLASSIC_INTERPRETER 0
    #define ENABLE_JIT 1
    #define ENABLE_LLINT 1
    #define ENABLE_YARR_JIT 1
#endif

#define WTF_USE_APPKIT 0
#define WTF_USE_SECURITY_FRAMEWORK 0
#endif

#if PLATFORM(WIN) && !OS(WINCE)
#define WTF_USE_CF 1
#endif

#if PLATFORM(WIN) && !OS(WINCE) && !PLATFORM(CHROMIUM) && !PLATFORM(WIN_CAIRO)
#define WTF_USE_CFNETWORK 1
#endif

#if USE(CFNETWORK) || PLATFORM(MAC) || PLATFORM(IOS)
#define WTF_USE_CFURLCACHE 1
#define WTF_USE_CFURLSTORAGESESSIONS 1
#endif

#if PLATFORM(WIN) && !OS(WINCE) && !PLATFORM(CHROMIUM) && !PLATFORM(QT)
#define ENABLE_WEB_ARCHIVE 1
#endif

#if PLATFORM(WIN) && !OS(WINCE) && !PLATFORM(CHROMIUM) && !PLATFORM(WIN_CAIRO) && !PLATFORM(QT)
#define ENABLE_FULLSCREEN_API 1
#endif

#if PLATFORM(WX)
#if !CPU(PPC)
#define ENABLE_ASSEMBLER 1
#define ENABLE_JIT 1
#endif
#define ENABLE_GLOBAL_FASTMALLOC_NEW 0
#define ENABLE_LLINT 0
#if OS(DARWIN)
#define WTF_USE_CF 1
#define ENABLE_WEB_ARCHIVE 1
#endif
#endif

#if OS(UNIX) && (PLATFORM(GTK) || PLATFORM(QT))
#define WTF_USE_PTHREADS 1
#define HAVE_PTHREAD_RWLOCK 1
#endif

#if !defined(HAVE_ACCESSIBILITY)
#if PLATFORM(IOS) || PLATFORM(MAC) || PLATFORM(WIN) || PLATFORM(GTK) || (PLATFORM(CHROMIUM) && !OS(ANDROID)) || PLATFORM(EFL)
#define HAVE_ACCESSIBILITY 1
#endif
#endif /* !defined(HAVE_ACCESSIBILITY) */

#if OS(UNIX)
#define HAVE_SIGNAL_H 1
#define WTF_USE_OS_RANDOMNESS 1
#endif

#if (OS(FREEBSD) || OS(OPENBSD)) && !defined(__GLIBC__)
#define HAVE_PTHREAD_NP_H 1
#endif

#if !defined(HAVE_VASPRINTF)
#if !COMPILER(MSVC) && !COMPILER(RVCT) && !COMPILER(MINGW) && !(COMPILER(GCC) && OS(QNX))
#define HAVE_VASPRINTF 1
#endif
#endif

#if !defined(HAVE_STRNSTR)
#if OS(DARWIN) || (OS(FREEBSD) && !defined(__GLIBC__))
#define HAVE_STRNSTR 1
#endif
#endif

#if !OS(WINDOWS) && !OS(SOLARIS) \
    && !OS(RVCT) \
    && !OS(ANDROID)
#define HAVE_TM_GMTOFF 1
#define HAVE_TM_ZONE 1
#define HAVE_TIMEGM 1
#endif

#if OS(DARWIN)

#define HAVE_ERRNO_H 1
#define HAVE_LANGINFO_H 1
#define HAVE_MMAP 1
#define HAVE_MERGESORT 1
#define HAVE_SBRK 1
#define HAVE_STRINGS_H 1
#define HAVE_SYS_PARAM_H 1
#define HAVE_SYS_TIME_H 1
#define HAVE_SYS_TIMEB_H 1
#define WTF_USE_ACCELERATE 1

#if PLATFORM(IOS) || __MAC_OS_X_VERSION_MIN_REQUIRED >= 1060

#define HAVE_DISPATCH_H 1
#define HAVE_HOSTED_CORE_ANIMATION 1

#if !PLATFORM(IOS)
#define HAVE_MADV_FREE_REUSE 1
#define HAVE_MADV_FREE 1
#define HAVE_PTHREAD_SETNAME_NP 1
#endif

#endif

#if PLATFORM(IOS)
#define HAVE_MADV_FREE 1
#define HAVE_PTHREAD_SETNAME_NP 1
#endif

#elif OS(WINDOWS)

#if !OS(WINCE)
#define HAVE_SYS_TIMEB_H 1
#define HAVE_ALIGNED_MALLOC 1
#define HAVE_ISDEBUGGERPRESENT 1
#endif
#define HAVE_VIRTUALALLOC 1
#define WTF_USE_OS_RANDOMNESS 1

#elif OS(QNX)

#define HAVE_ERRNO_H 1
#define HAVE_MMAP 1
#define HAVE_MADV_FREE_REUSE 1
#define HAVE_MADV_FREE 1
#define HAVE_SBRK 1
#define HAVE_STRINGS_H 1
#define HAVE_SYS_PARAM_H 1
#define HAVE_SYS_TIME_H 1
#define WTF_USE_PTHREADS 1

#elif OS(ANDROID)

#define HAVE_ERRNO_H 1
#define HAVE_NMAP 1
#define HAVE_SBRK 1
#define HAVE_STRINGS_H 1
#define HAVE_SYS_PARAM_H 1
#define HAVE_SYS_TIME_H 1

#else

/* FIXME: is this actually used or do other platforms generate their own config.h? */

#define HAVE_ERRNO_H 1
#define HAVE_LANGINFO_H 1
#define HAVE_MMAP 1
#define HAVE_SBRK 1
#define HAVE_STRINGS_H 1
#define HAVE_SYS_PARAM_H 1
#define HAVE_SYS_TIME_H 1

#endif

/* ENABLE macro defaults */

#if PLATFORM(QT)
/* We must not customize the global operator new and delete for the Qt port. */
#define ENABLE_GLOBAL_FASTMALLOC_NEW 0
#if !OS(UNIX)
#define USE_SYSTEM_MALLOC 1
#endif
#endif

#if PLATFORM(EFL)
#define ENABLE_GLOBAL_FASTMALLOC_NEW 0
#endif

#if !defined(ENABLE_ICONDATABASE)
#define ENABLE_ICONDATABASE 1
#endif

#if !defined(ENABLE_SQL_DATABASE)
#define ENABLE_SQL_DATABASE 1
#endif

#if !defined(ENABLE_JAVASCRIPT_DEBUGGER)
#define ENABLE_JAVASCRIPT_DEBUGGER 1
#endif

#if !defined(ENABLE_FTPDIR)
#define ENABLE_FTPDIR 1
#endif

#if !defined(ENABLE_CONTEXT_MENUS)
#define ENABLE_CONTEXT_MENUS 1
#endif

#if !defined(ENABLE_DRAG_SUPPORT)
#define ENABLE_DRAG_SUPPORT 1
#endif

#if !defined(ENABLE_INSPECTOR)
#define ENABLE_INSPECTOR 1
#endif

#if !defined(ENABLE_NETSCAPE_PLUGIN_API)
#define ENABLE_NETSCAPE_PLUGIN_API 1
#endif

#if !defined(ENABLE_GLOBAL_FASTMALLOC_NEW)
#define ENABLE_GLOBAL_FASTMALLOC_NEW 1
#endif

#if !defined(ENABLE_PARSED_STYLE_SHEET_CACHING)
#define ENABLE_PARSED_STYLE_SHEET_CACHING 1
#endif

#if !defined(ENABLE_SUBPIXEL_LAYOUT)
#if PLATFORM(CHROMIUM) || PLATFORM(TIZEN)
#define ENABLE_SUBPIXEL_LAYOUT 1 
#else
#define ENABLE_SUBPIXEL_LAYOUT 0
#endif
#endif

#define ENABLE_DEBUG_WITH_BREAKPOINT 0
#define ENABLE_SAMPLING_COUNTERS 0
#define ENABLE_SAMPLING_FLAGS 0
#define ENABLE_SAMPLING_REGIONS 0
#define ENABLE_OPCODE_SAMPLING 0
#define ENABLE_CODEBLOCK_SAMPLING 0
#if ENABLE(CODEBLOCK_SAMPLING) && !ENABLE(OPCODE_SAMPLING)
#error "CODEBLOCK_SAMPLING requires OPCODE_SAMPLING"
#endif
#if ENABLE(OPCODE_SAMPLING) || ENABLE(SAMPLING_FLAGS) || ENABLE(SAMPLING_REGIONS)
#define ENABLE_SAMPLING_THREAD 1
#endif

#if !defined(ENABLE_TEXT_CARET) && !PLATFORM(IOS)
#define ENABLE_TEXT_CARET 1
#endif

#if !defined(WTF_USE_JSVALUE64) && !defined(WTF_USE_JSVALUE32_64)
#if (CPU(X86_64) && (OS(UNIX) || OS(WINDOWS))) \
    || (CPU(IA64) && !CPU(IA64_32)) \
    || CPU(ALPHA) \
    || CPU(SPARC64) \
    || CPU(S390X) \
    || CPU(PPC64)
#define WTF_USE_JSVALUE64 1
#else
#define WTF_USE_JSVALUE32_64 1
#endif
#endif /* !defined(WTF_USE_JSVALUE64) && !defined(WTF_USE_JSVALUE32_64) */

/* Disable the JIT on versions of GCC prior to 4.1 */
#if !defined(ENABLE_JIT) && COMPILER(GCC) && !GCC_VERSION_AT_LEAST(4, 1, 0)
#define ENABLE_JIT 0
#endif

/* JIT is not implemented for Windows 64-bit */
#if !defined(ENABLE_JIT) && OS(WINDOWS) && CPU(X86_64)
#define ENABLE_JIT 0
#endif

#if !defined(ENABLE_JIT) && CPU(SH4) && PLATFORM(QT)
#define ENABLE_JIT 1
#endif

/* The JIT is enabled by default on all x86, x86-64, ARM & MIPS platforms. */
#if !defined(ENABLE_JIT) \
    && (CPU(X86) || CPU(X86_64) || CPU(ARM) || CPU(MIPS)) \
    && (OS(DARWIN) || !COMPILER(GCC) || GCC_VERSION_AT_LEAST(4, 1, 0)) \
    && !OS(WINCE) \
    && !OS(QNX)
#define ENABLE_JIT 1
#endif

/* If possible, try to enable a disassembler. This is optional. We proceed in two
   steps: first we try to find some disassembler that we can use, and then we
   decide if the high-level disassembler API can be enabled. */
#if !defined(WTF_USE_UDIS86) && ENABLE(JIT) && PLATFORM(MAC) && (CPU(X86) || CPU(X86_64))
#define WTF_USE_UDIS86 1
#endif

#if !defined(ENABLE_DISASSEMBLER) && USE(UDIS86)
#define ENABLE_DISASSEMBLER 1
#endif

/* On some of the platforms where we have a JIT, we want to also have the
   low-level interpreter. */
#if !defined(ENABLE_LLINT) \
    && ENABLE(JIT) \
    && (OS(DARWIN) || OS(LINUX)) \
    && (PLATFORM(MAC) || PLATFORM(IOS) || PLATFORM(GTK)) \
    && (CPU(X86) || CPU(X86_64) || CPU(ARM_THUMB2))
#define ENABLE_LLINT 1
#endif

#if !defined(ENABLE_DFG_JIT) && ENABLE(JIT)
/* Enable the DFG JIT on X86 and X86_64.  Only tested on Mac and GNU/Linux. */
#if (CPU(X86) || CPU(X86_64)) && (PLATFORM(MAC) || OS(LINUX))
#define ENABLE_DFG_JIT 1
#endif
/* Enable the DFG JIT on ARMv7.  Only tested on iOS. */
#if CPU(ARM_THUMB2) && (PLATFORM(IOS) || PLATFORM(BLACKBERRY) || PLATFORM(TIZEN))
#define ENABLE_DFG_JIT 1
#endif
/* Enable the DFG JIT on ARM. */
#if CPU(ARM_TRADITIONAL) && !PLATFORM(TIZEN)
#define ENABLE_DFG_JIT 1
#endif
#endif

/* Profiling of types and values used by JIT code. DFG_JIT depends on it, but you
   can enable it manually with DFG turned off if you want to use it as a standalone
   profiler. In that case, you probably want to also enable VERBOSE_VALUE_PROFILE
   below. */
#if !defined(ENABLE_VALUE_PROFILER) && ENABLE(DFG_JIT)
#define ENABLE_VALUE_PROFILER 1
#endif

#if !defined(ENABLE_VERBOSE_VALUE_PROFILE) && ENABLE(VALUE_PROFILER)
#define ENABLE_VERBOSE_VALUE_PROFILE 0
#endif

#if !defined(ENABLE_SIMPLE_HEAP_PROFILING)
#define ENABLE_SIMPLE_HEAP_PROFILING 0
#endif

/* Counts uses of write barriers using sampling counters. Be sure to also
   set ENABLE_SAMPLING_COUNTERS to 1. */
#if !defined(ENABLE_WRITE_BARRIER_PROFILING)
#define ENABLE_WRITE_BARRIER_PROFILING 0
#endif

/* Ensure that either the JIT or the interpreter has been enabled. */
#if !defined(ENABLE_CLASSIC_INTERPRETER) && !ENABLE(JIT) && !ENABLE(LLINT)
#define ENABLE_CLASSIC_INTERPRETER 1
#endif

/* If the jit and classic interpreter is not available, enable the LLInt C Loop: */
#if !ENABLE(JIT) && !ENABLE(CLASSIC_INTERPRETER)
    #define ENABLE_LLINT 1
    #define ENABLE_LLINT_C_LOOP 1
    #define ENABLE_DFG_JIT 0
#endif

/* Do a sanity check to make sure that we at least have one execution engine in
   use: */
#if !(ENABLE(JIT) || ENABLE(CLASSIC_INTERPRETER) || ENABLE(LLINT))
#error You have to have at least one execution model enabled to build JSC
#endif
/* Do a sanity check to make sure that we don't have both the classic interpreter
   and the llint C loop in use at the same time: */
#if ENABLE(CLASSIC_INTERPRETER) && ENABLE(LLINT_C_LOOP)
#error You cannot build both the classic interpreter and the llint C loop together
#endif

/* Configure the JIT */
#if CPU(X86) && COMPILER(MSVC)
#define JSC_HOST_CALL __fastcall
#elif CPU(X86) && COMPILER(GCC)
#define JSC_HOST_CALL __attribute__ ((fastcall))
#else
#define JSC_HOST_CALL
#endif

/* Configure the interpreter */
#if COMPILER(GCC) || (RVCT_VERSION_AT_LEAST(4, 0, 0, 0) && defined(__GNUC__))
#define HAVE_COMPUTED_GOTO 1
#endif
#if HAVE(COMPUTED_GOTO) && ENABLE(CLASSIC_INTERPRETER)
#define ENABLE_COMPUTED_GOTO_CLASSIC_INTERPRETER 1
#endif

/* Determine if we need to enable Computed Goto Opcodes or not: */
#if (HAVE(COMPUTED_GOTO) && ENABLE(LLINT)) || ENABLE(COMPUTED_GOTO_CLASSIC_INTERPRETER)
#define ENABLE_COMPUTED_GOTO_OPCODES 1
#endif

/* Regular Expression Tracing - Set to 1 to trace RegExp's in jsc.  Results dumped at exit */
#define ENABLE_REGEXP_TRACING 0

/* Yet Another Regex Runtime - turned on by default for JIT enabled ports. */
#if !defined(ENABLE_YARR_JIT) && (ENABLE(JIT) || ENABLE(LLINT_C_LOOP)) && !PLATFORM(CHROMIUM)
#define ENABLE_YARR_JIT 1

/* Setting this flag compares JIT results with interpreter results. */
#define ENABLE_YARR_JIT_DEBUG 0
#endif

#if ENABLE(JIT) || ENABLE(YARR_JIT)
#define ENABLE_ASSEMBLER 1
#endif

/* Pick which allocator to use; we only need an executable allocator if the assembler is compiled in.
   On x86-64 we use a single fixed mmap, on other platforms we mmap on demand. */
#if ENABLE(ASSEMBLER)
#if CPU(X86_64) || PLATFORM(IOS)
#define ENABLE_EXECUTABLE_ALLOCATOR_FIXED 1
#else
#define ENABLE_EXECUTABLE_ALLOCATOR_DEMAND 1
#endif
#endif

#if !defined(ENABLE_PAN_SCROLLING) && OS(WINDOWS)
#define ENABLE_PAN_SCROLLING 1
#endif

/* Use the QXmlStreamReader implementation for XMLDocumentParser */
/* Use the QXmlQuery implementation for XSLTProcessor */
#if PLATFORM(QT)
#if !USE(LIBXML2)
#define WTF_USE_QXMLSTREAM 1
#define WTF_USE_QXMLQUERY 1
#endif
#endif

/* Accelerated compositing */
#if PLATFORM(MAC) || PLATFORM(IOS) || PLATFORM(QT) || (PLATFORM(WIN) && !OS(WINCE) && !PLATFORM(WIN_CAIRO))
#define WTF_USE_ACCELERATED_COMPOSITING 1
#endif

#if PLATFORM(MAC) || PLATFORM(IOS)
#define ENABLE_CSS_IMAGE_SET 1
#endif


/* Qt always uses Texture Mapper */
#if PLATFORM(QT)
#define WTF_USE_TEXTURE_MAPPER 1
#if USE(3D_GRAPHICS)
#define WTF_USE_TEXTURE_MAPPER_GL 1
#endif
#endif

#if ENABLE(WEBGL) && !defined(WTF_USE_3D_GRAPHICS)
#define WTF_USE_3D_GRAPHICS 1
#endif

/* Compositing on the UI-process in WebKit2 */
#if PLATFORM(QT)
#define WTF_USE_UI_SIDE_COMPOSITING 1
#endif

#if PLATFORM(MAC) || PLATFORM(IOS)
#define WTF_USE_PROTECTION_SPACE_AUTH_CALLBACK 1
#endif

#if !ENABLE(NETSCAPE_PLUGIN_API) || (ENABLE(NETSCAPE_PLUGIN_API) && ((OS(UNIX) && (PLATFORM(GTK) || PLATFORM(QT) || PLATFORM(WX))) || PLATFORM(EFL)))
#define ENABLE_PLUGIN_PACKAGE_SIMPLE_HASH 1
#endif

#if PLATFORM(MAC) && !PLATFORM(IOS) && __MAC_OS_X_VERSION_MIN_REQUIRED >= 1080
#define ENABLE_THREADED_SCROLLING 1
#endif

/* Set up a define for a common error that is intended to cause a build error -- thus the space after Error. */
#define WTF_PLATFORM_CFNETWORK Error USE_macro_should_be_used_with_CFNETWORK

/* FIXME: Eventually we should enable this for all platforms and get rid of the define. */
#if PLATFORM(IOS) || PLATFORM(MAC) || PLATFORM(WIN) || PLATFORM(QT) || PLATFORM(GTK) || PLATFORM(EFL)
#define WTF_USE_PLATFORM_STRATEGIES 1
#endif

#if PLATFORM(WIN)
#define WTF_USE_CROSS_PLATFORM_CONTEXT_MENUS 1
#endif

#if PLATFORM(MAC) && HAVE(ACCESSIBILITY)
#define WTF_USE_ACCESSIBILITY_CONTEXT_MENUS 1
#endif

#if CPU(ARM_THUMB2)
#define ENABLE_BRANCH_COMPACTION 1
#endif

#if !defined(ENABLE_THREADING_LIBDISPATCH) && HAVE(DISPATCH_H)
#define ENABLE_THREADING_LIBDISPATCH 1
#elif !defined(ENABLE_THREADING_OPENMP) && defined(_OPENMP)
#define ENABLE_THREADING_OPENMP 1
#elif !defined(THREADING_GENERIC)
#define ENABLE_THREADING_GENERIC 1
#endif

#if ENABLE(GLIB_SUPPORT)
#include <wtf/gobject/GTypedefs.h>
#endif

#if PLATFORM(EFL)
#include <wtf/efl/EflTypedefs.h>
#endif

/* FIXME: This define won't be needed once #27551 is fully landed. However,
   since most ports try to support sub-project independence, adding new headers
   to WTF causes many ports to break, and so this way we can address the build
   breakages one port at a time. */
#if !defined(WTF_USE_EXPORT_MACROS) && (PLATFORM(MAC) || PLATFORM(QT) || PLATFORM(WX) || PLATFORM(BLACKBERRY))
#define WTF_USE_EXPORT_MACROS 1
#endif

#if !defined(WTF_USE_EXPORT_MACROS_FOR_TESTING) && (PLATFORM(GTK) || PLATFORM(WIN))
#define WTF_USE_EXPORT_MACROS_FOR_TESTING 1
#endif

#if (PLATFORM(QT) && !OS(DARWIN)) || PLATFORM(GTK) || PLATFORM(EFL)
#define WTF_USE_UNIX_DOMAIN_SOCKETS 1
#endif

#if !defined(ENABLE_COMPARE_AND_SWAP) && (OS(WINDOWS) || (COMPILER(GCC) && (CPU(X86) || CPU(X86_64) || CPU(ARM_THUMB2))))
#define ENABLE_COMPARE_AND_SWAP 1
#endif

#define ENABLE_OBJECT_MARK_LOGGING 0

#if !defined(ENABLE_PARALLEL_GC) && !ENABLE(OBJECT_MARK_LOGGING) && (PLATFORM(MAC) || PLATFORM(IOS) || PLATFORM(BLACKBERRY) || PLATFORM(GTK) || PLATFORM(TIZEN)) && ENABLE(COMPARE_AND_SWAP)
#define ENABLE_PARALLEL_GC 1
#elif PLATFORM(QT)
// Parallel GC is temporarily disabled on Qt because of regular crashes, see https://bugs.webkit.org/show_bug.cgi?id=90957 for details
#define ENABLE_PARALLEL_GC 0
#endif

#if !defined(ENABLE_GC_VALIDATION) && !defined(NDEBUG)
#define ENABLE_GC_VALIDATION 1
#endif

#if PLATFORM(MAC) && !PLATFORM(IOS) && __MAC_OS_X_VERSION_MIN_REQUIRED >= 1070
#define WTF_USE_AVFOUNDATION 1
#endif

#if PLATFORM(MAC) && !PLATFORM(IOS) && __MAC_OS_X_VERSION_MIN_REQUIRED >= 1080
#define WTF_USE_COREMEDIA 1
#endif

#if PLATFORM(MAC) && !PLATFORM(IOS) && __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090
#define HAVE_AVFOUNDATION_TEXT_TRACK_SUPPORT 1
#endif

#if PLATFORM(MAC) && !PLATFORM(IOS) && __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090
#define HAVE_MEDIA_ACCESSIBILITY_FRAMEWORK 1
#endif

#if PLATFORM(MAC) || PLATFORM(GTK) || (PLATFORM(WIN) && !OS(WINCE) && !PLATFORM(WIN_CAIRO)) || PLATFORM(BLACKBERRY)
#define WTF_USE_REQUEST_ANIMATION_FRAME_TIMER 1
#endif

#if PLATFORM(MAC) || PLATFORM(BLACKBERRY)
#define WTF_USE_REQUEST_ANIMATION_FRAME_DISPLAY_MONITOR 1
#endif

#if PLATFORM(MAC) && (PLATFORM(IOS) || __MAC_OS_X_VERSION_MIN_REQUIRED >= 1070)
#define HAVE_INVERTED_WHEEL_EVENTS 1
#endif

#if PLATFORM(MAC)
#define WTF_USE_COREAUDIO 1
#endif

#if !defined(WTF_USE_V8) && PLATFORM(CHROMIUM)
#define WTF_USE_V8 1
#endif

/* Not using V8 implies using JSC and vice versa */
#if !USE(V8)
#define WTF_USE_JSC 1
#endif

#if ENABLE(NOTIFICATIONS) && PLATFORM(MAC)
#define ENABLE_TEXT_NOTIFICATIONS_ONLY 1
#endif

#if !defined(WTF_USE_ZLIB) && !PLATFORM(QT)
#define WTF_USE_ZLIB 1
#endif

#if PLATFORM(QT)
#include <qglobal.h>
#if defined(QT_OPENGL_ES_2) && !defined(WTF_USE_OPENGL_ES_2)
#define WTF_USE_OPENGL_ES_2 1
#endif
#endif

#endif /* WTF_Platform_h */
