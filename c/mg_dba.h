/*
   ----------------------------------------------------------------------------
   | mg_dba.so|dll                                                            |
   | Description: An abstraction of the InterSystems Cache/IRIS API           |
   |              and YottaDB API                                             |
   | Author:      Chris Munt cmunt@mgateway.com                               |
   |                         chris.e.munt@gmail.com                           |
   | Copyright (c) 2017-2019 M/Gateway Developments Ltd,                      |
   | Surrey UK.                                                               |
   | All rights reserved.                                                     |
   |                                                                          |
   | http://www.mgateway.com                                                  |
   |                                                                          |
   | Licensed under the Apache License, Version 2.0 (the "License"); you may  |
   | not use this file except in compliance with the License.                 |
   | You may obtain a copy of the License at                                  |
   |                                                                          |
   | http://www.apache.org/licenses/LICENSE-2.0                               |
   |                                                                          |
   | Unless required by applicable law or agreed to in writing, software      |
   | distributed under the License is distributed on an "AS IS" BASIS,        |
   | WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. |
   | See the License for the specific language governing permissions and      |
   | limitations under the License.                                           |      
   |                                                                          |
   ----------------------------------------------------------------------------
*/

#ifndef DBX_H
#define DBX_H

#if defined(_WIN32)

#define BUILDING_NODE_EXTENSION     1
#if defined(_MSC_VER)
/* Check for MS compiler later than VC6 */
#if (_MSC_VER >= 1400)
#define _CRT_SECURE_NO_DEPRECATE    1
#define _CRT_NONSTDC_NO_DEPRECATE   1
#endif
#endif

#elif defined(__linux__) || defined(__linux) || defined(linux)

#if !defined(LINUX)
#define LINUX                       1
#endif

#elif defined(__APPLE__)

#if !defined(MACOSX)
#define MACOSX                      1
#endif

#endif

#if defined(SOLARIS)
#ifndef __GNUC__
#  define  __attribute__(x)
#endif
#endif

#if defined(_WIN32)
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#else
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>
#include <dlfcn.h>
#include <signal.h>
#include <pwd.h>
#endif


/* Cache/IRIS */

#define CACHE_MAXSTRLEN	32767
#define CACHE_MAXLOSTSZ	3641144

typedef char		Callin_char_t;
#define CACHE_INT64 long long
#define CACHESTR	CACHE_ASTR

typedef struct {
   unsigned short len;
   Callin_char_t  str[CACHE_MAXSTRLEN];
} CACHE_ASTR, *CACHE_ASTRP;

typedef struct {
   unsigned int	len;
   union {
      Callin_char_t * ch;
      unsigned short *wch;
      unsigned short *lch;
   } str;
} CACHE_EXSTR, *CACHE_EXSTRP;

#define CACHE_TTALL     1
#define CACHE_TTNEVER   8
#define CACHE_PROGMODE  32

#define CACHE_INT	      1
#define CACHE_DOUBLE	   2
#define CACHE_ASTRING   3

#define CACHE_CHAR      4
#define CACHE_INT2      5
#define CACHE_INT4      6
#define CACHE_INT8      7
#define CACHE_UCHAR     8
#define CACHE_UINT2     9
#define CACHE_UINT4     10
#define CACHE_UINT8     11
#define CACHE_FLOAT     12
#define CACHE_HFLOAT    13
#define CACHE_UINT      14
#define CACHE_WSTRING   15
#define CACHE_OREF      16
#define CACHE_LASTRING  17
#define CACHE_LWSTRING  18
#define CACHE_IEEE_DBL  19
#define CACHE_HSTRING   20
#define CACHE_UNDEF     21

#define CACHE_CHANGEPASSWORD  -16
#define CACHE_ACCESSDENIED    -15
#define CACHE_EXSTR_INUSE     -14
#define CACHE_NORES	         -13
#define CACHE_BADARG	         -12
#define CACHE_NOTINCACHE      -11
#define CACHE_RETTRUNC 	      -10
#define CACHE_ERUNKNOWN	      -9	
#define CACHE_RETTOOSMALL     -8	
#define CACHE_NOCON 	         -7
#define CACHE_INTERRUPT       -6
#define CACHE_CONBROKEN       -4
#define CACHE_STRTOOLONG      -3
#define CACHE_ALREADYCON      -2
#define CACHE_FAILURE	      -1
#define CACHE_SUCCESS 	      0

#define CACHE_ERMXSTR         5
#define CACHE_ERNOLINE        8
#define CACHE_ERUNDEF         9
#define CACHE_ERSYSTEM        10
#define CACHE_ERSUBSCR        16
#define CACHE_ERNOROUTINE     17
#define CACHE_ERSTRINGSTACK   20
#define CACHE_ERUNIMPLEMENTED 22
#define CACHE_ERARGSTACK      25
#define CACHE_ERPROTECT       27
#define CACHE_ERPARAMETER     40
#define CACHE_ERNAMSP         83
#define CACHE_ERWIDECHAR      89
#define CACHE_ERNOCLASS       122
#define CACHE_ERBADOREF       119
#define CACHE_ERNOMETHOD      120
#define CACHE_ERNOPROPERTY    121

#define CACHE_ETIMEOUT        -100
#define CACHE_BAD_STRING      -101
#define CACHE_BAD_NAMESPACE   -102
#define CACHE_BAD_GLOBAL      -103
#define CACHE_BAD_FUNCTION    -104
#define CACHE_BAD_CLASS       -105
#define CACHE_BAD_METHOD      -106

#define CACHE_INCREMENTAL_LOCK   1

/* End of Cache/IRIS */


/* YottaDB */

#define YDB_DEL_TREE 1

typedef struct {
   unsigned int   len_alloc;
   unsigned int   len_used;
   char		      *buf_addr;
} ydb_buffer_t;

typedef struct {
   unsigned long  length;
   char		      *address;
} ydb_string_t;

typedef struct {
   ydb_string_t   rtn_name;
   void		      *handle;
} ci_name_descriptor;

typedef ydb_buffer_t DBXSTR;

/* End of YottaDB */


#define DBX_DBTYPE_CACHE         1
#define DBX_DBTYPE_IRIS          2
#define DBX_DBTYPE_YOTTADB       5

#define DBX_MAXCONS              32
#define DBX_MAXARGS              64

#define DBX_ERROR_SIZE           512

#define DBX_DSORT_INVALID        0
#define DBX_DSORT_DATA           1
#define DBX_DSORT_SUBSCRIPT      2
#define DBX_DSORT_GLOBAL         3
#define DBX_DSORT_EOD            9
#define DBX_DSORT_STATUS         10
#define DBX_DSORT_ERROR          11

#define DBX_DSORT_ISVALID(a)     ((a == DBX_DSORT_GLOBAL) || (a == DBX_DSORT_SUBSCRIPT) || (a == DBX_DSORT_DATA) || (a == DBX_DSORT_EOD) || (a == DBX_DSORT_STATUS))

#define DBX_DTYPE_NONE           0
#define DBX_DTYPE_DBXSTR         1
#define DBX_DTYPE_STR            2
#define DBX_DTYPE_INT            4
#define DBX_DTYPE_INT64          5
#define DBX_DTYPE_DOUBLE         6
#define DBX_DTYPE_OREF           7
#define DBX_DTYPE_NULL           10

#define DBX_MAXSIZE              32767
#define DBX_BUFFER               32768

#define DBX_LS_MAXSIZE           3641144
#define DBX_LS_BUFFER            3641145

#if defined(MAX_PATH) && (MAX_PATH>511)
#define DBX_MAX_PATH             MAX_PATH
#else
#define DBX_MAX_PATH             512
#endif

#if defined(_WIN32)
#define DBX_NULL_DEVICE          "//./nul"
#else
#define DBX_NULL_DEVICE          "/dev/null/"
#endif

#if defined(_WIN32)
#define DBX_CACHE_DLL            "cache.dll"
#define DBX_IRIS_DLL             "irisdb.dll"
#define DBX_YDB_DLL              "yottadb.dll"
#else
#define DBX_CACHE_SO             "libcache.so"
#define DBX_CACHE_DYLIB          "libcache.dylib"
#define DBX_IRIS_SO              "libirisdb.so"
#define DBX_IRIS_DYLIB           "libirisdb.dylib"
#define DBX_YDB_SO               "libyottadb.so"
#define DBX_YDB_DYLIB            "libyottadb.dylib"
#endif

#if defined(__linux__) || defined(linux) || defined(LINUX)
#define DBX_MEMCPY(a,b,c)           memmove(a,b,c)
#else
#define DBX_MEMCPY(a,b,c)           memcpy(a,b,c)
#endif

#define DBX_LOCK(RC, TIMEOUT) \
   if (pcon->use_isc_mutex) { \
      RC = mgw_mutex_lock(pcon->p_isc_mutex, TIMEOUT); \
   } \

#define DBX_UNLOCK(RC) \
   if (pcon->use_isc_mutex) { \
      RC = mgw_mutex_unlock(pcon->p_isc_mutex); \
   } \

#if defined(_WIN32)

#define DBX_EXTFUN(a)    __declspec(dllexport) a __cdecl

typedef DWORD           DBXTHID;
typedef HINSTANCE       DBXPLIB;
typedef FARPROC         DBXPROC;

#else /* #if defined(_WIN32) */

#define DBX_EXTFUN(a)    a

typedef pthread_t       DBXTHID;
typedef void            *DBXPLIB;
typedef void            *DBXPROC;

#endif /* #if defined(_WIN32) */



typedef struct tagDBXDEBUG {
   unsigned char  debug;
   FILE *         p_fdebug;
} DBXDEBUG, *PDBXDEBUG;


typedef struct tagDBXZV {
   unsigned char  product;
   double         mgw_version;
   int            majorversion;
   int            minorversion;
   int            mgw_build;
   unsigned long  vnumber; /* yymbbbb */
   char           version[64];
} DBXZV, *PDBXZV;


typedef struct tagDBXMUTEX {
   unsigned char     created;
   int               stack;
#if defined(_WIN32)
   HANDLE            h_mutex;
#else
   pthread_mutex_t   h_mutex;
#endif /* #if defined(_WIN32) */
   DBXTHID           thid;
} DBXMUTEX, *PDBXMUTEX;


typedef struct tagDBXCVAL {
   void           *pstr;
   CACHE_EXSTR    zstr;
} DBXCVAL, *PDBXCVAL;


typedef struct tagDBXVAL {
   short          type;
   union {
      int            int32;
      long long      int64;
      double         real;
      unsigned int   oref;
   } num;
   unsigned long  offset;
   DBXSTR         svalue;
   DBXCVAL cvalue;
} DBXVAL, *PDBXVAL;


typedef struct tagDBXFUN {
   unsigned int   rflag;
   int            label_len;
   char *         label;
   int            routine_len;
   char *         routine;
   char           buffer[128];
} DBXFUN, *PDBXFUN;


typedef struct tagDBXISCSO {

   short             loaded;
   short             iris;
   short             merge_enabled;
   short             functions_enabled;
   short             objects_enabled;
   short             multithread_enabled;
   char              funprfx[8];
   char              libdir[256];
   char              libnam[256];
   char              dbname[32];
   DBXPLIB           p_library;

   int               (* p_CacheSetDir)                   (char * dir);
   int               (* p_CacheSecureStartA)             (CACHE_ASTRP username, CACHE_ASTRP password, CACHE_ASTRP exename, unsigned long flags, int tout, CACHE_ASTRP prinp, CACHE_ASTRP prout);
   int               (* p_CacheEnd)                      (void);

   unsigned char *   (* p_CacheExStrNew)                 (CACHE_EXSTRP zstr, int size);
   unsigned short *  (* p_CacheExStrNewW)                (CACHE_EXSTRP zstr, int size);
   wchar_t *         (* p_CacheExStrNewH)                (CACHE_EXSTRP zstr, int size);
   int               (* p_CachePushExStr)                (CACHE_EXSTRP sptr);
   int               (* p_CachePushExStrW)               (CACHE_EXSTRP sptr);
   int               (* p_CachePushExStrH)               (CACHE_EXSTRP sptr);
   int               (* p_CachePopExStr)                 (CACHE_EXSTRP sstrp);
   int               (* p_CachePopExStrW)                (CACHE_EXSTRP sstrp);
   int               (* p_CachePopExStrH)                (CACHE_EXSTRP sstrp);
   int               (* p_CacheExStrKill)                (CACHE_EXSTRP obj);
   int               (* p_CachePushStr)                  (int len, Callin_char_t * ptr);
   int               (* p_CachePushStrW)                 (int len, short * ptr);
   int               (* p_CachePushStrH)                 (int len, wchar_t * ptr);
   int               (* p_CachePopStr)                   (int * lenp, Callin_char_t ** strp);
   int               (* p_CachePopStrW)                  (int * lenp, short ** strp);
   int               (* p_CachePopStrH)                  (int * lenp, wchar_t ** strp);
   int               (* p_CachePushDbl)                  (double num);
   int               (* p_CachePushIEEEDbl)              (double num);
   int               (* p_CachePopDbl)                   (double * nump);
   int               (* p_CachePushInt)                  (int num);
   int               (* p_CachePopInt)                   (int * nump);
   int               (* p_CachePushInt64)                (CACHE_INT64 num);
   int               (* p_CachePopInt64)                 (CACHE_INT64 * nump);

   int               (* p_CachePushGlobal)               (int nlen, const Callin_char_t * nptr);
   int               (* p_CachePushGlobalX)              (int nlen, const Callin_char_t * nptr, int elen, const Callin_char_t * eptr);
   int               (* p_CacheGlobalGet)                (int narg, int flag);
   int               (* p_CacheGlobalSet)                (int narg);
   int               (* p_CacheGlobalData)               (int narg, int valueflag);
   int               (* p_CacheGlobalKill)               (int narg, int nodeonly);
   int               (* p_CacheGlobalOrder)              (int narg, int dir, int valueflag);
   int               (* p_CacheGlobalQuery)              (int narg, int dir, int valueflag);
   int               (* p_CacheGlobalIncrement)          (int narg);
   int               (* p_CacheGlobalRelease)            (void);

   int               (* p_CacheAcquireLock)              (int nsub, int flg, int tout, int * rval);
   int               (* p_CacheReleaseAllLocks)          (void);
   int               (* p_CacheReleaseLock)              (int nsub, int flg);
   int               (* p_CachePushLock)                 (int nlen, const Callin_char_t * nptr);

   int               (* p_CacheAddGlobal)                (int num, const Callin_char_t * nptr);
   int               (* p_CacheAddGlobalDescriptor)      (int num);
   int               (* p_CacheAddSSVN)                  (int num, const Callin_char_t * nptr);
   int               (* p_CacheAddSSVNDescriptor)        (int num);
   int               (* p_CacheMerge)                    (void);

   int               (* p_CachePushFunc)                 (unsigned int * rflag, int tlen, const Callin_char_t * tptr, int nlen, const Callin_char_t * nptr);
   int               (* p_CacheExtFun)                   (unsigned int flags, int narg);
   int               (* p_CachePushRtn)                  (unsigned int * rflag, int tlen, const Callin_char_t * tptr, int nlen, const Callin_char_t * nptr);
   int               (* p_CacheDoFun)                    (unsigned int flags, int narg);
   int               (* p_CacheDoRtn)                    (unsigned int flags, int narg);

   int               (* p_CacheCloseOref)                (unsigned int oref);
   int               (* p_CacheIncrementCountOref)       (unsigned int oref);
   int               (* p_CachePopOref)                  (unsigned int * orefp);
   int               (* p_CachePushOref)                 (unsigned int oref);
   int               (* p_CacheInvokeMethod)             (int narg);
   int               (* p_CachePushMethod)               (unsigned int oref, int mlen, const Callin_char_t * mptr, int flg);
   int               (* p_CacheInvokeClassMethod)        (int narg);
   int               (* p_CachePushClassMethod)          (int clen, const Callin_char_t * cptr, int mlen, const Callin_char_t * mptr, int flg);
   int               (* p_CacheGetProperty)              (void);
   int               (* p_CacheSetProperty)              (void);
   int               (* p_CachePushProperty)             (unsigned int oref, int plen, const Callin_char_t * pptr);

   int               (* p_CacheType)                     (void);

   int               (* p_CacheEvalA)                    (CACHE_ASTRP volatile expr);
   int               (* p_CacheExecuteA)                 (CACHE_ASTRP volatile cmd);
   int               (* p_CacheConvert)                  (unsigned long type, void * rbuf);

   int               (* p_CacheErrorA)                   (CACHE_ASTRP, CACHE_ASTRP, int *);
   int               (* p_CacheErrxlateA)                (int, CACHE_ASTRP);

   int               (* p_CacheEnableMultiThread)        (void);

} DBXISCSO, *PDBXISCSO;


typedef struct tagDBXYDBSO {
   short             loaded;
   char              libdir[256];
   char              libnam[256];
   char              funprfx[8];
   char              dbname[32];
   DBXPLIB           p_library;

   int               (* p_ydb_init)                      (void);
   int               (* p_ydb_exit)                      (void);
   int               (* p_ydb_malloc)                    (size_t size);
   int               (* p_ydb_free)                      (void *ptr);
   int               (* p_ydb_data_s)                    (ydb_buffer_t *varname, int subs_used, ydb_buffer_t *subsarray, unsigned int *ret_value);
   int               (* p_ydb_delete_s)                  (ydb_buffer_t *varname, int subs_used, ydb_buffer_t *subsarray, int deltype);
   int               (* p_ydb_set_s)                     (ydb_buffer_t *varname, int subs_used, ydb_buffer_t *subsarray, ydb_buffer_t *value);
   int               (* p_ydb_get_s)                     (ydb_buffer_t *varname, int subs_used, ydb_buffer_t *subsarray, ydb_buffer_t *ret_value);
   int               (* p_ydb_subscript_next_s)          (ydb_buffer_t *varname, int subs_used, ydb_buffer_t *subsarray, ydb_buffer_t *ret_value);
   int               (* p_ydb_subscript_previous_s)      (ydb_buffer_t *varname, int subs_used, ydb_buffer_t *subsarray, ydb_buffer_t *ret_value);
   int               (* p_ydb_node_next_s)               (ydb_buffer_t *varname, int subs_used, ydb_buffer_t *subsarray, int *ret_subs_used, ydb_buffer_t *ret_subsarray);
   int               (* p_ydb_node_previous_s)           (ydb_buffer_t *varname, int subs_used, ydb_buffer_t *subsarray, int *ret_subs_used, ydb_buffer_t *ret_subsarray);
   int               (* p_ydb_incr_s)                    (ydb_buffer_t *varname, int subs_used, ydb_buffer_t *subsarray, ydb_buffer_t *increment, ydb_buffer_t *ret_value);
   int               (* p_ydb_ci)                        (const char *c_rtn_name, ...);
   int               (* p_ydb_cip)                       (ci_name_descriptor *ci_info, ...);
} DBXYDBSO, *PDBXYDBSO;


typedef struct tagDBXCON {
   short          dbtype;
   int            argc;
   unsigned long  pid;
   char           shdir[256];
   char           username[64];
   char           password[64];
   char           nspace[64];
   char           input_device[64];
   char           output_device[64];
   char           debug_str[64];
   DBXSTR         input_str;
   DBXVAL         output_val;
   int            offset;
   DBXVAL         args[DBX_MAXARGS];
   ydb_buffer_t   yargs[DBX_MAXARGS];
   int            error_code;
   char           error[DBX_ERROR_SIZE];
   short          use_isc_mutex;
   DBXMUTEX       *p_isc_mutex;
   DBXMUTEX       isc_mutex;
   DBXZV          *p_zv;
   DBXZV          zv;
   DBXDEBUG       *p_debug;
   DBXDEBUG       debug;
   DBXISCSO       *p_isc_so;
   DBXYDBSO       *p_ydb_so;
} DBXCON, *PDBXCON;


DBX_EXTFUN(int)         dbx_init                      ();
DBX_EXTFUN(int)         dbx_version                   (int index, char *output, int output_len);
DBX_EXTFUN(int)         dbx_open                      (unsigned char *input, unsigned char *output);
DBX_EXTFUN(int)         dbx_close                     (unsigned char *input, unsigned char *output);
DBX_EXTFUN(int)         dbx_set                       (unsigned char *input, unsigned char *output);
DBX_EXTFUN(int)         dbx_get                       (unsigned char *input, unsigned char *output);
DBX_EXTFUN(int)         dbx_next                      (unsigned char *input, unsigned char *output);
DBX_EXTFUN(int)         dbx_previous                  (unsigned char *input, unsigned char *output);
DBX_EXTFUN(int)         dbx_delete                    (unsigned char *input, unsigned char *output);
DBX_EXTFUN(int)         dbx_defined                   (unsigned char *input, unsigned char *output);
DBX_EXTFUN(int)         dbx_increment                 (unsigned char *input, unsigned char *output);
DBX_EXTFUN(int)         dbx_function                  (unsigned char *input, unsigned char *output);
DBX_EXTFUN(int)         dbx_classmethod               (unsigned char *input, unsigned char *output);
DBX_EXTFUN(int)         dbx_method                    (unsigned char *input, unsigned char *output);
DBX_EXTFUN(int)         dbx_getproperty               (unsigned char *input, unsigned char *output);
DBX_EXTFUN(int)         dbx_setproperty               (unsigned char *input, unsigned char *output);
DBX_EXTFUN(int)         dbx_closeinstance             (unsigned char *input, unsigned char *output);
DBX_EXTFUN(int)         dbx_getnamespace              (unsigned char *input, unsigned char *output);
DBX_EXTFUN(int)         dbx_setnamespace              (unsigned char *input, unsigned char *output);
DBX_EXTFUN(int)         dbx_sleep                     (int period_ms);
DBX_EXTFUN(int)         dbx_benchmark                 (unsigned char *inputstr, unsigned char *outputstr);

int                     isc_load_library              (DBXCON *pcon);
int                     isc_authenticate              (DBXCON *pcon);
int                     isc_open                      (DBXCON *pcon);
int                     isc_parse_zv                  (char *zv, DBXZV * p_isc_sv);
int                     isc_change_namespace          (DBXCON *pcon, char *nspace);
int                     isc_pop_value                 (DBXCON *pcon, DBXVAL *value, int required_type);
int                     isc_error_message             (DBXCON *pcon, int error_code);

int                     ydb_load_library              (DBXCON *pcon);
int                     ydb_open                      (DBXCON *pcon);
int                     ydb_parse_zv                  (char *zv, DBXZV * p_isc_sv);
int                     ydb_error_message             (DBXCON *pcon, int error_code);
int                     ydb_function                  (DBXCON *pcon, DBXFUN *pfun);

DBXCON *                mgw_unpack_header             (unsigned char *input, unsigned char *output);
int                     mgw_unpack_arguments          (DBXCON *pcon);
int                     mgw_global_reference          (DBXCON *pcon);
int                     mgw_function_reference        (DBXCON *pcon, DBXFUN *pfun);
int                     mgw_class_reference           (DBXCON *pcon, short context);
int                     mgw_add_block_size            (DBXSTR *block, unsigned long offset, unsigned long data_len, int dsort, int dtype);
unsigned long           mgw_get_block_size            (DBXSTR *block, unsigned long offset, int *dsort, int *dtype);
unsigned long           mgw_get_size                  (unsigned char *str);
void *                  mgw_realloc                   (void *p, int curr_size, int new_size, short id);
void *                  mgw_malloc                    (int size, short id);
int                     mgw_free                      (void *p, short id);
int                     mgw_ucase                     (char *string);
int                     mgw_lcase                     (char *string);
int                     mgw_create_string             (DBXCON *pcon, void *data, short type);
int                     mgw_buffer_dump               (DBXCON *pcon, void *buffer, unsigned int len, char *title, short mode);
int                     mgw_log_event                 (DBXDEBUG *p_debug, char *message, char *title, int level);
int                     mgw_pause                     (int msecs);
DBXPLIB                 mgw_dso_load                  (char *library);
DBXPROC                 mgw_dso_sym                   (DBXPLIB p_library, char *symbol);
int                     mgw_dso_unload                (DBXPLIB p_library);
DBXTHID                 mgw_current_thread_id         (void);
unsigned long           mgw_current_process_id        (void);
int                     mgw_error_message             (DBXCON *pcon, int error_code);
int                     mgw_set_error_message         (DBXCON *pcon);
int                     mgw_cleanup                   (DBXCON *pcon);

int                     mgw_mutex_create              (DBXMUTEX *p_mutex);
int                     mgw_mutex_lock                (DBXMUTEX *p_mutex, int timeout);
int                     mgw_mutex_unlock              (DBXMUTEX *p_mutex);
int                     mgw_mutex_destroy             (DBXMUTEX *p_mutex);
int                     mgw_sleep                     (unsigned long msecs);

#endif


