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

/*
   Development Diary (in brief):

Version 1.0.1 20 April 2017:
   First release.

Version 1.0.2 3 February 2018:
   Access to Cache Objects added.

Version 1.0.3 3 July 2019:
   Open Source release.

*/


#include "mg_dbasys.h"
#include "mg_dba.h"

#if !defined(_WIN32)
extern int errno;
#endif

static DBXCON *connection[DBX_MAXCONS];


#if defined(_WIN32)
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
   switch (fdwReason)
   { 
      case DLL_PROCESS_ATTACH:
         break;
      case DLL_THREAD_ATTACH:
         break;
      case DLL_THREAD_DETACH:
         break;
      case DLL_PROCESS_DETACH:
         break;
   }
   return TRUE;
}
#endif


DBX_EXTFUN(int) dbx_init()
{
   int n;

   for (n = 0; n < DBX_MAXCONS; n ++) {
      connection[n] = NULL;
   }

   return 0;
}


DBX_EXTFUN(int) dbx_version(int index, char *output, int output_len)
{
   DBXCON *pcon;

   pcon = NULL;
   if (index >= 0 && index < DBX_MAXCONS) {
      pcon = connection[index];
   }

#if defined(_WIN32)
   sprintf((char *) output, "mg_dba.dll:%s", DBX_VERSION);
#else
   sprintf((char *) output, "mg_dba.so:%s", DBX_VERSION);
#endif

   if (pcon) {
      if (pcon->p_zv->version[0]) {
         if (pcon->p_zv->product == DBX_DBTYPE_YOTTADB)
            strcat((char *) output, "; YottaDB:");
         else if (pcon->p_zv->product == DBX_DBTYPE_IRIS)
            strcat((char *) output, "; InterSystems IRIS:");
         else
            strcat((char *) output, "; InterSystems Cache:");
         strcat((char *) output, pcon->p_zv->version);
      }

      mgw_create_string(pcon, (void *) output, DBX_DTYPE_STR);
   }

   return 0;
}


DBX_EXTFUN(int) dbx_open(unsigned char *input, unsigned char *output)
{
   int rc, n, len, error_code;
   char buffer[1024];
   DBXCON *pcon;
   char *p, *p1, *p2;

   pcon = (DBXCON *) mgw_malloc(sizeof(DBXCON), 0);
   memset((void *) pcon, 0, sizeof(DBXCON));
   connection[0] = pcon;

   pcon->p_isc_so = NULL;
   pcon->p_ydb_so = NULL;

   pcon->p_debug = &pcon->debug;
   pcon->p_isc_mutex = &pcon->isc_mutex;
   pcon->p_zv = &pcon->zv;

   pcon->p_debug->debug = 0;
   pcon->p_debug->p_fdebug = stdout;

   pcon->input_str.len_used = 0;

   pcon->output_val.svalue.len_alloc = 1024;
   pcon->output_val.svalue.len_used = 0;
   pcon->output_val.offset = 0;
   pcon->output_val.type = DBX_DTYPE_DBXSTR;

   for (n = 0; n < DBX_MAXARGS; n ++) {
      pcon->args[n].type = DBX_DTYPE_NONE;
      pcon->args[n].cvalue.pstr = NULL;
   }

   mgw_unpack_header(input, output);
   mgw_unpack_arguments(pcon);
/*
   printf("\ndbx_open : pcon->p_isc_so->libdir=%s; pcon->p_isc_so->libnam=%s; pcon->p_isc_so->loaded=%d; pcon->p_isc_so->functions_enabled=%d; pcon->p_isc_so->merge_enabled=%d;\n", pcon->p_isc_so->libdir, pcon->p_isc_so->libnam, pcon->p_isc_so->loaded, pcon->p_isc_so->functions_enabled, pcon->p_isc_so->merge_enabled);
   printf("\ndbx_open : pcon->p_ydb_so->loaded=%d;\n", pcon->p_ydb_so->loaded);
*/

   if (pcon->p_debug->debug == 1) {
      fprintf(pcon->p_debug->p_fdebug, "\r\n   >>> dbx_open");
      fflush(pcon->p_debug->p_fdebug);
   }

   pcon->pid = 0;

   error_code = 0;

   pcon->dbtype = 0;
   pcon->shdir[0] = '\0';

   for (n = 0; n < pcon->argc; n ++) {

      p = (char *) pcon->args[n].svalue.buf_addr;
      len = pcon->args[n].svalue.len_used;

      switch (n) {
         case 0:
            len = (len < 30) ? len : 30;
            strncpy(buffer, p, len);
            buffer[len] = '\0';
            mgw_lcase(buffer);

            if (!strcmp(buffer, "cache"))
               pcon->dbtype = DBX_DBTYPE_CACHE;
            else if (!strcmp(buffer, "iris"))
               pcon->dbtype = DBX_DBTYPE_IRIS;
            else if (!strcmp(buffer, "yottadb"))
               pcon->dbtype = DBX_DBTYPE_YOTTADB;
            break;
         case 1:
            len = (len < 250) ? len : 250;
            strncpy(pcon->shdir, p, len);
            pcon->shdir[len] = '\0';
            break;
         case 2:
            len = (len < 60) ? len : 60;
            strncpy(pcon->username, p, len);
            pcon->username[len] = '\0';
            break;
         case 3:
            len = (len < 60) ? len : 60;
            strncpy(pcon->password, p, len);
            pcon->password[len] = '\0';
            break;
         case 4:
            len = (len < 60) ? len : 60;
            strncpy(pcon->nspace, p, len);
            pcon->nspace[len] = '\0';
            break;
         case 5:
            len = (len < 60) ? len : 60;
            strncpy(pcon->input_device, p, len);
            pcon->input_device[len] = '\0';
            break;
         case 6:
            len = (len < 60) ? len : 60;
            strncpy(pcon->output_device, p, len);
            pcon->output_device[len] = '\0';
            break;
         case 7:
            len = (len < 60) ? len : 60;
            strncpy(pcon->debug_str, p, len);
            if (len) {
               pcon->p_debug->debug = 1;
            }
            break;
         case 8:
            len = (len < 1020) ? len : 1020;
            strncpy(buffer, p, len);
         default:
            break;
      }
   }

   p = buffer;
   p2 = p;
   while ((p2 = strstr(p, "\n"))) {
      *p2 = '\0';
      p1 = strstr(p, "=");
      if (p1) {
         *p1 = '\0';
         p1 ++;
#if defined(_WIN32)
         SetEnvironmentVariable((LPCTSTR) p, (LPCTSTR) p1);
#else
         /* printf("\nLinux : environment variable p=%s p1=%s;", p, p1); */
         setenv(p, p1, 1);
#endif
      }
      else {
         break;
      }
      p = p2 + 1;
   }

   if (!pcon->dbtype) {
      strcpy(pcon->error, "Unable to determine the database type");
      rc = CACHE_NOCON;
      goto dbx_open_exit;
   }

   if (!pcon->shdir[0]) {
      strcpy(pcon->error, "Unable to determine the path to the database installation");
      rc = CACHE_NOCON;
      goto dbx_open_exit;
   }

   if (pcon->dbtype != DBX_DBTYPE_YOTTADB) {
      rc = isc_open(pcon);
   }
   else {
      rc = ydb_open(pcon);
   }

dbx_open_exit:

   if (rc == CACHE_SUCCESS) {
      mgw_create_string(pcon, (void *) &rc, DBX_DTYPE_INT);
   }
   else {
      mgw_error_message(pcon, rc);
   }

   return 0;
}


DBX_EXTFUN(int) dbx_close(unsigned char *input, unsigned char *output)
{
   int rc, rc1, narg;
   DBXCON *pcon;

   pcon = mgw_unpack_header(input, output);

   if (!pcon) {
      return -3;
   }

   narg = mgw_unpack_arguments(pcon);

   if (pcon->p_debug->debug == 1) {
      fprintf(pcon->p_debug->p_fdebug, "\r\n   >>> dbx_close");
      fflush(pcon->p_debug->p_fdebug);
   }

   rc1 = 0;

   if (pcon->dbtype == DBX_DBTYPE_YOTTADB) {
      if (pcon->p_ydb_so->loaded) {
         rc = pcon->p_ydb_so->p_ydb_exit();
         printf("\r\np_ydb_exit=%d\r\n", rc);
      }

      strcpy(pcon->error, "");
      mgw_create_string(pcon, (void *) "1", DBX_DTYPE_STR);
/*
      mgw_dso_unload(pcon->p_ydb_so->p_library); 
      pcon->p_ydb_so->p_library = NULL;
      pcon->p_ydb_so->loaded = 0;
*/
      strcpy(pcon->p_ydb_so->libdir, "");
      strcpy(pcon->p_ydb_so->libnam, "");

   }
   else {
      if (pcon->p_isc_so->loaded) {

         DBX_LOCK(rc, 0);

         rc = pcon->p_isc_so->p_CacheEnd();
         rc1 = rc;

         DBX_UNLOCK(rc);

         if (pcon->p_debug->debug == 1) {
            fprintf(pcon->p_debug->p_fdebug, "\r\n       >>> %d==CacheEnd()", rc1);
            fflush(pcon->p_debug->p_fdebug);
         }

      }

      strcpy(pcon->error, "");
      mgw_create_string(pcon, (void *) "1", DBX_DTYPE_STR);

      mgw_dso_unload(pcon->p_isc_so->p_library); 

      pcon->p_isc_so->p_library = NULL;
      pcon->p_isc_so->loaded = 0;

      strcpy(pcon->p_isc_so->libdir, "");
      strcpy(pcon->p_isc_so->libnam, "");
   }

   strcpy(pcon->p_zv->version, "");

   strcpy(pcon->shdir, "");
   strcpy(pcon->username, "");
   strcpy(pcon->password, "");
   strcpy(pcon->nspace, "");
   strcpy(pcon->input_device, "");
   strcpy(pcon->output_device, "");
   strcpy(pcon->debug_str, "");

   rc = mgw_mutex_destroy(pcon->p_isc_mutex);

   if (pcon->p_debug->debug == 1) {
      fprintf(pcon->p_debug->p_fdebug, "\r\n");
      fflush(pcon->p_debug->p_fdebug);
      if (pcon->p_debug->p_fdebug != stdout) {
         fclose(pcon->p_debug->p_fdebug);
         pcon->p_debug->p_fdebug = stdout;
      }
      pcon->p_debug->debug = 0;
   }

   rc = CACHE_SUCCESS;
   if (rc == CACHE_SUCCESS) {
      mgw_create_string(pcon, (void *) &rc, DBX_DTYPE_INT);
   }
   else {
      mgw_error_message(pcon, rc);
   }

   return 0;
}


DBX_EXTFUN(int) dbx_set(unsigned char *input, unsigned char *output)
{
   int rc;
   DBXCON *pcon;

   pcon = mgw_unpack_header(input, output);

   if (!pcon) {
      return -3;
   }

   if (pcon->p_debug->debug == 1) {
      fprintf(pcon->p_debug->p_fdebug, "\r\n   >>> dbx_set");
      fflush(pcon->p_debug->p_fdebug);
   }

   DBX_LOCK(rc, 0);

   rc = mgw_global_reference(pcon);
   if (rc != CACHE_SUCCESS) {
      mgw_error_message(pcon, rc);
      goto dbx_set_exit;
   }

   if (pcon->dbtype == DBX_DBTYPE_YOTTADB) {
      rc = pcon->p_ydb_so->p_ydb_set_s(&(pcon->args[0].svalue), pcon->argc - 2, &pcon->yargs[0], &(pcon->args[pcon->argc - 1].svalue));
   }
   else {
      rc = pcon->p_isc_so->p_CacheGlobalSet(pcon->argc - 2);
   }

   if (rc == CACHE_SUCCESS) {
      mgw_create_string(pcon, (void *) &rc, DBX_DTYPE_INT);
   }
   else {
      mgw_error_message(pcon, rc);
   }

dbx_set_exit:

   DBX_UNLOCK(rc);

   mgw_cleanup(pcon);

   return 0;
}


DBX_EXTFUN(int) dbx_get(unsigned char *input, unsigned char *output)
{
   int rc;
   DBXCON *pcon;

   pcon = mgw_unpack_header(input, output);

   if (!pcon) {
      return -3;
   }

   if (pcon->p_debug->debug == 1) {
      fprintf(pcon->p_debug->p_fdebug, "\r\n   >>> dbx_get");
      fflush(pcon->p_debug->p_fdebug);
   }

   DBX_LOCK(rc, 0);

   rc = mgw_global_reference(pcon);
   if (rc != CACHE_SUCCESS) {
      mgw_error_message(pcon, rc);
      goto dbx_get_exit;
   }

   if (pcon->dbtype == DBX_DBTYPE_YOTTADB) {

      pcon->output_val.svalue.len_used = 0;
      pcon->output_val.svalue.buf_addr += 5;

      rc = pcon->p_ydb_so->p_ydb_get_s(&(pcon->args[0].svalue), pcon->argc - 1, &pcon->yargs[0], &(pcon->output_val.svalue));

      pcon->output_val.svalue.buf_addr -= 5;
      mgw_add_block_size(&(pcon->output_val.svalue), 0, (unsigned long) pcon->output_val.svalue.len_used, DBX_DSORT_DATA, DBX_DTYPE_DBXSTR);
   }
   else {
      rc = pcon->p_isc_so->p_CacheGlobalGet(pcon->argc - 1, 0); /* 1 for no 'undefined' */
   }

   if (rc == CACHE_ERUNDEF) {
      mgw_create_string(pcon, (void *) "", DBX_DTYPE_STR);
   }
   else if (rc == CACHE_SUCCESS) {
      if (pcon->dbtype != DBX_DBTYPE_YOTTADB) {
         isc_pop_value(pcon, &(pcon->output_val), DBX_DTYPE_DBXSTR);
      }
   }
   else {
      mgw_error_message(pcon, rc);
   }

dbx_get_exit:

   DBX_UNLOCK(rc);

   mgw_cleanup(pcon);

   return 0;
}


DBX_EXTFUN(int) dbx_next(unsigned char *input, unsigned char *output)
{
   int rc;
   DBXCON *pcon;

   pcon = mgw_unpack_header(input, output);

   if (!pcon) {
      return -3;
   }

   if (pcon->p_debug->debug == 1) {
      fprintf(pcon->p_debug->p_fdebug, "\r\n   >>> dbx_next");
      fflush(pcon->p_debug->p_fdebug);
   }

   DBX_LOCK(rc, 0);

   rc = mgw_global_reference(pcon);
   if (rc != CACHE_SUCCESS) {
      mgw_error_message(pcon, rc);
      goto dbx_next_exit;
   }

   if (pcon->dbtype == DBX_DBTYPE_YOTTADB) {

      pcon->output_val.svalue.len_used = 0;
      pcon->output_val.svalue.buf_addr += 5;

      rc = pcon->p_ydb_so->p_ydb_subscript_next_s(&(pcon->args[0].svalue), pcon->argc - 1, &pcon->yargs[0], &(pcon->output_val.svalue));

      pcon->output_val.svalue.buf_addr -= 5;
      mgw_add_block_size(&(pcon->output_val.svalue), 0, (unsigned long) pcon->output_val.svalue.len_used, DBX_DSORT_DATA, DBX_DTYPE_DBXSTR);
   }
   else {
      rc = pcon->p_isc_so->p_CacheGlobalOrder(pcon->argc - 1, 1, 0);
   }

   if (rc == CACHE_SUCCESS) {
      if (pcon->dbtype != DBX_DBTYPE_YOTTADB) {
         isc_pop_value(pcon, &(pcon->output_val), DBX_DTYPE_DBXSTR);
      }
   }
   else {
      mgw_error_message(pcon, rc);
   }

dbx_next_exit:

   DBX_UNLOCK(rc);

   mgw_cleanup(pcon);

   return 0;
}


DBX_EXTFUN(int) dbx_previous(unsigned char *input, unsigned char *output)
{
   int rc;
   DBXCON *pcon;

   pcon = mgw_unpack_header(input, output);

   if (!pcon) {
      return -3;
   }

   if (pcon->p_debug->debug == 1) {
      fprintf(pcon->p_debug->p_fdebug, "\r\n   >>> dbx_previous");
      fflush(pcon->p_debug->p_fdebug);
   }

   DBX_LOCK(rc, 0);

   rc = mgw_global_reference(pcon);
   if (rc != CACHE_SUCCESS) {
      mgw_error_message(pcon, rc);
      goto dbx_previous_exit;
   }

   if (pcon->dbtype == DBX_DBTYPE_YOTTADB) {

      pcon->output_val.svalue.len_used = 0;
      pcon->output_val.svalue.buf_addr += 5;

      rc = pcon->p_ydb_so->p_ydb_subscript_previous_s(&(pcon->args[0].svalue), pcon->argc - 1, &pcon->yargs[0], &(pcon->output_val.svalue));

      pcon->output_val.svalue.buf_addr -= 5;
      mgw_add_block_size(&(pcon->output_val.svalue), 0, (unsigned long) pcon->output_val.svalue.len_used, DBX_DSORT_DATA, DBX_DTYPE_DBXSTR);
   }
   else {
      rc = pcon->p_isc_so->p_CacheGlobalOrder(pcon->argc - 1, -1, 0);
   }

   if (rc == CACHE_SUCCESS) {
      if (pcon->dbtype != DBX_DBTYPE_YOTTADB) {
         isc_pop_value(pcon, &(pcon->output_val), DBX_DTYPE_DBXSTR);
      }
   }
   else {
      mgw_error_message(pcon, rc);
   }

dbx_previous_exit:

   DBX_UNLOCK(rc);

   mgw_cleanup(pcon);

   return 0;
}



DBX_EXTFUN(int) dbx_delete(unsigned char *input, unsigned char *output)
{
   int rc, n;
   DBXCON *pcon;

   pcon = mgw_unpack_header(input, output);

   if (!pcon) {
      return -3;
   }

   if (pcon->p_debug->debug == 1) {
      fprintf(pcon->p_debug->p_fdebug, "\r\n   >>> dbx_delete");
      fflush(pcon->p_debug->p_fdebug);
   }

   DBX_LOCK(rc, 0);

   rc = mgw_global_reference(pcon);
   if (rc != CACHE_SUCCESS) {
      mgw_error_message(pcon, rc);
      goto dbx_delete_exit;
   }

   if (pcon->dbtype == DBX_DBTYPE_YOTTADB) {
      rc = pcon->p_ydb_so->p_ydb_delete_s(&(pcon->args[0].svalue), pcon->argc - 1, &pcon->yargs[0], YDB_DEL_TREE);
   }
   else {
      rc = pcon->p_isc_so->p_CacheGlobalKill(pcon->argc - 1, 0);
   }

   if (rc == CACHE_SUCCESS) {
/*
      printf("\r\n defined  pcon->output_val.offset=%lu;\r\n",  pcon->output_val.offset);
*/
      sprintf((char *) pcon->output_val.svalue.buf_addr + pcon->output_val.offset, "%d", rc);
      n = (int) strlen((char *) pcon->output_val.svalue.buf_addr + pcon->output_val.offset);
      pcon->output_val.svalue.len_used += n;
      mgw_add_block_size(&(pcon->output_val.svalue), 0, (unsigned long) n, DBX_DSORT_DATA, DBX_DTYPE_DBXSTR);
   }
   else {
      mgw_error_message(pcon, rc);
   }

dbx_delete_exit:

   DBX_UNLOCK(rc);

   mgw_cleanup(pcon);

   return 0;
}


DBX_EXTFUN(int) dbx_defined(unsigned char *input, unsigned char *output)
{
   int rc, n;
   DBXCON *pcon;

   pcon = mgw_unpack_header(input, output);

   if (!pcon) {
      return -3;
   }

   if (pcon->p_debug->debug == 1) {
      fprintf(pcon->p_debug->p_fdebug, "\r\n   >>> dbx_defined");
      fflush(pcon->p_debug->p_fdebug);
   }

   DBX_LOCK(rc, 0);

   rc = mgw_global_reference(pcon);
   if (rc != CACHE_SUCCESS) {
      mgw_error_message(pcon, rc);
      goto dbx_data_exit;
   }

   if (pcon->dbtype == DBX_DBTYPE_YOTTADB) {
      rc = pcon->p_ydb_so->p_ydb_data_s(&(pcon->args[0].svalue), pcon->argc - 1, &pcon->yargs[0], (unsigned int *) &n);
   }
   else {
      rc = pcon->p_isc_so->p_CacheGlobalData(pcon->argc - 1, 0);
   }

   if (rc == CACHE_SUCCESS) {
      if (pcon->dbtype != DBX_DBTYPE_YOTTADB) {
         pcon->p_isc_so->p_CachePopInt(&n);
      }
      sprintf((char *) pcon->output_val.svalue.buf_addr + pcon->output_val.offset, "%d", n);
      n = (int) strlen((char *) pcon->output_val.svalue.buf_addr + pcon->output_val.offset);
      pcon->output_val.svalue.len_used += n;
      mgw_add_block_size(&(pcon->output_val.svalue), 0, (unsigned long) n, DBX_DSORT_DATA, DBX_DTYPE_DBXSTR);
   }
   else {
      mgw_error_message(pcon, rc);
   }


dbx_data_exit:

   DBX_UNLOCK(rc);

   mgw_cleanup(pcon);

   return 0;
}


DBX_EXTFUN(int) dbx_increment(unsigned char *input, unsigned char *output)
{
   int rc;
   DBXCON *pcon;

   pcon = mgw_unpack_header(input, output);

   if (!pcon) {
      return -3;
   }

   if (pcon->p_debug->debug == 1) {
      fprintf(pcon->p_debug->p_fdebug, "\r\n   >>> dbx_increment");
      fflush(pcon->p_debug->p_fdebug);
   }

   DBX_LOCK(rc, 0);

   rc = mgw_global_reference(pcon);
   if (rc != CACHE_SUCCESS) {
      mgw_error_message(pcon, rc);
      goto dbx_increment_exit;
   }

   if (pcon->dbtype == DBX_DBTYPE_YOTTADB) {

      pcon->output_val.svalue.len_used = 0;
      pcon->output_val.svalue.buf_addr += 5;

      rc = pcon->p_ydb_so->p_ydb_incr_s(&(pcon->args[0].svalue), pcon->argc - 2, &pcon->yargs[0], &(pcon->args[pcon->argc - 1].svalue), &(pcon->output_val.svalue));

      pcon->output_val.svalue.buf_addr -= 5;
      mgw_add_block_size(&(pcon->output_val.svalue), 0, (unsigned long) pcon->output_val.svalue.len_used, DBX_DSORT_DATA, DBX_DTYPE_DBXSTR);
   }
   else {
      rc = pcon->p_isc_so->p_CacheGlobalIncrement(pcon->argc - 2);
   }

   if (rc == CACHE_SUCCESS) {
      if (pcon->dbtype != DBX_DBTYPE_YOTTADB) {
         isc_pop_value(pcon, &(pcon->output_val), DBX_DTYPE_DBXSTR);
      }
   }
   else {
      mgw_error_message(pcon, rc);
   }

dbx_increment_exit:

   DBX_UNLOCK(rc);

   return 0;
}


DBX_EXTFUN(int) dbx_function(unsigned char *input, unsigned char *output)
{
   int rc;
   DBXFUN fun;
   DBXCON *pcon;

   pcon = mgw_unpack_header(input, output);

   if (!pcon) {
      return -3;
   }

   if (pcon->p_debug->debug == 1) {
      fprintf(pcon->p_debug->p_fdebug, "\r\n   >>> dbx_function");
      fflush(pcon->p_debug->p_fdebug);
   }

   if (pcon->dbtype != DBX_DBTYPE_YOTTADB && !pcon->p_isc_so->functions_enabled) {
      pcon->error_code = 2020;
      strcpy(pcon->error, "Cache functions are not available with this platform");
      mgw_set_error_message(pcon);
      return 0;
   }

   fun.rflag = 0;

   DBX_LOCK(rc, 0);

   rc = mgw_function_reference(pcon, &fun);
   if (rc != CACHE_SUCCESS) {
      mgw_error_message(pcon, rc);
      goto dbx_function_exit;
   }

   if (pcon->dbtype == DBX_DBTYPE_YOTTADB) {

      pcon->output_val.svalue.len_used = 0;
      pcon->output_val.svalue.buf_addr += 5;

      rc = ydb_function(pcon, &fun);

      pcon->output_val.svalue.buf_addr -= 5;
      mgw_add_block_size(&(pcon->output_val.svalue), 0, (unsigned long) pcon->output_val.svalue.len_used, DBX_DSORT_DATA, DBX_DTYPE_DBXSTR);
   }
   else {
      rc = pcon->p_isc_so->p_CacheExtFun(fun.rflag, pcon->argc - 1);
   }

   if (pcon->p_debug->debug == 1) {
      fprintf(pcon->p_debug->p_fdebug, "\r\n       >>> %d==CacheExtFun(%d, %d)", rc, fun.rflag, pcon->argc);
      fflush(pcon->p_debug->p_fdebug);
   }

   if (rc == CACHE_SUCCESS) {
      if (pcon->dbtype != DBX_DBTYPE_YOTTADB) {
         isc_pop_value(pcon, &(pcon->output_val), DBX_DTYPE_DBXSTR);
      }
   }
   else {
      mgw_error_message(pcon, rc);
   }

dbx_function_exit:

   DBX_UNLOCK(rc);

   return 0;
}


DBX_EXTFUN(int) dbx_classmethod(unsigned char *input, unsigned char *output)
{
   int rc;
   DBXCON *pcon;

   pcon = mgw_unpack_header(input, output);

   if (!pcon) {
      return -3;
   }

   if (pcon->p_debug->debug == 1) {
      fprintf(pcon->p_debug->p_fdebug, "\r\n   >>> dbx_classmethod");
      fflush(pcon->p_debug->p_fdebug);
   }

   if (pcon->dbtype == DBX_DBTYPE_YOTTADB) {
      pcon->error_code = 2020;
      strcpy(pcon->error, "Cache objects are not available with this platform");
      mgw_set_error_message(pcon);
      return 0;
   }

   if (!pcon->p_isc_so->objects_enabled) {
      pcon->error_code = 2020;
      strcpy(pcon->error, "Cache objects are not available with this platform");
      mgw_set_error_message(pcon);
      return 0;
   }

   DBX_LOCK(rc, 0);

   rc = mgw_class_reference(pcon, 0);
   if (rc != CACHE_SUCCESS) {
      mgw_error_message(pcon, rc);
      goto dbx_classmethod_exit;
   }

   rc = pcon->p_isc_so->p_CacheInvokeClassMethod(pcon->argc - 2);

   if (pcon->p_debug->debug == 1) {
      fprintf(pcon->p_debug->p_fdebug, "\r\n       >>> %d==CacheInvokeClassMethod(%d)", rc, pcon->argc);
      fflush(pcon->p_debug->p_fdebug);
   }

   if (rc == CACHE_SUCCESS) {
      isc_pop_value(pcon, &(pcon->output_val), DBX_DTYPE_DBXSTR);
   }
   else {
      mgw_error_message(pcon, rc);
   }

dbx_classmethod_exit:

   DBX_UNLOCK(rc);

   return 0;
}


DBX_EXTFUN(int) dbx_method(unsigned char *input, unsigned char *output)
{
   int rc;
   DBXCON *pcon;

   pcon = mgw_unpack_header(input, output);

   if (!pcon) {
      return -3;
   }

   if (pcon->p_debug->debug == 1) {
      fprintf(pcon->p_debug->p_fdebug, "\r\n   >>> dbx_method");
      fflush(pcon->p_debug->p_fdebug);
   }

   if (pcon->dbtype == DBX_DBTYPE_YOTTADB) {
      pcon->error_code = 2020;
      strcpy(pcon->error, "Cache objects are not available with this platform");
      mgw_set_error_message(pcon);
      return 0;
   }

   if (!pcon->p_isc_so->objects_enabled) {
      pcon->error_code = 2020;
      strcpy(pcon->error, "Cache objects are not available with this platform");
      mgw_set_error_message(pcon);
      return 0;
   }

   DBX_LOCK(rc, 0);

   rc = mgw_class_reference(pcon, 1);
   if (rc != CACHE_SUCCESS) {
      mgw_error_message(pcon, rc);
      goto dbx_method_exit;
   }

   rc = pcon->p_isc_so->p_CacheInvokeMethod(pcon->argc - 2);
   if (pcon->p_debug->debug == 1) {
      fprintf(pcon->p_debug->p_fdebug, "\r\n       >>> %d==CacheInvokeMethod(%d)", rc, pcon->argc);
      fflush(pcon->p_debug->p_fdebug);
   }

   if (rc == CACHE_SUCCESS) {
      isc_pop_value(pcon, &(pcon->output_val), DBX_DTYPE_DBXSTR);
   }
   else {
      mgw_error_message(pcon, rc);
   }

dbx_method_exit:

   DBX_UNLOCK(rc);

   return 0;
}


DBX_EXTFUN(int) dbx_getproperty(unsigned char *input, unsigned char *output)
{
   int rc;
   DBXCON *pcon;

   pcon = mgw_unpack_header(input, output);

   if (!pcon) {
      return -3;
   }

   if (pcon->p_debug->debug == 1) {
      fprintf(pcon->p_debug->p_fdebug, "\r\n   >>> dbx_getproperty");
      fflush(pcon->p_debug->p_fdebug);
   }

   if (pcon->dbtype == DBX_DBTYPE_YOTTADB) {
      pcon->error_code = 2020;
      strcpy(pcon->error, "Cache objects are not available with this platform");
      mgw_set_error_message(pcon);
      return 0;
   }

   if (!pcon->p_isc_so->objects_enabled) {
      pcon->error_code = 2020;
      strcpy(pcon->error, "Cache objects are not available with this platform");
      mgw_set_error_message(pcon);
      return 0;
   }

   DBX_LOCK(rc, 0);

   rc = mgw_class_reference(pcon, 2);
   if (rc != CACHE_SUCCESS) {
      mgw_error_message(pcon, rc);
      goto dbx_getproperty_exit;
   }

   rc = pcon->p_isc_so->p_CacheGetProperty();
   if (pcon->p_debug->debug == 1) {
      fprintf(pcon->p_debug->p_fdebug, "\r\n       >>> %d==CacheGetProperty()", rc);
      fflush(pcon->p_debug->p_fdebug);
   }

   if (rc == CACHE_SUCCESS) {
      isc_pop_value(pcon, &(pcon->output_val), DBX_DTYPE_DBXSTR);
   }
   else {
      mgw_error_message(pcon, rc);
   }

dbx_getproperty_exit:

   DBX_UNLOCK(rc);

   return 0;
}


DBX_EXTFUN(int) dbx_setproperty(unsigned char *input, unsigned char *output)
{
   int rc;
   DBXCON *pcon;

   pcon = mgw_unpack_header(input, output);

   if (!pcon) {
      return -3;
   }

   if (pcon->p_debug->debug == 1) {
      fprintf(pcon->p_debug->p_fdebug, "\r\n   >>> dbx_setproperty");
      fflush(pcon->p_debug->p_fdebug);
   }

   if (pcon->dbtype == DBX_DBTYPE_YOTTADB) {
      pcon->error_code = 2020;
      strcpy(pcon->error, "Cache objects are not available with this platform");
      mgw_set_error_message(pcon);
      return 0;
   }

   if (!pcon->p_isc_so->objects_enabled) {
      pcon->error_code = 2020;
      strcpy(pcon->error, "Cache objects are not available with this platform");
      mgw_set_error_message(pcon);
      return 0;
   }

   DBX_LOCK(rc, 0);

   rc = mgw_class_reference(pcon, 2);
   if (rc != CACHE_SUCCESS) {
      mgw_error_message(pcon, rc);
      goto dbx_setproperty_exit;
   }

   rc = pcon->p_isc_so->p_CacheSetProperty();
   if (pcon->p_debug->debug == 1) {
      fprintf(pcon->p_debug->p_fdebug, "\r\n       >>> %d==CacheSetProperty()", rc);
      fflush(pcon->p_debug->p_fdebug);
   }

   if (rc == CACHE_SUCCESS) {
      mgw_create_string(pcon, (void *) "", DBX_DTYPE_STR);
   }
   else {
      mgw_error_message(pcon, rc);
   }

dbx_setproperty_exit:

   DBX_UNLOCK(rc);

   return 0;
}



DBX_EXTFUN(int) dbx_closeinstance(unsigned char *input, unsigned char *output)
{
   int rc;
   DBXCON *pcon;

   pcon = mgw_unpack_header(input, output);

   if (!pcon) {
      return -3;
   }

   if (pcon->p_debug->debug == 1) {
      fprintf(pcon->p_debug->p_fdebug, "\r\n   >>> dbx_closeinstance");
      fflush(pcon->p_debug->p_fdebug);
   }

   if (pcon->dbtype == DBX_DBTYPE_YOTTADB) {
      pcon->error_code = 2020;
      strcpy(pcon->error, "Cache objects are not available with this platform");
      mgw_set_error_message(pcon);
      return 0;
   }

   if (!pcon->p_isc_so->objects_enabled) {
      pcon->error_code = 2020;
      strcpy(pcon->error, "Cache objects are not available with this platform");
      mgw_set_error_message(pcon);
      return 0;
   }

   DBX_LOCK(rc, 0);

   rc = mgw_class_reference(pcon, 3);

   if (pcon->p_debug->debug == 1) {
      fprintf(pcon->p_debug->p_fdebug, "\r\n       >>> %d==CacheCloseOref(%d)", rc, (int) strtol(pcon->args[0].svalue.buf_addr, NULL, 10));
      fflush(pcon->p_debug->p_fdebug);
   }

   if (rc == CACHE_SUCCESS) {
      isc_pop_value(pcon, &(pcon->output_val), DBX_DTYPE_DBXSTR);
   }
   else {
      mgw_error_message(pcon, rc);
   }

   DBX_UNLOCK(rc);

   return 0;
}


DBX_EXTFUN(int) dbx_getnamespace(unsigned char *input, unsigned char *output)
{
   int rc;
   DBXCON *pcon;
   CACHE_ASTR retval;
   CACHE_ASTR expr;

   pcon = mgw_unpack_header(input, output);

   if (!pcon) {
      return -3;
   }

   if (pcon->p_debug->debug == 1) {
      fprintf(pcon->p_debug->p_fdebug, "\r\n   >>> dbx_getnamespace");
      fflush(pcon->p_debug->p_fdebug);
   }

   if (pcon->dbtype == DBX_DBTYPE_YOTTADB) {
      pcon->error_code = 2020;
      strcpy(pcon->error, "Cache Namespace operations are not available with this platform");
      mgw_set_error_message(pcon);
      return 0;
   }

   if (pcon->p_isc_so->p_CacheEvalA == NULL) {
      pcon->error_code = 4020;
      strcpy(pcon->error, "Cache Namespace operations are not available with this platform");
      mgw_set_error_message(pcon);
      return 0;
   }

   strcpy((char *) expr.str, "$Namespace");
   expr.len = (unsigned short) strlen((char *) expr.str);

   mgw_unpack_arguments(pcon);

   DBX_LOCK(rc, 0);

   rc = pcon->p_isc_so->p_CacheEvalA(&expr);

   if (pcon->p_debug->debug == 1) {
      fprintf(pcon->p_debug->p_fdebug, "\r\n       >>> %d==CacheEvalA(%p(%s))", rc, &expr, expr.str);
      fflush(pcon->p_debug->p_fdebug);
   }

   if (rc == CACHE_SUCCESS) {
      retval.len = 256;
      rc = pcon->p_isc_so->p_CacheConvert(CACHE_ASTRING, &retval);

      if (retval.len < pcon->output_val.svalue.len_alloc) {

         strncpy((char *) pcon->output_val.svalue.buf_addr + pcon->output_val.offset, (char *) retval.str, retval.len);
         pcon->output_val.svalue.buf_addr[retval.len + pcon->output_val.offset] = '\0';
         pcon->output_val.svalue.len_used += retval.len;
         mgw_add_block_size(&(pcon->output_val.svalue), 0, (unsigned long) retval.len, DBX_DSORT_DATA, DBX_DTYPE_DBXSTR);

         if (pcon->p_debug->debug == 1) {
            fprintf(pcon->p_debug->p_fdebug, "\r\n       >>> %d==CacheConvert(%d, %p(%s))", rc, CACHE_ASTRING, &retval, (char *) pcon->output_val.svalue.buf_addr);
            fflush(pcon->p_debug->p_fdebug);
         }
      }
      else {
         rc = CACHE_BAD_NAMESPACE;
         mgw_error_message(pcon, rc);
      }
   }
   else {
      mgw_error_message(pcon, rc);
   }

   DBX_UNLOCK(rc);

   mgw_cleanup(pcon);

   return 0;
}


DBX_EXTFUN(int) dbx_setnamespace(unsigned char *input, unsigned char *output)
{
   int rc;
   DBXCON *pcon;
   char nspace[128];

   pcon = mgw_unpack_header(input, output);

   if (!pcon) {
      return -3;
   }

   if (pcon->p_debug->debug == 1) {
      fprintf(pcon->p_debug->p_fdebug, "\r\n   >>> dbx_setnamespace");
      fflush(pcon->p_debug->p_fdebug);
   }

   if (pcon->dbtype == DBX_DBTYPE_YOTTADB) {
      pcon->error_code = 2020;
      strcpy(pcon->error, "Cache Namespace operations are not available with this platform");
      mgw_set_error_message(pcon);
      return 0;
   }

   if (pcon->p_isc_so->p_CacheExecuteA == NULL) {
      pcon->error_code = 4020;
      strcpy(pcon->error, "Cache Namespace operations are not available with this platform");
      mgw_set_error_message(pcon);
      return 0;
   }

   mgw_unpack_arguments(pcon);

   DBX_LOCK(rc, 0);

   *nspace = '\0';
   if (pcon->argc > 0 && pcon->args[0].svalue.len_used < 120) {
      strncpy(nspace, (char *) pcon->args[0].svalue.buf_addr, pcon->args[0].svalue.len_used);
      nspace[pcon->args[0].svalue.len_used] = '\0';
   }

   rc = isc_change_namespace(pcon, nspace);

   if (rc == CACHE_SUCCESS) {
      strncpy((char *) pcon->output_val.svalue.buf_addr + pcon->output_val.offset, (char *) nspace, pcon->args[0].svalue.len_used);
      pcon->output_val.svalue.buf_addr[pcon->args[0].svalue.len_used + pcon->output_val.offset] = '\0';
      pcon->output_val.svalue.len_used += pcon->args[0].svalue.len_used;
      mgw_add_block_size(&(pcon->output_val.svalue), 0, (unsigned long) pcon->args[0].svalue.len_used, DBX_DSORT_DATA, DBX_DTYPE_DBXSTR);
   }
   else {
      mgw_error_message(pcon, rc);
   }

   DBX_UNLOCK(rc);

   mgw_cleanup(pcon);

   return 0;
}


DBX_EXTFUN(int) dbx_sleep(int period_ms)
{
   unsigned long msecs;

   msecs = (unsigned long) period_ms;

   mgw_sleep(msecs);

   return 0;

}


DBX_EXTFUN(int) dbx_benchmark(unsigned char *inputstr, unsigned char *outputstr)
{
   strcpy((char *) inputstr, "Output String");
   return 0;
}


int isc_load_library(DBXCON *pcon)
{
   int n, len, result;
   char primlib[DBX_ERROR_SIZE], primerr[DBX_ERROR_SIZE];
   char verfile[256], fun[64];
   char *libnam[16];

   strcpy(pcon->p_isc_so->libdir, pcon->shdir);
   strcpy(pcon->p_isc_so->funprfx, "Cache");
   strcpy(pcon->p_isc_so->dbname, "Cache");

   strcpy(verfile, pcon->shdir);
   len = (int) strlen(pcon->p_isc_so->libdir);
   if (pcon->p_isc_so->libdir[len - 1] == '/' || pcon->p_isc_so->libdir[len - 1] == '\\') {
      pcon->p_isc_so->libdir[len - 1] = '\0';
      len --;
   }
   for (n = len - 1; n > 0; n --) {
      if (pcon->p_isc_so->libdir[n] == '/') {
         strcpy((pcon->p_isc_so->libdir + (n + 1)), "bin/");
         break;
      }
      else if (pcon->p_isc_so->libdir[n] == '\\') {
         strcpy((pcon->p_isc_so->libdir + (n + 1)), "bin\\");
         break;
      }
   }

   /* printf("version=%s;\n", pcon->p_zv->version); */

   n = 0;
   if (pcon->dbtype == DBX_DBTYPE_IRIS) {
#if defined(_WIN32)
      libnam[n ++] = (char *) DBX_IRIS_DLL;
      libnam[n ++] = (char *) DBX_CACHE_DLL;
#else
#if defined(MACOSX)
      libnam[n ++] = (char *) DBX_IRIS_DYLIB;
      libnam[n ++] = (char *) DBX_IRIS_SO;
      libnam[n ++] = (char *) DBX_CACHE_DYLIB;
      libnam[n ++] = (char *) DBX_CACHE_SO;
#else
      libnam[n ++] = (char *) DBX_IRIS_SO;
      libnam[n ++] = (char *) DBX_IRIS_DYLIB;
      libnam[n ++] = (char *) DBX_CACHE_SO;
      libnam[n ++] = (char *) DBX_CACHE_DYLIB;
#endif
#endif
   }
   else {
#if defined(_WIN32)
      libnam[n ++] = (char *) DBX_CACHE_DLL;
      libnam[n ++] = (char *) DBX_IRIS_DLL;
#else
#if defined(MACOSX)
      libnam[n ++] = (char *) DBX_CACHE_DYLIB;
      libnam[n ++] = (char *) DBX_CACHE_SO;
      libnam[n ++] = (char *) DBX_IRIS_DYLIB;
      libnam[n ++] = (char *) DBX_IRIS_SO;
#else
      libnam[n ++] = (char *) DBX_CACHE_SO;
      libnam[n ++] = (char *) DBX_CACHE_DYLIB;
      libnam[n ++] = (char *) DBX_IRIS_SO;
      libnam[n ++] = (char *) DBX_IRIS_DYLIB;
#endif
#endif
   }

   libnam[n ++] = NULL;
   strcpy(pcon->p_isc_so->libnam, pcon->p_isc_so->libdir);
   len = (int) strlen(pcon->p_isc_so->libnam);

   for (n = 0; libnam[n]; n ++) {
      strcpy(pcon->p_isc_so->libnam + len, libnam[n]);
      if (!n) {
         strcpy(primlib, pcon->p_isc_so->libnam);
      }

      pcon->p_isc_so->p_library = mgw_dso_load(pcon->p_isc_so->libnam);
      if (pcon->p_debug->debug == 1) {
         fprintf(pcon->p_debug->p_fdebug, "\r\n       >>> %p==mgw_dso_load(%s)", pcon->p_isc_so->p_library, pcon->p_isc_so->libnam);
         fflush(pcon->p_debug->p_fdebug);
      }
      if (pcon->p_isc_so->p_library) {
         if (strstr(libnam[n], "iris")) {
            pcon->p_isc_so->iris = 1;
            strcpy(pcon->p_isc_so->funprfx, "Iris");
            strcpy(pcon->p_isc_so->dbname, "IRIS");
         }
         strcpy(pcon->error, "");
         pcon->error_code = 0;
         break;
      }

      if (!n) {
         int len1, len2;
         char *p;
#if defined(_WIN32)
         DWORD errorcode;
         LPVOID lpMsgBuf;

         lpMsgBuf = NULL;
         errorcode = GetLastError();
         sprintf(pcon->error, "Error loading %s Library: %s; Error Code : %ld", pcon->p_isc_so->dbname, primlib, errorcode);
         len2 = (int) strlen(pcon->error);
         len1 = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                        NULL,
                        errorcode,
                        /* MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), */
                        MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
                        (LPTSTR) &lpMsgBuf,
                        0,
                        NULL 
                        );
         if (lpMsgBuf && len1 > 0 && (DBX_ERROR_SIZE - len2) > 30) {
            strncpy(primerr, (const char *) lpMsgBuf, DBX_ERROR_SIZE - 1);
            p = strstr(primerr, "\r\n");
            if (p)
               *p = '\0';
            len1 = (DBX_ERROR_SIZE - (len2 + 10));
            if (len1 < 1)
               len1 = 0;
            primerr[len1] = '\0';
            p = strstr(primerr, "%1");
            if (p) {
               *p = 'I';
               *(p + 1) = 't';
            }
            strcat(pcon->error, " (");
            strcat(pcon->error, primerr);
            strcat(pcon->error, ")");
         }
         if (lpMsgBuf)
            LocalFree(lpMsgBuf);
#else
         p = (char *) dlerror();
         sprintf(primerr, "Cannot load %s library: Error Code: %d", pcon->p_isc_so->dbname, errno);
         len2 = strlen(pcon->error);
         if (p) {
            strncpy(primerr, p, DBX_ERROR_SIZE - 1);
            primerr[DBX_ERROR_SIZE - 1] = '\0';
            len1 = (DBX_ERROR_SIZE - (len2 + 10));
            if (len1 < 1)
               len1 = 0;
            primerr[len1] = '\0';
            strcat(pcon->error, " (");
            strcat(pcon->error, primerr);
            strcat(pcon->error, ")");
         }
#endif
      }
   }

   if (!pcon->p_isc_so->p_library) {
      goto isc_load_library_exit;
   }

   sprintf(fun, "%sSetDir", pcon->p_isc_so->funprfx);
   pcon->p_isc_so->p_CacheSetDir = (int (*) (char *)) mgw_dso_sym(pcon->p_isc_so->p_library, (char *) fun);
   if (!pcon->p_isc_so->p_CacheSetDir) {
      sprintf(pcon->error, "Error loading %s library: %s; Cannot locate the following function : %s", pcon->p_isc_so->dbname, pcon->p_isc_so->libnam, fun);
      goto isc_load_library_exit;
   }

   sprintf(fun, "%sSecureStartA", pcon->p_isc_so->funprfx);
   pcon->p_isc_so->p_CacheSecureStartA = (int (*) (CACHE_ASTRP, CACHE_ASTRP, CACHE_ASTRP, unsigned long, int, CACHE_ASTRP, CACHE_ASTRP)) mgw_dso_sym(pcon->p_isc_so->p_library, (char *) fun);
   if (!pcon->p_isc_so->p_CacheSecureStartA) {
      sprintf(pcon->error, "Error loading %s library: %s; Cannot locate the following function : %s", pcon->p_isc_so->dbname, pcon->p_isc_so->libnam, fun);
      goto isc_load_library_exit;
   }

   sprintf(fun, "%sEnd", pcon->p_isc_so->funprfx);
   pcon->p_isc_so->p_CacheEnd = (int (*) (void)) mgw_dso_sym(pcon->p_isc_so->p_library, (char *) fun);
   if (!pcon->p_isc_so->p_CacheEnd) {
      sprintf(pcon->error, "Error loading %s library: %s; Cannot locate the following function : %s", pcon->p_isc_so->dbname, pcon->p_isc_so->libnam, fun);
      goto isc_load_library_exit;
   }

   sprintf(fun, "%sExStrNew", pcon->p_isc_so->funprfx);
   pcon->p_isc_so->p_CacheExStrNew = (unsigned char * (*) (CACHE_EXSTRP, int)) mgw_dso_sym(pcon->p_isc_so->p_library, (char *) fun);
   if (!pcon->p_isc_so->p_CacheExStrNew) {
      sprintf(pcon->error, "Error loading %s library: %s; Cannot locate the following function : %s", pcon->p_isc_so->dbname, pcon->p_isc_so->libnam, fun);
      goto isc_load_library_exit;
   }
   sprintf(fun, "%sExStrNewW", pcon->p_isc_so->funprfx);
   pcon->p_isc_so->p_CacheExStrNewW = (unsigned short * (*) (CACHE_EXSTRP, int)) mgw_dso_sym(pcon->p_isc_so->p_library, (char *) fun);
   if (!pcon->p_isc_so->p_CacheExStrNewW) {
      sprintf(pcon->error, "Error loading %s library: %s; Cannot locate the following function : %s", pcon->p_isc_so->dbname, pcon->p_isc_so->libnam, fun);
      goto isc_load_library_exit;
   }
   sprintf(fun, "%sExStrNewH", pcon->p_isc_so->funprfx);
   pcon->p_isc_so->p_CacheExStrNewH = (wchar_t * (*) (CACHE_EXSTRP, int)) mgw_dso_sym(pcon->p_isc_so->p_library, (char *) fun);
/*
   if (!pcon->p_isc_so->p_CacheExStrNewH) {
      sprintf(pcon->error, "Error loading %s library: %s; Cannot locate the following function : %s", pcon->p_isc_so->dbname, pcon->p_isc_so->libnam, fun);
      goto isc_load_library_exit;
   }
*/
   sprintf(fun, "%sPushExStr", pcon->p_isc_so->funprfx);
   pcon->p_isc_so->p_CachePushExStr = (int (*) (CACHE_EXSTRP)) mgw_dso_sym(pcon->p_isc_so->p_library, (char *) fun);
   if (!pcon->p_isc_so->p_CachePushExStr) {
      sprintf(pcon->error, "Error loading %s library: %s; Cannot locate the following function : %s", pcon->p_isc_so->dbname, pcon->p_isc_so->libnam, fun);
      goto isc_load_library_exit;
   }
   sprintf(fun, "%sPushExStrW", pcon->p_isc_so->funprfx);
   pcon->p_isc_so->p_CachePushExStrW = (int (*) (CACHE_EXSTRP)) mgw_dso_sym(pcon->p_isc_so->p_library, (char *) fun);
   if (!pcon->p_isc_so->p_CachePushExStrW) {
      sprintf(pcon->error, "Error loading %s library: %s; Cannot locate the following function : %s", pcon->p_isc_so->dbname, pcon->p_isc_so->libnam, fun);
      goto isc_load_library_exit;
   }
   sprintf(fun, "%sPushExStrH", pcon->p_isc_so->funprfx);
   pcon->p_isc_so->p_CachePushExStrH = (int (*) (CACHE_EXSTRP)) mgw_dso_sym(pcon->p_isc_so->p_library, (char *) fun);

   sprintf(fun, "%sPopExStr", pcon->p_isc_so->funprfx);
   pcon->p_isc_so->p_CachePopExStr = (int (*) (CACHE_EXSTRP)) mgw_dso_sym(pcon->p_isc_so->p_library, (char *) fun);
   if (!pcon->p_isc_so->p_CachePopExStr) {
      sprintf(pcon->error, "Error loading %s library: %s; Cannot locate the following function : %s", pcon->p_isc_so->dbname, pcon->p_isc_so->libnam, fun);
      goto isc_load_library_exit;
   }
   sprintf(fun, "%sPopExStrW", pcon->p_isc_so->funprfx);
   pcon->p_isc_so->p_CachePopExStrW = (int (*) (CACHE_EXSTRP)) mgw_dso_sym(pcon->p_isc_so->p_library, (char *) fun);
   if (!pcon->p_isc_so->p_CachePopExStrW) {
      sprintf(pcon->error, "Error loading %s library: %s; Cannot locate the following function : %s", pcon->p_isc_so->dbname, pcon->p_isc_so->libnam, fun);
      goto isc_load_library_exit;
   }
   sprintf(fun, "%sPopExStrH", pcon->p_isc_so->funprfx);
   pcon->p_isc_so->p_CachePopExStrH = (int (*) (CACHE_EXSTRP)) mgw_dso_sym(pcon->p_isc_so->p_library, (char *) fun);

   sprintf(fun, "%sExStrKill", pcon->p_isc_so->funprfx);
   pcon->p_isc_so->p_CacheExStrKill = (int (*) (CACHE_EXSTRP)) mgw_dso_sym(pcon->p_isc_so->p_library, (char *) fun);
   if (!pcon->p_isc_so->p_CacheExStrKill) {
      sprintf(pcon->error, "Error loading %s library: %s; Cannot locate the following function : %s", pcon->p_isc_so->dbname, pcon->p_isc_so->libnam, fun);
      goto isc_load_library_exit;
   }
   sprintf(fun, "%sPushStr", pcon->p_isc_so->funprfx);
   pcon->p_isc_so->p_CachePushStr = (int (*) (int, Callin_char_t *)) mgw_dso_sym(pcon->p_isc_so->p_library, (char *) fun);
   if (!pcon->p_isc_so->p_CachePushStr) {
      sprintf(pcon->error, "Error loading %s library: %s; Cannot locate the following function : %s", pcon->p_isc_so->dbname, pcon->p_isc_so->libnam, fun);
      goto isc_load_library_exit;
   }
   sprintf(fun, "%sPushStrW", pcon->p_isc_so->funprfx);
   pcon->p_isc_so->p_CachePushStrW = (int (*) (int, short *)) mgw_dso_sym(pcon->p_isc_so->p_library, (char *) fun);
   if (!pcon->p_isc_so->p_CachePushStrW) {
      sprintf(pcon->error, "Error loading %s library: %s; Cannot locate the following function : %s", pcon->p_isc_so->dbname, pcon->p_isc_so->libnam, fun);
      goto isc_load_library_exit;
   }
   sprintf(fun, "%sPushStrH", pcon->p_isc_so->funprfx);
   pcon->p_isc_so->p_CachePushStrH = (int (*) (int, wchar_t *)) mgw_dso_sym(pcon->p_isc_so->p_library, (char *) fun);

   sprintf(fun, "%sPopStr", pcon->p_isc_so->funprfx);
   pcon->p_isc_so->p_CachePopStr = (int (*) (int *, Callin_char_t **)) mgw_dso_sym(pcon->p_isc_so->p_library, (char *) fun);
   if (!pcon->p_isc_so->p_CachePopStr) {
      sprintf(pcon->error, "Error loading %s library: %s; Cannot locate the following function : %s", pcon->p_isc_so->dbname, pcon->p_isc_so->libnam, fun);
      goto isc_load_library_exit;
   }
   sprintf(fun, "%sPopStrW", pcon->p_isc_so->funprfx);
   pcon->p_isc_so->p_CachePopStrW = (int (*) (int *, short **)) mgw_dso_sym(pcon->p_isc_so->p_library, (char *) fun);
   if (!pcon->p_isc_so->p_CachePopStrW) {
      sprintf(pcon->error, "Error loading %s library: %s; Cannot locate the following function : %s", pcon->p_isc_so->dbname, pcon->p_isc_so->libnam, fun);
      goto isc_load_library_exit;
   }
   sprintf(fun, "%sPopStrH", pcon->p_isc_so->funprfx);
   pcon->p_isc_so->p_CachePopStrH = (int (*) (int *, wchar_t **)) mgw_dso_sym(pcon->p_isc_so->p_library, (char *) fun);

   sprintf(fun, "%sPushDbl", pcon->p_isc_so->funprfx);
   pcon->p_isc_so->p_CachePushDbl = (int (*) (double)) mgw_dso_sym(pcon->p_isc_so->p_library, (char *) fun);
   if (!pcon->p_isc_so->p_CachePushDbl) {
      sprintf(pcon->error, "Error loading %s library: %s; Cannot locate the following function : %s", pcon->p_isc_so->dbname, pcon->p_isc_so->libnam, fun);
      goto isc_load_library_exit;
   }
   sprintf(fun, "%sPushIEEEDbl", pcon->p_isc_so->funprfx);
   pcon->p_isc_so->p_CachePushIEEEDbl = (int (*) (double)) mgw_dso_sym(pcon->p_isc_so->p_library, (char *) fun);
   if (!pcon->p_isc_so->p_CachePushIEEEDbl) {
      sprintf(pcon->error, "Error loading %s library: %s; Cannot locate the following function : %s", pcon->p_isc_so->dbname, pcon->p_isc_so->libnam, fun);
      goto isc_load_library_exit;
   }
   sprintf(fun, "%sPopDbl", pcon->p_isc_so->funprfx);
   pcon->p_isc_so->p_CachePopDbl = (int (*) (double *)) mgw_dso_sym(pcon->p_isc_so->p_library, (char *) fun);
   if (!pcon->p_isc_so->p_CachePopDbl) {
      sprintf(pcon->error, "Error loading %s library: %s; Cannot locate the following function : %s", pcon->p_isc_so->dbname, pcon->p_isc_so->libnam, fun);
      goto isc_load_library_exit;
   }
   sprintf(fun, "%sPushInt", pcon->p_isc_so->funprfx);
   pcon->p_isc_so->p_CachePushInt = (int (*) (int)) mgw_dso_sym(pcon->p_isc_so->p_library, (char *) fun);
   if (!pcon->p_isc_so->p_CachePushInt) {
      sprintf(pcon->error, "Error loading %s library: %s; Cannot locate the following function : %s", pcon->p_isc_so->dbname, pcon->p_isc_so->libnam, fun);
      goto isc_load_library_exit;
   }
   sprintf(fun, "%sPopInt", pcon->p_isc_so->funprfx);
   pcon->p_isc_so->p_CachePopInt = (int (*) (int *)) mgw_dso_sym(pcon->p_isc_so->p_library, (char *) fun);
   if (!pcon->p_isc_so->p_CachePopInt) {
      sprintf(pcon->error, "Error loading %s library: %s; Cannot locate the following function : %s", pcon->p_isc_so->dbname, pcon->p_isc_so->libnam, fun);
      goto isc_load_library_exit;
   }

   sprintf(fun, "%sPushInt64", pcon->p_isc_so->funprfx);
   pcon->p_isc_so->p_CachePushInt64 = (int (*) (CACHE_INT64)) mgw_dso_sym(pcon->p_isc_so->p_library, (char *) fun);
   if (!pcon->p_isc_so->p_CachePushInt64) {
      pcon->p_isc_so->p_CachePushInt64 = (int (*) (CACHE_INT64)) pcon->p_isc_so->p_CachePushInt;
   }
   sprintf(fun, "%sPushInt64", pcon->p_isc_so->funprfx);
   if (!pcon->p_isc_so->p_CachePushInt64) {
      sprintf(pcon->error, "Error loading %s library: %s; Cannot locate the following function : %s", pcon->p_isc_so->dbname, pcon->p_isc_so->libnam, fun);
      goto isc_load_library_exit;
   }
   sprintf(fun, "%sPopInt64", pcon->p_isc_so->funprfx);
   pcon->p_isc_so->p_CachePopInt64 = (int (*) (CACHE_INT64 *)) mgw_dso_sym(pcon->p_isc_so->p_library, (char *) fun);
   if (!pcon->p_isc_so->p_CachePopInt64) {
      pcon->p_isc_so->p_CachePopInt64 = (int (*) (CACHE_INT64 *)) pcon->p_isc_so->p_CachePopInt;
   }
   if (!pcon->p_isc_so->p_CachePopInt64) {
      sprintf(pcon->error, "Error loading %s library: %s; Cannot locate the following function : %s", pcon->p_isc_so->dbname, pcon->p_isc_so->libnam, fun);
      goto isc_load_library_exit;
   }

   sprintf(fun, "%sPushGlobal", pcon->p_isc_so->funprfx);
   pcon->p_isc_so->p_CachePushGlobal = (int (*) (int, const Callin_char_t *)) mgw_dso_sym(pcon->p_isc_so->p_library, (char *) fun);
   if (!pcon->p_isc_so->p_CachePushGlobal) {
      sprintf(pcon->error, "Error loading %s library: %s; Cannot locate the following function : %s", pcon->p_isc_so->dbname, pcon->p_isc_so->libnam, fun);
      goto isc_load_library_exit;
   }
   sprintf(fun, "%sPushGlobalX", pcon->p_isc_so->funprfx);
   pcon->p_isc_so->p_CachePushGlobalX = (int (*) (int, const Callin_char_t *, int, const Callin_char_t *)) mgw_dso_sym(pcon->p_isc_so->p_library, (char *) fun);
   if (!pcon->p_isc_so->p_CachePushGlobalX) {
      sprintf(pcon->error, "Error loading %s library: %s; Cannot locate the following function : %s", pcon->p_isc_so->dbname, pcon->p_isc_so->libnam, fun);
      goto isc_load_library_exit;
   }
   sprintf(fun, "%sGlobalGet", pcon->p_isc_so->funprfx);
   pcon->p_isc_so->p_CacheGlobalGet = (int (*) (int, int)) mgw_dso_sym(pcon->p_isc_so->p_library, (char *) fun);
   if (!pcon->p_isc_so->p_CacheGlobalGet) {
      sprintf(pcon->error, "Error loading %s library: %s; Cannot locate the following function : %s", pcon->p_isc_so->dbname, pcon->p_isc_so->libnam, fun);
      goto isc_load_library_exit;
   }
   sprintf(fun, "%sGlobalSet", pcon->p_isc_so->funprfx);
   pcon->p_isc_so->p_CacheGlobalSet = (int (*) (int)) mgw_dso_sym(pcon->p_isc_so->p_library, (char *) fun);
   if (!pcon->p_isc_so->p_CacheGlobalSet) {
      sprintf(pcon->error, "Error loading %s library: %s; Cannot locate the following function : %s", pcon->p_isc_so->dbname, pcon->p_isc_so->libnam, fun);
      goto isc_load_library_exit;
   }
   sprintf(fun, "%sGlobalData", pcon->p_isc_so->funprfx);
   pcon->p_isc_so->p_CacheGlobalData = (int (*) (int, int)) mgw_dso_sym(pcon->p_isc_so->p_library, (char *) fun);
   if (!pcon->p_isc_so->p_CacheGlobalData) {
      sprintf(pcon->error, "Error loading %s library: %s; Cannot locate the following function : %s", pcon->p_isc_so->dbname, pcon->p_isc_so->libnam, fun);
      goto isc_load_library_exit;
   }
   sprintf(fun, "%sGlobalKill", pcon->p_isc_so->funprfx);
   pcon->p_isc_so->p_CacheGlobalKill = (int (*) (int, int)) mgw_dso_sym(pcon->p_isc_so->p_library, (char *) fun);
   if (!pcon->p_isc_so->p_CacheGlobalKill) {
      sprintf(pcon->error, "Error loading %s library: %s; Cannot locate the following function : %s", pcon->p_isc_so->dbname, pcon->p_isc_so->libnam, fun);
      goto isc_load_library_exit;
   }
   sprintf(fun, "%sGlobalOrder", pcon->p_isc_so->funprfx);
   pcon->p_isc_so->p_CacheGlobalOrder = (int (*) (int, int, int)) mgw_dso_sym(pcon->p_isc_so->p_library, (char *) fun);
   if (!pcon->p_isc_so->p_CacheGlobalOrder) {
      sprintf(pcon->error, "Error loading %s library: %s; Cannot locate the following function : %s", pcon->p_isc_so->dbname, pcon->p_isc_so->libnam, fun);
      goto isc_load_library_exit;
   }
   sprintf(fun, "%sGlobalQuery", pcon->p_isc_so->funprfx);
   pcon->p_isc_so->p_CacheGlobalQuery = (int (*) (int, int, int)) mgw_dso_sym(pcon->p_isc_so->p_library, (char *) fun);
   if (!pcon->p_isc_so->p_CacheGlobalQuery) {
      sprintf(pcon->error, "Error loading %s library: %s; Cannot locate the following function : %s", pcon->p_isc_so->dbname, pcon->p_isc_so->libnam, fun);
      goto isc_load_library_exit;
   }
   sprintf(fun, "%sGlobalIncrement", pcon->p_isc_so->funprfx);
   pcon->p_isc_so->p_CacheGlobalIncrement = (int (*) (int)) mgw_dso_sym(pcon->p_isc_so->p_library, (char *) fun);
   if (!pcon->p_isc_so->p_CacheGlobalIncrement) {
      sprintf(pcon->error, "Error loading %s library: %s; Cannot locate the following function : %s", pcon->p_isc_so->dbname, pcon->p_isc_so->libnam, fun);
      goto isc_load_library_exit;
   }
   sprintf(fun, "%sGlobalRelease", pcon->p_isc_so->funprfx);
   pcon->p_isc_so->p_CacheGlobalRelease = (int (*) (void)) mgw_dso_sym(pcon->p_isc_so->p_library, (char *) fun);
   if (!pcon->p_isc_so->p_CacheGlobalRelease) {
      sprintf(pcon->error, "Error loading %s library: %s; Cannot locate the following function : %s", pcon->p_isc_so->dbname, pcon->p_isc_so->libnam, fun);
      goto isc_load_library_exit;
   }
   sprintf(fun, "%sAcquireLock", pcon->p_isc_so->funprfx);
   pcon->p_isc_so->p_CacheAcquireLock = (int (*) (int, int, int, int *)) mgw_dso_sym(pcon->p_isc_so->p_library, (char *) fun);
   if (!pcon->p_isc_so->p_CacheAcquireLock) {
      sprintf(pcon->error, "Error loading %s library: %s; Cannot locate the following function : %s", pcon->p_isc_so->dbname, pcon->p_isc_so->libnam, fun);
      goto isc_load_library_exit;
   }
   sprintf(fun, "%sReleaseAllLocks", pcon->p_isc_so->funprfx);
   pcon->p_isc_so->p_CacheReleaseAllLocks = (int (*) (void)) mgw_dso_sym(pcon->p_isc_so->p_library, (char *) fun);
   if (!pcon->p_isc_so->p_CacheReleaseAllLocks) {
      sprintf(pcon->error, "Error loading %s library: %s; Cannot locate the following function : %s", pcon->p_isc_so->dbname, pcon->p_isc_so->libnam, fun);
      goto isc_load_library_exit;
   }
   sprintf(fun, "%sReleaseLock", pcon->p_isc_so->funprfx);
   pcon->p_isc_so->p_CacheReleaseLock = (int (*) (int, int)) mgw_dso_sym(pcon->p_isc_so->p_library, (char *) fun);
   if (!pcon->p_isc_so->p_CacheReleaseLock) {
      sprintf(pcon->error, "Error loading %s library: %s; Cannot locate the following function : %s", pcon->p_isc_so->dbname, pcon->p_isc_so->libnam, fun);
      goto isc_load_library_exit;
   }
   sprintf(fun, "%sPushLock", pcon->p_isc_so->funprfx);
   pcon->p_isc_so->p_CachePushLock = (int (*) (int, const Callin_char_t *)) mgw_dso_sym(pcon->p_isc_so->p_library, (char *) fun);
   if (!pcon->p_isc_so->p_CachePushLock) {
      sprintf(pcon->error, "Error loading %s library: %s; Cannot locate the following function : %s", pcon->p_isc_so->dbname, pcon->p_isc_so->libnam, fun);
      goto isc_load_library_exit;
   }

   sprintf(fun, "%sAddGlobal", pcon->p_isc_so->funprfx);
   pcon->p_isc_so->p_CacheAddGlobal = (int (*) (int, const Callin_char_t *)) mgw_dso_sym(pcon->p_isc_so->p_library, (char *) fun);
   sprintf(fun, "%sAddGlobalDescriptor", pcon->p_isc_so->funprfx);
   pcon->p_isc_so->p_CacheAddGlobalDescriptor = (int (*) (int)) mgw_dso_sym(pcon->p_isc_so->p_library, (char *) fun);
   sprintf(fun, "%sAddSSVN", pcon->p_isc_so->funprfx);
   pcon->p_isc_so->p_CacheAddSSVN = (int (*) (int, const Callin_char_t *)) mgw_dso_sym(pcon->p_isc_so->p_library, (char *) fun);
   sprintf(fun, "%sAddSSVNDescriptor", pcon->p_isc_so->funprfx);
   pcon->p_isc_so->p_CacheAddSSVNDescriptor = (int (*) (int)) mgw_dso_sym(pcon->p_isc_so->p_library, (char *) fun);
   sprintf(fun, "%sMerge", pcon->p_isc_so->funprfx);
   pcon->p_isc_so->p_CacheMerge = (int (*) (void)) mgw_dso_sym(pcon->p_isc_so->p_library, (char *) fun);

   /* printf("pcon->p_isc_so->p_CacheAddGlobal=%p; pcon->p_isc_so->p_CacheAddGlobalDescriptor=%p; pcon->p_isc_so->p_CacheAddSSVN=%p; pcon->p_isc_so->p_CacheAddSSVNDescriptor=%p; pcon->p_isc_so->p_CacheMerge=%p;",  pcon->p_isc_so->p_CacheAddGlobal, pcon->p_isc_so->p_CacheAddGlobalDescriptor, pcon->p_isc_so->p_CacheAddSSVN, pcon->p_isc_so->p_CacheAddSSVNDescriptor, pcon->p_isc_so->p_CacheMerge); */

   if (pcon->p_isc_so->p_CacheAddGlobal && pcon->p_isc_so->p_CacheAddGlobalDescriptor && pcon->p_isc_so->p_CacheAddSSVN && pcon->p_isc_so->p_CacheAddSSVNDescriptor && pcon->p_isc_so->p_CacheMerge) {
      pcon->p_isc_so->merge_enabled = 1;
   }

   sprintf(fun, "%sPushFunc", pcon->p_isc_so->funprfx);
   pcon->p_isc_so->p_CachePushFunc = (int (*) (unsigned int *, int, const Callin_char_t *, int, const Callin_char_t *)) mgw_dso_sym(pcon->p_isc_so->p_library, (char *) fun);
   sprintf(fun, "%sExtFun", pcon->p_isc_so->funprfx);
   pcon->p_isc_so->p_CacheExtFun = (int (*) (unsigned int, int)) mgw_dso_sym(pcon->p_isc_so->p_library, (char *) fun);
   sprintf(fun, "%sPushRtn", pcon->p_isc_so->funprfx);
   pcon->p_isc_so->p_CachePushRtn = (int (*) (unsigned int *, int, const Callin_char_t *, int, const Callin_char_t *)) mgw_dso_sym(pcon->p_isc_so->p_library, (char *) fun);
   sprintf(fun, "%sDoFun", pcon->p_isc_so->funprfx);
   pcon->p_isc_so->p_CacheDoFun = (int (*) (unsigned int, int)) mgw_dso_sym(pcon->p_isc_so->p_library, (char *) fun);
   sprintf(fun, "%sDoRtn", pcon->p_isc_so->funprfx);
   pcon->p_isc_so->p_CacheDoRtn = (int (*) (unsigned int, int)) mgw_dso_sym(pcon->p_isc_so->p_library, (char *) fun);

   /* printf("\npcon->p_isc_so->p_CachePushFunc=%p; pcon->p_isc_so->p_CacheExtFun=%p; pcon->p_isc_so->p_CachePushRtn=%p; pcon->p_isc_so->p_CacheDoFun=%p; pcon->p_isc_so->p_CacheDoRtn=%p;\n",  pcon->p_isc_so->p_CachePushFunc, pcon->p_isc_so->p_CacheExtFun, pcon->p_isc_so->p_CachePushRtn, pcon->p_isc_so->p_CacheDoFun, pcon->p_isc_so->p_CacheDoRtn); */

   if (pcon->p_isc_so->p_CachePushFunc && pcon->p_isc_so->p_CacheExtFun && pcon->p_isc_so->p_CachePushRtn && pcon->p_isc_so->p_CacheDoFun && pcon->p_isc_so->p_CacheDoRtn) {
      pcon->p_isc_so->functions_enabled = 1;
      if (pcon->p_zv->version[0]) {
      pcon->p_isc_so->functions_enabled = 0;
      }
   }

   sprintf(fun, "%sCloseOref", pcon->p_isc_so->funprfx);
   pcon->p_isc_so->p_CacheCloseOref = (int (*) (unsigned int oref)) mgw_dso_sym(pcon->p_isc_so->p_library, (char *) fun);
   sprintf(fun, "%sIncrementCountOref", pcon->p_isc_so->funprfx);
   pcon->p_isc_so->p_CacheIncrementCountOref = (int (*) (unsigned int oref)) mgw_dso_sym(pcon->p_isc_so->p_library, (char *) fun);
   sprintf(fun, "%sPopOref", pcon->p_isc_so->funprfx);
   pcon->p_isc_so->p_CachePopOref = (int (*) (unsigned int * orefp)) mgw_dso_sym(pcon->p_isc_so->p_library, (char *) fun);
   sprintf(fun, "%sPushOref", pcon->p_isc_so->funprfx);
   pcon->p_isc_so->p_CachePushOref = (int (*) (unsigned int oref)) mgw_dso_sym(pcon->p_isc_so->p_library, (char *) fun);

   sprintf(fun, "%sInvokeMethod", pcon->p_isc_so->funprfx);
   pcon->p_isc_so->p_CacheInvokeMethod = (int (*) (int narg)) mgw_dso_sym(pcon->p_isc_so->p_library, (char *) fun);
   sprintf(fun, "%sPushMethod", pcon->p_isc_so->funprfx);
   pcon->p_isc_so->p_CachePushMethod = (int (*) (unsigned int oref, int mlen, const Callin_char_t * mptr, int flg)) mgw_dso_sym(pcon->p_isc_so->p_library, (char *) fun);
   sprintf(fun, "%sInvokeClassMethod", pcon->p_isc_so->funprfx);
   pcon->p_isc_so->p_CacheInvokeClassMethod = (int (*) (int narg)) mgw_dso_sym(pcon->p_isc_so->p_library, (char *) fun);
   sprintf(fun, "%sPushClassMethod", pcon->p_isc_so->funprfx);
   pcon->p_isc_so->p_CachePushClassMethod = (int (*) (int clen, const Callin_char_t * cptr, int mlen, const Callin_char_t * mptr, int flg)) mgw_dso_sym(pcon->p_isc_so->p_library, (char *) fun);

   sprintf(fun, "%sGetProperty", pcon->p_isc_so->funprfx);
   pcon->p_isc_so->p_CacheGetProperty = (int (*) (void)) mgw_dso_sym(pcon->p_isc_so->p_library, (char *) fun);
   sprintf(fun, "%sSetProperty", pcon->p_isc_so->funprfx);
   pcon->p_isc_so->p_CacheSetProperty = (int (*) (void)) mgw_dso_sym(pcon->p_isc_so->p_library, (char *) fun);
   sprintf(fun, "%sPushProperty", pcon->p_isc_so->funprfx);
   pcon->p_isc_so->p_CachePushProperty = (int (*) (unsigned int oref, int plen, const Callin_char_t * pptr)) mgw_dso_sym(pcon->p_isc_so->p_library, (char *) fun);

   if (pcon->p_isc_so->p_CacheCloseOref && pcon->p_isc_so->p_CacheIncrementCountOref && pcon->p_isc_so->p_CachePopOref && pcon->p_isc_so->p_CachePushOref
         && pcon->p_isc_so->p_CacheInvokeMethod && pcon->p_isc_so->p_CachePushMethod && pcon->p_isc_so->p_CacheInvokeClassMethod && pcon->p_isc_so->p_CachePushClassMethod
         && pcon->p_isc_so->p_CacheGetProperty && pcon->p_isc_so->p_CacheSetProperty && pcon->p_isc_so->p_CachePushProperty) {
      pcon->p_isc_so->objects_enabled = 1;
      if (pcon->p_zv->version[0]) {
         pcon->p_isc_so->objects_enabled = 0;
      }
   }

   sprintf(fun, "%sType", pcon->p_isc_so->funprfx);
   pcon->p_isc_so->p_CacheType = (int (*) (void)) mgw_dso_sym(pcon->p_isc_so->p_library, (char *) fun);

   sprintf(fun, "%sEvalA", pcon->p_isc_so->funprfx);
   pcon->p_isc_so->p_CacheEvalA = (int (*) (CACHE_ASTRP volatile)) mgw_dso_sym(pcon->p_isc_so->p_library, (char *) fun);
   sprintf(fun, "%sExecuteA", pcon->p_isc_so->funprfx);
   pcon->p_isc_so->p_CacheExecuteA = (int (*) (CACHE_ASTRP volatile)) mgw_dso_sym(pcon->p_isc_so->p_library, (char *) fun);
   sprintf(fun, "%sConvert", pcon->p_isc_so->funprfx);
   pcon->p_isc_so->p_CacheConvert = (int (*) (unsigned long, void *)) mgw_dso_sym(pcon->p_isc_so->p_library, (char *) fun);

   sprintf(fun, "%sErrorA", pcon->p_isc_so->funprfx);
   pcon->p_isc_so->p_CacheErrorA = (int (*) (CACHE_ASTRP, CACHE_ASTRP, int *)) mgw_dso_sym(pcon->p_isc_so->p_library, (char *) fun);
   sprintf(fun, "%sErrxlateA", pcon->p_isc_so->funprfx);
   pcon->p_isc_so->p_CacheErrxlateA = (int (*) (int, CACHE_ASTRP)) mgw_dso_sym(pcon->p_isc_so->p_library, (char *) fun);

   sprintf(fun, "%sEnableMultiThread", pcon->p_isc_so->funprfx);
   pcon->p_isc_so->p_CacheEnableMultiThread = (int (*) (void)) mgw_dso_sym(pcon->p_isc_so->p_library, (char *) fun);
   if (pcon->p_isc_so->p_CacheEnableMultiThread) {
      pcon->p_isc_so->multithread_enabled = 1;
   }
   else {
      pcon->p_isc_so->multithread_enabled = 0;
   }

   pcon->pid = mgw_current_process_id();

   pcon->p_isc_so->loaded = 1;

isc_load_library_exit:

   if (pcon->error[0]) {
      pcon->p_isc_so->loaded = 0;
      pcon->error_code = 1009;
      strcpy((char *) pcon->output_val.svalue.buf_addr, "0");
      result = CACHE_NOCON;

      return result;
   }

   return CACHE_SUCCESS;
}


int isc_authenticate(DBXCON *pcon)
{
   unsigned char reopen;
   int termflag, timeout;
   char buffer[256];
	CACHESTR pin, *ppin;
	CACHESTR pout, *ppout;
	CACHESTR pusername;
	CACHESTR ppassword;
	CACHESTR pexename;
	int rc;

   reopen = 0;
   termflag = 0;
	timeout = 15;

isc_authenticate_reopen:

   pcon->error_code = 0;
   *pcon->error = '\0';
	strcpy((char *) pexename.str, "mg_dba");
	pexename.len = (unsigned short) strlen((char *) pexename.str);
/*
	strcpy((char *) pin.str, "//./nul");
	strcpy((char *) pout.str, "//./nul");
	pin.len = (unsigned short) strlen((char *) pin.str);
	pout.len = (unsigned short) strlen((char *) pout.str);
*/
   ppin = NULL;
   if (pcon->input_device[0]) {
	   strcpy(buffer, pcon->input_device);
      mgw_lcase(buffer);
      if (!strcmp(buffer, "stdin")) {
	      strcpy((char *) pin.str, "");
         ppin = &pin;
      }
      else if (strcmp(pcon->input_device, DBX_NULL_DEVICE)) {
	      strcpy((char *) pin.str, pcon->input_device);
         ppin = &pin;
      }
      if (ppin)
	      ppin->len = (unsigned short) strlen((char *) ppin->str);
   }
   ppout = NULL;
   if (pcon->output_device[0]) {
	   strcpy(buffer, pcon->output_device);
      mgw_lcase(buffer);
      if (!strcmp(buffer, "stdout")) {
	      strcpy((char *) pout.str, "");
         ppout = &pout;
      }
      else if (strcmp(pcon->output_device, DBX_NULL_DEVICE)) {
	      strcpy((char *) pout.str, pcon->output_device);
         ppout = &pout;
      }
      if (ppout)
	      ppout->len = (unsigned short) strlen((char *) ppout->str);
   }

   if (ppin && ppout) { 
      termflag = CACHE_TTALL|CACHE_PROGMODE;
   }
   else {
      termflag = CACHE_TTNEVER|CACHE_PROGMODE;
   }

	strcpy((char *) pusername.str, pcon->username);
	strcpy((char *) ppassword.str, pcon->password);

	pusername.len = (unsigned short) strlen((char *) pusername.str);
	ppassword.len = (unsigned short) strlen((char *) ppassword.str);

#if !defined(_WIN32)

   signal(SIGUSR1, SIG_IGN);

   if (pcon->p_debug->debug == 1) {
#if defined(MACOSX)
      fprintf(pcon->p_debug->p_fdebug, "\r\n       >>> signal(SIGUSR1(%d), SIG_IGN(%p))", SIGUSR1, SIG_IGN);
#else
      fprintf(pcon->p_debug->p_fdebug, "\r\n       >>> signal(SIGUSR1(%d), SIG_IGN(%p))", SIGUSR1, SIG_IGN);
#endif
      fflush(pcon->p_debug->p_fdebug);
   }

#endif

	rc = pcon->p_isc_so->p_CacheSecureStartA(&pusername, &ppassword, &pexename, termflag, timeout, ppin, ppout);

   if (pcon->p_debug->debug == 1) {
/*
      fprintf(pcon->p_debug->p_fdebug, "\r\n       >>> %d==CacheSecureStartA(%p(%s), %p(%s), %p(%s), \r\n%d, %d, %p(%s), %p(%s))", rc, &pusername, pusername.str, &ppassword, ppassword.str, &pexename, pexename.str, termflag, timeout, &pin, pin.str, &pout, pout.str);
*/
      fprintf(pcon->p_debug->p_fdebug, "\r\n       >>> %d==CacheSecureStartA(%p(%s),", rc, &pusername, (char *) pusername.str);
      fprintf(pcon->p_debug->p_fdebug, "\r\n                                 %p(%s),", &ppassword, (char *) ppassword.str);
      fprintf(pcon->p_debug->p_fdebug, "\r\n                                 %p(%s),", &pexename, (char *) pexename.str);
      fprintf(pcon->p_debug->p_fdebug, "\r\n                                 %d, %d,", termflag, timeout);
      fprintf(pcon->p_debug->p_fdebug, "\r\n                                 %p(%s),", ppin, ppin ? (char *) ppin->str : "null");
      fprintf(pcon->p_debug->p_fdebug, "\r\n                                 %p(%s))", ppout, ppout ? (char *) ppout->str : "null");

      fflush(pcon->p_debug->p_fdebug);
   }

	if (rc != CACHE_SUCCESS) {
      pcon->error_code = rc;
	   if (rc == CACHE_ACCESSDENIED) {
	      sprintf(pcon->error, "Authentication: CacheSecureStart() : Access Denied : Check the audit log for the real authentication error (%d)\n", rc);
	      return 0;
	   }
	   if (rc == CACHE_CHANGEPASSWORD) {
	      sprintf(pcon->error, "Authentication: CacheSecureStart() : Password Change Required (%d)\n", rc);
	      return 0;
	   }
	   if (rc == CACHE_ALREADYCON) {
	      sprintf(pcon->error, "Authentication: CacheSecureStart() : Already Connected (%d)\n", rc);
	      return 1;
	   }
	   if (rc == CACHE_CONBROKEN) {
	      sprintf(pcon->error, "Authentication: CacheSecureStart() : Connection was formed and then broken by the server. (%d)\n", rc);
	      return 0;
	   }

	   if (rc == CACHE_FAILURE) {
	      sprintf(pcon->error, "Authentication: CacheSecureStart() : An unexpected error has occurred. (%d)\n", rc);
	      return 0;
	   }
	   if (rc == CACHE_STRTOOLONG) {
	      sprintf(pcon->error, "Authentication: CacheSecureStart() : prinp or prout is too long. (%d)\n", rc);
	      return 0;
	   }
	   sprintf(pcon->error, "Authentication: CacheSecureStart() : Failed (%d)\n", rc);
	   return 0;
   }

   if (pcon->p_isc_so->p_CacheEvalA && pcon->p_isc_so->p_CacheConvert) {
      CACHE_ASTR retval;
      CACHE_ASTR expr;

      strcpy((char *) expr.str, "$ZVersion");
      expr.len = (unsigned short) strlen((char *) expr.str);
      rc = pcon->p_isc_so->p_CacheEvalA(&expr);

      if (pcon->p_debug->debug == 1) {
         fprintf(pcon->p_debug->p_fdebug, "\r\n       >>> %d==CacheEvalA(%p(%s))", rc, &expr, expr.str);
         fflush(pcon->p_debug->p_fdebug);
      }

      if (rc == CACHE_CONBROKEN)
         reopen = 1;
      if (rc == CACHE_SUCCESS) {
         retval.len = 256;
         rc = pcon->p_isc_so->p_CacheConvert(CACHE_ASTRING, &retval);

         if (pcon->p_debug->debug == 1) {
            fprintf(pcon->p_debug->p_fdebug, "\r\n       >>> %d==CacheConvert(%d, %p(%s))", rc, CACHE_ASTRING, &retval, retval.str);
            fflush(pcon->p_debug->p_fdebug);
         }
         if (rc == CACHE_CONBROKEN)
            reopen = 1;
         if (rc == CACHE_SUCCESS) {
            isc_parse_zv((char *) retval.str, pcon->p_zv);
            sprintf(pcon->p_zv->version, "%d.%d.b%d", pcon->p_zv->majorversion, pcon->p_zv->minorversion, pcon->p_zv->mgw_build);
         }
      }
   }

   if (reopen) {
      goto isc_authenticate_reopen;
   }

   rc = isc_change_namespace(pcon, pcon->nspace);

   if (pcon->p_isc_so->multithread_enabled) {
      rc = pcon->p_isc_so->p_CacheEnableMultiThread();
      if (pcon->p_debug->debug == 1) {
         fprintf(pcon->p_debug->p_fdebug, "\r\n       >>> %d==CacheEnableMultiThread()", rc);
      }
   }
   else {
      rc = -1;
      if (pcon->p_debug->debug == 1) {
         fprintf(pcon->p_debug->p_fdebug, "\r\n       >>> %d==CacheEnableMultiThread() [Not Available with this version of Cache]", rc);
      }
   }

   return 1;
}


int isc_open(DBXCON *pcon)
{
   int rc, error_code, result;

   error_code = 0;

  if (!pcon->p_isc_so) {
      pcon->p_isc_so = (DBXISCSO *) mgw_malloc(sizeof(DBXISCSO), 0);
      if (!pcon->p_isc_so) {
         strcpy(pcon->error, "No Memory");
         pcon->error_code = 1009; 
         result = CACHE_NOCON;
         return result;
      }
      memset((void *) pcon->p_isc_so, 0, sizeof(DBXISCSO));
      pcon->p_isc_so->loaded = 0;
   }

   if (pcon->p_isc_so->loaded == 2) {
      strcpy(pcon->error, "Cannot create multiple connections to the database");
      pcon->error_code = 1009; 
      strncpy(pcon->error, pcon->error, DBX_ERROR_SIZE - 1);
      pcon->error[DBX_ERROR_SIZE - 1] = '\0';
      strcpy((char *) pcon->output_val.svalue.buf_addr, "0");
      rc = CACHE_NOCON;
      goto isc_open_exit;
   }

   if (!pcon->p_isc_so->loaded) {
      pcon->p_isc_so->merge_enabled = 0;
      pcon->p_isc_so->functions_enabled = 0;
      pcon->p_isc_so->objects_enabled = 0;
      pcon->p_isc_so->multithread_enabled = 0;
   }

   if (!pcon->p_isc_so->loaded) {
      rc = isc_load_library(pcon);
      if (rc != CACHE_SUCCESS) {
         goto isc_open_exit;
      }
   }

   rc = pcon->p_isc_so->p_CacheSetDir(pcon->shdir);

   if (pcon->p_debug->debug == 1) {
      fprintf(pcon->p_debug->p_fdebug, "\r\n       >>> %d==CacheSetDir(%s)", rc, pcon->shdir);
      fflush(pcon->p_debug->p_fdebug);
   }

   if (!isc_authenticate(pcon)) {
      pcon->error_code = error_code;
      strcpy((char *) pcon->output_val.svalue.buf_addr, "0");
      rc = CACHE_NOCON;
   }
   else {
      strcpy((char *) pcon->output_val.svalue.buf_addr, "1");
      pcon->p_isc_so->loaded = 2;
      rc = CACHE_SUCCESS;
   }

   if (pcon->p_debug->debug == 1) {
      fprintf(pcon->p_debug->p_fdebug, "\r\n");
      fflush(pcon->p_debug->p_fdebug);
   }

isc_open_exit:

   if (rc == CACHE_SUCCESS) {
      mgw_create_string(pcon, (void *) &rc, DBX_DTYPE_INT);
   }
   else {
      mgw_error_message(pcon, rc);
   }

   return 0;
}


int isc_parse_zv(char *zv, DBXZV * p_isc_sv)
{
   int result;
   double mgw_version;
   char *p, *p1, *p2;

   p_isc_sv->mgw_version = 0;
   p_isc_sv->majorversion = 0;
   p_isc_sv->minorversion = 0;
   p_isc_sv->mgw_build = 0;
   p_isc_sv->vnumber = 0;

   result = 0;

   p = strstr(zv, "Cache");
   if (p) {
      p_isc_sv->product = DBX_DBTYPE_CACHE;
   }
   else {
      p_isc_sv->product = DBX_DBTYPE_IRIS;
   }

   p = zv;
   mgw_version = 0;
   while (*(++ p)) {
      if (*(p - 1) == ' ' && isdigit((int) (*p))) {
         mgw_version = strtod(p, NULL);
         if (*(p + 1) == '.' && mgw_version >= 1.0 && mgw_version <= 5.2)
            break;
         else if (*(p + 4) == '.' && mgw_version >= 2000.0)
            break;
         mgw_version = 0;
      }
   }

   if (mgw_version > 0) {
      p_isc_sv->mgw_version = mgw_version;
      p_isc_sv->majorversion = (int) strtol(p, NULL, 10);
      p1 = strstr(p, ".");
      if (p1) {
         p_isc_sv->minorversion = (int) strtol(p1 + 1, NULL, 10);
      }
      p2 = strstr(p, "Build ");
      if (p2) {
         p_isc_sv->mgw_build = (int) strtol(p2 + 6, NULL, 10);
      }

      if (p_isc_sv->majorversion >= 2007)
         p_isc_sv->vnumber = (((p_isc_sv->majorversion - 2000) * 100000) + (p_isc_sv->minorversion * 10000) + p_isc_sv->mgw_build);
      else
         p_isc_sv->vnumber = ((p_isc_sv->majorversion * 100000) + (p_isc_sv->minorversion * 10000) + p_isc_sv->mgw_build);

      result = 1;
   }

   return result;
}


int isc_change_namespace(DBXCON *pcon, char *nspace)
{
   int rc, len;
   CACHE_ASTR expr;

   len = (int) strlen(nspace);
   if (len == 0 || len > 64) {
      return CACHE_ERNAMSP;
   }
   if (pcon->p_isc_so->p_CacheExecuteA == NULL) {
      return CACHE_ERNAMSP;
   }

   sprintf((char *) expr.str, "ZN \"%s\"", nspace); /* changes namespace */
   expr.len = (unsigned short) strlen((char *) expr.str);

   mgw_mutex_lock(pcon->p_isc_mutex, 0);

   rc = pcon->p_isc_so->p_CacheExecuteA(&expr);

   mgw_mutex_unlock(pcon->p_isc_mutex);

   if (pcon->p_debug->debug == 1) {
      fprintf(pcon->p_debug->p_fdebug, "\r\n       >>> %d==CacheExecuteA(%p(%s))", rc, &expr, expr.str);
      fflush(pcon->p_debug->p_fdebug);
   }

   return rc;
}


int isc_pop_value(DBXCON *pcon, DBXVAL *value, int required_type)
{
   int rc, rc1, ex, ctype, offset, oref;
   unsigned int n, max, len;
   char *pstr8, *p8, *outstr8;
   CACHE_EXSTR zstr;

   ex = 1;
   offset = 0;
   zstr.len = 0;
   zstr.str.ch = NULL;
   outstr8 = NULL;
   ctype = CACHE_ASTRING;

   if (pcon->p_isc_so->p_CacheType) {
      ctype = pcon->p_isc_so->p_CacheType();

      if (pcon->p_debug->debug == 1) {
         fprintf(pcon->p_debug->p_fdebug, "\r\n       >>> %d==CacheType()", ctype);
         fflush(pcon->p_debug->p_fdebug);
      }

      if (ctype == CACHE_OREF) {
         rc = pcon->p_isc_so->p_CachePopOref(&oref);

         value->type = DBX_DTYPE_OREF;
         value->num.oref = oref;
         sprintf((char *) value->svalue.buf_addr + value->offset, "%d", oref);
         value->svalue.len_used += (int) strlen((char *) value->svalue.buf_addr + value->offset);
         mgw_add_block_size(&(value->svalue), 0, (unsigned long) value->svalue.len_used - value->offset, DBX_DSORT_DATA, DBX_DTYPE_DBXSTR);

         if (pcon->p_debug->debug == 1) {
            fprintf(pcon->p_debug->p_fdebug, "\r\n       >>> %d==CachePopOref(%d)", rc, oref);
            fflush(pcon->p_debug->p_fdebug);
         }
         return rc;
      }
   }
   else {
      ctype = CACHE_ASTRING;
   }

   if (ex) {
      rc = pcon->p_isc_so->p_CachePopExStr(&zstr);
      len = zstr.len;
      outstr8 = (char *) zstr.str.ch;
      if (pcon->p_debug->debug == 1) {
         fprintf(pcon->p_debug->p_fdebug, "\r\n       >>> %d==CachePopExStr(%p {len=%d;str=%p})", rc, &zstr, zstr.len, (void *) zstr.str.ch);
         fflush(pcon->p_debug->p_fdebug);
      }
   }
   else {
      rc = pcon->p_isc_so->p_CachePopStr((int *) &len, (Callin_char_t **) &outstr8);
      if (pcon->p_debug->debug == 1) {
         fprintf(pcon->p_debug->p_fdebug, "\r\n       >>> %d==CachePopStr(%d, %p)", rc, len, outstr8);
         fflush(pcon->p_debug->p_fdebug);
      }
   }


   max = 0;
   if (value->svalue.len_alloc > 8) {
      max = (value->svalue.len_alloc - 2);
   }

   pstr8 = (char *) value->svalue.buf_addr;
   offset = value->offset;
   if (len >= max) {
      p8 = (char *) mgw_malloc(sizeof(char) * (len + 2), 301);
      if (p8) {
         if (value->svalue.buf_addr)
            mgw_free((void *) value->svalue.buf_addr, 301);
         value->svalue.buf_addr = (unsigned char *) p8;
         pstr8 = (char *) value->svalue.buf_addr;
         max = len;
      }
   }
   for (n = 0; n < len; n ++) {
      if (n > max)
         break;
      pstr8[n + offset] = (char) outstr8[n];
   }
   pstr8[n + offset] = '\0';

   value->svalue.len_used += n;

   mgw_add_block_size(&(value->svalue), 0, (unsigned long) n, DBX_DSORT_DATA, DBX_DTYPE_DBXSTR);

   if (ex) {
      rc1 = pcon->p_isc_so->p_CacheExStrKill(&zstr);
   }

   if (pcon->p_debug->debug == 1) {
      char buffer[128];
      if (value->svalue.len_used > 60) {
         strncpy(buffer, (char *) value->svalue.buf_addr, 60);
         buffer[60] = '\0';
         strcat(buffer, " ...");
      }
      else {
         strcpy(buffer, (char *) value->svalue.buf_addr);
      }
      fprintf(pcon->p_debug->p_fdebug, "\r\n           >>> %s", buffer);
      fflush(pcon->p_debug->p_fdebug);
      if (ex) {
         fprintf(pcon->p_debug->p_fdebug, "\r\n       >>> %d==CacheExStrKill(%p)", rc1, &zstr);
      }
   }

   return rc;
}


int isc_error_message(DBXCON *pcon, int error_code)
{
   int size, size1, len, rc;
   CACHE_ASTR *pcerror;

   size = DBX_ERROR_SIZE;

   if (pcon) {
      if (error_code < 0) {
         pcon->error_code = 900 + (error_code * -1);
      }
      else {
         pcon->error_code = error_code;
      }
   }

   if (error_code == CACHE_BAD_GLOBAL) {
      if (pcon->error[0]) {
         goto isc_error_message_exit;
      }
   }
   if (error_code == CACHE_BAD_FUNCTION || error_code == CACHE_BAD_CLASS || error_code == CACHE_BAD_METHOD) {
      if (pcon->error[0]) {
         goto isc_error_message_exit;
      }
   }
   if (error_code == CACHE_BAD_STRING) {
      if (pcon->error[0]) {
         goto isc_error_message_exit;
      }
   }
   pcon->error[0] = '\0';

   size1 = size;

   pcerror = (CACHE_ASTR *) mgw_malloc(sizeof(CACHE_ASTR), 801);
   if (pcerror) {
      pcerror->str[0] = '\0';
      pcerror->len = 50;
      rc = pcon->p_isc_so->p_CacheErrxlateA(error_code, pcerror);
      if (pcon->p_debug->debug == 1) {
         fprintf(pcon->p_debug->p_fdebug, "\r\n   >>> %d==CacheErrxlateA(%d)", rc, error_code);
         fflush(pcon->p_debug->p_fdebug);
      }
      pcerror->str[50] = '\0';

      if (pcerror->len > 0) {
         len = pcerror->len;
         if (len >= DBX_ERROR_SIZE) {
            len = DBX_ERROR_SIZE - 1;
         }
         strncpy(pcon->error, (char *) pcerror->str, len);
         pcon->error[len] = '\0';
      }
      mgw_free((void *) pcerror, 801);
      size1 -= (int) strlen(pcon->error);
   }

   switch (error_code) {
      case CACHE_SUCCESS:
         strncat(pcon->error, "Operation completed successfully!", size1 - 1);
         break;
      case CACHE_ACCESSDENIED:
         strncat(pcon->error, "Authentication has failed. Check the audit log for the real authentication error.", size1 - 1);
         break;
      case CACHE_ALREADYCON:
         strncat(pcon->error, "Connection already existed. Returned if you call CacheSecureStartH from a $ZF function.", size1 - 1);
         break;
      case CACHE_CHANGEPASSWORD:
         strncat(pcon->error, "Password change required. This return value is only returned if you are using Cach\xe7 authentication.", size1 - 1);
         break;
      case CACHE_CONBROKEN:
         strncat(pcon->error, "Connection was broken by the server. Check arguments for validity.", size1 - 1);
         break;
      case CACHE_FAILURE:
         strncat(pcon->error, "An unexpected error has occurred.", size1 - 1);
         break;
      case CACHE_STRTOOLONG:
         strncat(pcon->error, "String is too long.", size1 - 1);
         break;
      case CACHE_NOCON:
         strncat(pcon->error, "No connection has been established.", size1 - 1);
         break;
      case CACHE_ERSYSTEM:
         strncat(pcon->error, "Either the Cache engine generated a <SYSTEM> error, or callin detected an internal data inconsistency.", size1 - 1);
         break;
      case CACHE_ERARGSTACK:
         strncat(pcon->error, "Argument stack overflow.", size1 - 1);
         break;
      case CACHE_ERSTRINGSTACK:
         strncat(pcon->error, "String stack overflow.", size1 - 1);
         break;
      case CACHE_ERPROTECT:
         strncat(pcon->error, "Protection violation.", size1 - 1);
         break;
      case CACHE_ERUNDEF:
         strncat(pcon->error, "Global node is undefined", size1 - 1);
         break;
      case CACHE_ERUNIMPLEMENTED:
         strncat(pcon->error, "String is undefined OR feature is not implemented.", size1 - 1);
         break;
      case CACHE_ERSUBSCR:
         strncat(pcon->error, "Subscript error in Global node (subscript null/empty or too long)", size1 - 1);
         break;
      case CACHE_ERNOROUTINE:
         strncat(pcon->error, "Routine does not exist", size1 - 1);
         break;
      case CACHE_ERNOLINE:
         strncat(pcon->error, "Function does not exist in routine", size1 - 1);
         break;
      case CACHE_ERPARAMETER:
         strncat(pcon->error, "Function arguments error", size1 - 1);
         break;
      case CACHE_BAD_GLOBAL:
         strncat(pcon->error, "Invalid global name", size1 - 1);
         break;
      case CACHE_BAD_NAMESPACE:
         strncat(pcon->error, "Invalid NameSpace name", size1 - 1);
         break;
      case CACHE_BAD_FUNCTION:
         strncat(pcon->error, "Invalid function name", size1 - 1);
         break;
      case CACHE_BAD_CLASS:
         strncat(pcon->error, "Invalid class name", size1 - 1);
         break;
      case CACHE_BAD_METHOD:
         strncat(pcon->error, "Invalid method name", size1 - 1);
         break;
      case CACHE_ERNOCLASS:
         strncat(pcon->error, "Class does not exist", size1 - 1);
         break;
      case CACHE_ERBADOREF:
         strncat(pcon->error, "Invalid Object Reference", size1 - 1);
         break;
      case CACHE_ERNOMETHOD:
         strncat(pcon->error, "Method does not exist", size1 - 1);
         break;
      case CACHE_ERNOPROPERTY:
         strncat(pcon->error, "Property does not exist", size1 - 1);
         break;
      case CACHE_ETIMEOUT:
         strncat(pcon->error, "Operation timed out", size1 - 1);
         break;
      case CACHE_BAD_STRING:
         strncat(pcon->error, "Invalid string", size1 - 1);
         break;
      case CACHE_ERNAMSP:
         strncat(pcon->error, "Invalid Namespace", size1 - 1);
         break;
      default:
         strncat(pcon->error, "Database Server Error", size1 - 1);
         break;
   }
   pcon->error[size - 1] = '\0';

isc_error_message_exit:

   mgw_set_error_message(pcon);

   return 0;
}


int ydb_load_library(DBXCON *pcon)
{
   int n, len, result;
   char primlib[DBX_ERROR_SIZE], primerr[DBX_ERROR_SIZE];
   char verfile[256], fun[64];
   char *libnam[16];

   strcpy(pcon->p_ydb_so->libdir, pcon->shdir);
   strcpy(pcon->p_ydb_so->funprfx, "ydb");
   strcpy(pcon->p_ydb_so->dbname, "YottaDB");

   strcpy(verfile, pcon->shdir);
   len = (int) strlen(pcon->p_ydb_so->libdir);
   if (pcon->p_ydb_so->libdir[len - 1] != '/' && pcon->p_ydb_so->libdir[len - 1] != '\\') {
      pcon->p_ydb_so->libdir[len] = '/';
      len ++;
   }

   n = 0;
#if defined(_WIN32)
   libnam[n ++] = (char *) DBX_YDB_DLL;
#else
#if defined(MACOSX)
   libnam[n ++] = (char *) DBX_YDB_DYLIB;
   libnam[n ++] = (char *) DBX_YDB_SO;
#else
   libnam[n ++] = (char *) DBX_YDB_SO;
   libnam[n ++] = (char *) DBX_YDB_DYLIB;
#endif
#endif

   libnam[n ++] = NULL;
   strcpy(pcon->p_ydb_so->libnam, pcon->p_ydb_so->libdir);
   len = (int) strlen(pcon->p_ydb_so->libnam);

   for (n = 0; libnam[n]; n ++) {
      strcpy(pcon->p_ydb_so->libnam + len, libnam[n]);
      if (!n) {
         strcpy(primlib, pcon->p_ydb_so->libnam);
      }

      pcon->p_ydb_so->p_library = mgw_dso_load(pcon->p_ydb_so->libnam);
      if (pcon->p_debug->debug == 1) {
         fprintf(pcon->p_debug->p_fdebug, "\r\n       >>> %p==mgw_dso_load(%s)", pcon->p_ydb_so->p_library, pcon->p_ydb_so->libnam);
         fflush(pcon->p_debug->p_fdebug);
      }
      if (pcon->p_ydb_so->p_library) {
         break;
      }

      if (!n) {
         int len1, len2;
         char *p;
#if defined(_WIN32)
         DWORD errorcode;
         LPVOID lpMsgBuf;

         lpMsgBuf = NULL;
         errorcode = GetLastError();
         sprintf(pcon->error, "Error loading %s Library: %s; Error Code : %ld",  pcon->p_ydb_so->dbname, primlib, errorcode);
         len2 = (int) strlen(pcon->error);
         len1 = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                        NULL,
                        errorcode,
                        /* MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), */
                        MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
                        (LPTSTR) &lpMsgBuf,
                        0,
                        NULL 
                        );
         if (lpMsgBuf && len1 > 0 && (DBX_ERROR_SIZE - len2) > 30) {
            strncpy(primerr, (const char *) lpMsgBuf, DBX_ERROR_SIZE - 1);
            p = strstr(primerr, "\r\n");
            if (p)
               *p = '\0';
            len1 = (DBX_ERROR_SIZE - (len2 + 10));
            if (len1 < 1)
               len1 = 0;
            primerr[len1] = '\0';
            p = strstr(primerr, "%1");
            if (p) {
               *p = 'I';
               *(p + 1) = 't';
            }
            strcat(pcon->error, " (");
            strcat(pcon->error, primerr);
            strcat(pcon->error, ")");
         }
         if (lpMsgBuf)
            LocalFree(lpMsgBuf);
#else
         p = (char *) dlerror();
         sprintf(primerr, "Cannot load %s library: Error Code: %d", pcon->p_ydb_so->dbname, errno);
         len2 = strlen(pcon->error);
         if (p) {
            strncpy(primerr, p, DBX_ERROR_SIZE - 1);
            primerr[DBX_ERROR_SIZE - 1] = '\0';
            len1 = (DBX_ERROR_SIZE - (len2 + 10));
            if (len1 < 1)
               len1 = 0;
            primerr[len1] = '\0';
            strcat(pcon->error, " (");
            strcat(pcon->error, primerr);
            strcat(pcon->error, ")");
         }
#endif
      }
   }

   if (!pcon->p_ydb_so->p_library) {
      goto ydb_load_library_exit;
   }

   sprintf(fun, "%s_init", pcon->p_ydb_so->funprfx);
   pcon->p_ydb_so->p_ydb_init = (int (*) (void)) mgw_dso_sym(pcon->p_ydb_so->p_library, (char *) fun);
   if (!pcon->p_ydb_so->p_ydb_init) {
      sprintf(pcon->error, "Error loading %s library: %s; Cannot locate the following function : %s", pcon->p_ydb_so->dbname, pcon->p_ydb_so->libnam, fun);
      goto ydb_load_library_exit;
   }
   sprintf(fun, "%s_exit", pcon->p_ydb_so->funprfx);
   pcon->p_ydb_so->p_ydb_exit = (int (*) (void)) mgw_dso_sym(pcon->p_ydb_so->p_library, (char *) fun);
   if (!pcon->p_ydb_so->p_ydb_exit) {
      sprintf(pcon->error, "Error loading %s library: %s; Cannot locate the following function : %s", pcon->p_ydb_so->dbname, pcon->p_ydb_so->libnam, fun);
      goto ydb_load_library_exit;
   }
   sprintf(fun, "%s_malloc", pcon->p_ydb_so->funprfx);
   pcon->p_ydb_so->p_ydb_malloc = (int (*) (size_t)) mgw_dso_sym(pcon->p_ydb_so->p_library, (char *) fun);
   if (!pcon->p_ydb_so->p_ydb_malloc) {
      sprintf(pcon->error, "Error loading %s library: %s; Cannot locate the following function : %s", pcon->p_ydb_so->dbname, pcon->p_ydb_so->libnam, fun);
      goto ydb_load_library_exit;
   }
   sprintf(fun, "%s_free", pcon->p_ydb_so->funprfx);
   pcon->p_ydb_so->p_ydb_free = (int (*) (void *)) mgw_dso_sym(pcon->p_ydb_so->p_library, (char *) fun);
   if (!pcon->p_ydb_so->p_ydb_free) {
      sprintf(pcon->error, "Error loading %s library: %s; Cannot locate the following function : %s", pcon->p_ydb_so->dbname, pcon->p_ydb_so->libnam, fun);
      goto ydb_load_library_exit;
   }
   sprintf(fun, "%s_data_s", pcon->p_ydb_so->funprfx);
   pcon->p_ydb_so->p_ydb_data_s = (int (*) (ydb_buffer_t *, int, ydb_buffer_t *, unsigned int *)) mgw_dso_sym(pcon->p_ydb_so->p_library, (char *) fun);
   if (!pcon->p_ydb_so->p_ydb_data_s) {
      sprintf(pcon->error, "Error loading %s library: %s; Cannot locate the following function : %s", pcon->p_ydb_so->dbname, pcon->p_ydb_so->libnam, fun);
      goto ydb_load_library_exit;
   }
   sprintf(fun, "%s_delete_s", pcon->p_ydb_so->funprfx);
   pcon->p_ydb_so->p_ydb_delete_s = (int (*) (ydb_buffer_t *, int, ydb_buffer_t *, int)) mgw_dso_sym(pcon->p_ydb_so->p_library, (char *) fun);
   if (!pcon->p_ydb_so->p_ydb_delete_s) {
      sprintf(pcon->error, "Error loading %s library: %s; Cannot locate the following function : %s", pcon->p_ydb_so->dbname, pcon->p_ydb_so->libnam, fun);
      goto ydb_load_library_exit;
   }
   sprintf(fun, "%s_set_s", pcon->p_ydb_so->funprfx);
   pcon->p_ydb_so->p_ydb_set_s = (int (*) (ydb_buffer_t *, int, ydb_buffer_t *, ydb_buffer_t *)) mgw_dso_sym(pcon->p_ydb_so->p_library, (char *) fun);
   if (!pcon->p_ydb_so->p_ydb_set_s) {
      sprintf(pcon->error, "Error loading %s library: %s; Cannot locate the following function : %s", pcon->p_ydb_so->dbname, pcon->p_ydb_so->libnam, fun);
      goto ydb_load_library_exit;
   }
   sprintf(fun, "%s_get_s", pcon->p_ydb_so->funprfx);
   pcon->p_ydb_so->p_ydb_get_s = (int (*) (ydb_buffer_t *, int, ydb_buffer_t *, ydb_buffer_t *)) mgw_dso_sym(pcon->p_ydb_so->p_library, (char *) fun);
   if (!pcon->p_ydb_so->p_ydb_get_s) {
      sprintf(pcon->error, "Error loading %s library: %s; Cannot locate the following function : %s", pcon->p_ydb_so->dbname, pcon->p_ydb_so->libnam, fun);
      goto ydb_load_library_exit;
   }
   sprintf(fun, "%s_subscript_next_s", pcon->p_ydb_so->funprfx);
   pcon->p_ydb_so->p_ydb_subscript_next_s = (int (*) (ydb_buffer_t *, int, ydb_buffer_t *, ydb_buffer_t *)) mgw_dso_sym(pcon->p_ydb_so->p_library, (char *) fun);
   if (!pcon->p_ydb_so->p_ydb_subscript_next_s) {
      sprintf(pcon->error, "Error loading %s library: %s; Cannot locate the following function : %s", pcon->p_ydb_so->dbname, pcon->p_ydb_so->libnam, fun);
      goto ydb_load_library_exit;
   }
   sprintf(fun, "%s_subscript_previous_s", pcon->p_ydb_so->funprfx);
   pcon->p_ydb_so->p_ydb_subscript_previous_s = (int (*) (ydb_buffer_t *, int, ydb_buffer_t *, ydb_buffer_t *)) mgw_dso_sym(pcon->p_ydb_so->p_library, (char *) fun);
   if (!pcon->p_ydb_so->p_ydb_subscript_previous_s) {
      sprintf(pcon->error, "Error loading %s library: %s; Cannot locate the following function : %s", pcon->p_ydb_so->dbname, pcon->p_ydb_so->libnam, fun);
      goto ydb_load_library_exit;
   }
   sprintf(fun, "%s_node_next_s", pcon->p_ydb_so->funprfx);
   pcon->p_ydb_so->p_ydb_node_next_s = (int (*) (ydb_buffer_t *, int, ydb_buffer_t *, int *, ydb_buffer_t *)) mgw_dso_sym(pcon->p_ydb_so->p_library, (char *) fun);
   if (!pcon->p_ydb_so->p_ydb_node_next_s) {
      sprintf(pcon->error, "Error loading %s library: %s; Cannot locate the following function : %s", pcon->p_ydb_so->dbname, pcon->p_ydb_so->libnam, fun);
      goto ydb_load_library_exit;
   }
   sprintf(fun, "%s_node_previous_s", pcon->p_ydb_so->funprfx);
   pcon->p_ydb_so->p_ydb_node_previous_s = (int (*) (ydb_buffer_t *, int, ydb_buffer_t *, int *, ydb_buffer_t *)) mgw_dso_sym(pcon->p_ydb_so->p_library, (char *) fun);
   if (!pcon->p_ydb_so->p_ydb_node_previous_s) {
      sprintf(pcon->error, "Error loading %s library: %s; Cannot locate the following function : %s", pcon->p_ydb_so->dbname, pcon->p_ydb_so->libnam, fun);
      goto ydb_load_library_exit;
   }
   sprintf(fun, "%s_incr_s", pcon->p_ydb_so->funprfx);
   pcon->p_ydb_so->p_ydb_incr_s = (int (*) (ydb_buffer_t *, int, ydb_buffer_t *, ydb_buffer_t *, ydb_buffer_t *)) mgw_dso_sym(pcon->p_ydb_so->p_library, (char *) fun);
   if (!pcon->p_ydb_so->p_ydb_incr_s) {
      sprintf(pcon->error, "Error loading %s library: %s; Cannot locate the following function : %s", pcon->p_ydb_so->dbname, pcon->p_ydb_so->libnam, fun);
      goto ydb_load_library_exit;
   }
   sprintf(fun, "%s_ci", pcon->p_ydb_so->funprfx);
   pcon->p_ydb_so->p_ydb_ci = (int (*) (const char *, ...)) mgw_dso_sym(pcon->p_ydb_so->p_library, (char *) fun);
   if (!pcon->p_ydb_so->p_ydb_ci) {
      sprintf(pcon->error, "Error loading %s library: %s; Cannot locate the following function : %s", pcon->p_ydb_so->dbname, pcon->p_ydb_so->libnam, fun);
      goto ydb_load_library_exit;
   }
   sprintf(fun, "%s_cip", pcon->p_ydb_so->funprfx);
   pcon->p_ydb_so->p_ydb_cip = (int (*) (ci_name_descriptor *, ...)) mgw_dso_sym(pcon->p_ydb_so->p_library, (char *) fun);
   if (!pcon->p_ydb_so->p_ydb_cip) {
      sprintf(pcon->error, "Error loading %s library: %s; Cannot locate the following function : %s", pcon->p_ydb_so->dbname, pcon->p_ydb_so->libnam, fun);
      goto ydb_load_library_exit;
   }

   pcon->pid = mgw_current_process_id();

   pcon->p_ydb_so->loaded = 1;

ydb_load_library_exit:

   if (pcon->error[0]) {
      pcon->p_ydb_so->loaded = 0;
      pcon->error_code = 1009;
      strcpy((char *) pcon->output_val.svalue.buf_addr, "0");
      result = CACHE_NOCON;

      return result;
   }

   return CACHE_SUCCESS;
}


int ydb_open(DBXCON *pcon)
{
   int rc, error_code, result;
   char buffer[256], buffer1[256];
   ydb_buffer_t zv, data;

   error_code = 0;

   if (!pcon->p_ydb_so) {
      pcon->p_ydb_so = (DBXYDBSO *) mgw_malloc(sizeof(DBXYDBSO), 0);
      if (!pcon->p_ydb_so) {
         strcpy(pcon->error, "No Memory");
         pcon->error_code = 1009; 
         result = CACHE_NOCON;
         return result;
      }
      memset((void *) pcon->p_ydb_so, 0, sizeof(DBXYDBSO));
      pcon->p_ydb_so->loaded = 0;
   }

   if (pcon->p_ydb_so->loaded == 2) {
      strcpy(pcon->error, "Cannot create multiple connections to the database");
      pcon->error_code = 1009; 
      strncpy(pcon->error, pcon->error, DBX_ERROR_SIZE - 1);
      pcon->error[DBX_ERROR_SIZE - 1] = '\0';
      strcpy((char *) pcon->output_val.svalue.buf_addr, "0");
      rc = CACHE_NOCON;
      goto ydb_open_exit;
   }

   if (!pcon->p_ydb_so->loaded) {
      rc = ydb_load_library(pcon);
      if (rc != CACHE_SUCCESS) {
         goto ydb_open_exit;
      }
   }

   rc = pcon->p_ydb_so->p_ydb_init();

   strcpy(buffer, "$zv");
   zv.buf_addr = buffer;
   zv.len_used = (int) strlen(buffer);
   zv.len_alloc = 255;

   data.buf_addr = buffer1;
   data.len_used = 0;
   data.len_alloc = 255;

   rc = pcon->p_ydb_so->p_ydb_get_s(&zv, 0, NULL, &data);

   if (data.len_used >= 0) {
      data.buf_addr[data.len_used] = '\0';
   }

   if (pcon->p_debug->debug == 1) {
      fprintf(pcon->p_debug->p_fdebug, "\r\n");
      fflush(pcon->p_debug->p_fdebug);
   }

   if (rc == CACHE_SUCCESS) {
      ydb_parse_zv(data.buf_addr, &(pcon->zv));
      sprintf(pcon->p_zv->version, "%d.%d.b%d", pcon->p_zv->majorversion, pcon->p_zv->minorversion, pcon->p_zv->mgw_build);
   }

ydb_open_exit:

   if (rc == CACHE_SUCCESS) {
      mgw_create_string(pcon, (void *) &rc, DBX_DTYPE_INT);
   }
   else {
      mgw_error_message(pcon, rc);
   }

   return 0;
}


int ydb_parse_zv(char *zv, DBXZV * p_ydb_sv)
{
   int result;
   double mgw_version;
   char *p, *p1, *p2;

   p_ydb_sv->mgw_version = 0;
   p_ydb_sv->majorversion = 0;
   p_ydb_sv->minorversion = 0;
   p_ydb_sv->mgw_build = 0;
   p_ydb_sv->vnumber = 0;

   result = 0;
   /* GT.M V6.3-004 Linux x86_64 */

   p_ydb_sv->product = DBX_DBTYPE_YOTTADB;

   p = zv;
   mgw_version = 0;
   while (*(++ p)) {
      if (*(p - 1) == 'V' && isdigit((int) (*p))) {
         mgw_version = strtod(p, NULL);
         break;
      }
   }

   if (mgw_version > 0) {
      p_ydb_sv->mgw_version = mgw_version;
      p_ydb_sv->majorversion = (int) strtol(p, NULL, 10);
      p1 = strstr(p, ".");
      if (p1) {
         p_ydb_sv->minorversion = (int) strtol(p1 + 1, NULL, 10);
      }
      p2 = strstr(p, "-");
      if (p2) {
         p_ydb_sv->mgw_build = (int) strtol(p2 + 1, NULL, 10);
      }

      p_ydb_sv->vnumber = ((p_ydb_sv->majorversion * 100000) + (p_ydb_sv->minorversion * 10000) + p_ydb_sv->mgw_build);

      result = 1;
   }
/*
   printf("\r\n ydb_parse_zv : p_ydb_sv->majorversion=%d; p_ydb_sv->minorversion=%d; p_ydb_sv->mgw_build=%d; p_ydb_sv->mgw_version=%f;", p_ydb_sv->majorversion, p_ydb_sv->minorversion, p_ydb_sv->mgw_build, p_ydb_sv->mgw_version);
*/
   return result;
}


int ydb_error_message(DBXCON *pcon, int error_code)
{
   int rc;
   char buffer[256], buffer1[256];
   ydb_buffer_t zstatus, data;

   strcpy(buffer, "$zstatus");
   zstatus.buf_addr = buffer;
   zstatus.len_used = (int) strlen(buffer);
   zstatus.len_alloc = 255;

   data.buf_addr = buffer1;
   data.len_used = 0;
   data.len_alloc = 255;

   rc = pcon->p_ydb_so->p_ydb_get_s(&zstatus, 0, NULL, &data);

   if (data.len_used >= 0) {
      data.buf_addr[data.len_used] = '\0';
   }

   strcpy(pcon->error, data.buf_addr);

   mgw_set_error_message(pcon);

   return rc;
}


int ydb_function(DBXCON *pcon, DBXFUN *pfun)
{
   int rc;

   pcon->output_val.svalue.len_used = 0;
   pcon->output_val.svalue.buf_addr[0] = '\0';

   switch (pcon->argc) {
      case 1:
         rc = pcon->p_ydb_so->p_ydb_ci(pfun->label, pcon->output_val.svalue.buf_addr);
         break;
      case 2:
         rc = pcon->p_ydb_so->p_ydb_ci(pfun->label, pcon->output_val.svalue.buf_addr, pcon->args[1].svalue.buf_addr);
         break;
      case 3:
         rc = pcon->p_ydb_so->p_ydb_ci(pfun->label, pcon->output_val.svalue.buf_addr, pcon->args[1].svalue.buf_addr, pcon->args[2].svalue.buf_addr);
         break;
      case 4:
         rc = pcon->p_ydb_so->p_ydb_ci(pfun->label, pcon->output_val.svalue.buf_addr, pcon->args[1].svalue.buf_addr, pcon->args[2].svalue.buf_addr, pcon->args[3].svalue.buf_addr);
         break;
      default:
         rc = CACHE_SUCCESS;
         break;
   }

   pcon->output_val.svalue.len_used = (int) strlen(pcon->output_val.svalue.buf_addr);

   return rc;
}


DBXCON * mgw_unpack_header(unsigned char *input, unsigned char *output)
{
   int len, output_bsize, offset, index;
   DBXCON *pcon;

   offset = 0;

   len = (int) mgw_get_size(input + offset);
   offset += 5;

   output_bsize = (int) mgw_get_size(input + offset);
   offset += 5;

   index = (int) mgw_get_size(input + offset);
   offset += 5;

/*
   printf("\r\n mgw_unpack_header : input=%p; output=%p; len=%d; output_bsize=%d; index=%d;", input, output, len, output_bsize, index);
*/
   if (index >= 0 && index < DBX_MAXCONS) {
      pcon = connection[index];
   }
   if (!pcon) {
      return NULL;
   }

   pcon->argc = 0;
   pcon->input_str.buf_addr = (char *) input;
   pcon->input_str.len_used = len;

   if (output) {
      pcon->output_val.svalue.buf_addr = (char *) output;
   }
   else {
      pcon->output_val.svalue.buf_addr = (char *) input;
   }

   pcon->output_val.svalue.len_alloc = output_bsize;
   memset((void *) pcon->output_val.svalue.buf_addr, 0, 5);
   pcon->output_val.offset = 5;
   pcon->output_val.svalue.len_used = 5;

   pcon->offset = offset;

   return pcon;
}


int mgw_unpack_arguments(DBXCON *pcon)
{
   int len, dsort, dtype;

   for (;;) {
      len = (int) mgw_get_block_size(&(pcon->input_str), pcon->offset, &dsort, &dtype);
      pcon->offset += 5;
/*
      printf("\r\nn=%d; len=%d; offset=%d; sort=%d; type=%d; str=%s;", pcon->argc, len, pcon->offset, dsort, dtype, pcon->input_str.str + offset);
*/
      if (dsort == DBX_DSORT_EOD) {
         break;
      }

      pcon->args[pcon->argc].type = dtype;
      pcon->args[pcon->argc].svalue.len_used = len;
      pcon->args[pcon->argc].svalue.buf_addr = (char *) (pcon->input_str.buf_addr + pcon->offset);
      pcon->offset += len;
      pcon->argc ++;

      if (pcon->argc > (DBX_MAXARGS - 1))
         break;
   }

   return pcon->argc;
}


int mgw_global_reference(DBXCON *pcon)
{
   int n, rc, len, dsort, dtype;
   unsigned int ne;
   char *p;

   rc = CACHE_SUCCESS;
   for (;;) {
      len = (int) mgw_get_block_size(&(pcon->input_str), pcon->offset, &dsort, &dtype);
      pcon->offset += 5;
/*
      printf("\r\nn=%d; len=%d; offset=%d; sort=%d; type=%d; str=%s;", pcon->argc, len, pcon->offset, dsort, dtype, pcon->input_str.buf_addr + pcon->offset);
*/
      if (dsort == DBX_DSORT_EOD) {
         break;
      }
      p = (char *) (pcon->input_str.buf_addr + pcon->offset);

      pcon->args[pcon->argc].type = dtype;
      pcon->args[pcon->argc].svalue.len_used = len;
      pcon->args[pcon->argc].svalue.len_alloc = len;
      pcon->args[pcon->argc].svalue.buf_addr = (char *) (pcon->input_str.buf_addr + pcon->offset);
      pcon->offset += len;
      n = pcon->argc;
      pcon->argc ++;
      if (pcon->argc > (DBX_MAXARGS - 1)) {
         break;
      }
      if (pcon->dbtype == DBX_DBTYPE_YOTTADB) {
         if (n > 0) {
            pcon->yargs[n - 1].len_used = pcon->args[n].svalue.len_used;
            pcon->yargs[n - 1].len_alloc = pcon->args[n].svalue.len_alloc;
            pcon->yargs[n - 1].buf_addr = (char *) pcon->args[n].svalue.buf_addr;
         }
         continue;
      }

      if (n == 0) {
         if (pcon->args[n].svalue.buf_addr[0] == '^')
            rc = pcon->p_isc_so->p_CachePushGlobal((int) pcon->args[n].svalue.len_used - 1, (Callin_char_t *) pcon->args[n].svalue.buf_addr + 1);
         else
            rc = pcon->p_isc_so->p_CachePushGlobal((int) pcon->args[n].svalue.len_used, (Callin_char_t *) pcon->args[n].svalue.buf_addr);
      }
      else {

         if (pcon->args[n].type == DBX_DTYPE_INT) {
            rc = pcon->p_isc_so->p_CachePushInt(pcon->args[n].num.int32);
         }
         else if (pcon->args[n].type == DBX_DTYPE_DOUBLE) {
            rc = pcon->p_isc_so->p_CachePushDbl(pcon->args[n].num.real);
         }
         else {
            if (pcon->args[n].svalue.len_used < DBX_MAXSIZE) {
               rc = pcon->p_isc_so->p_CachePushStr(pcon->args[n].svalue.len_used, (Callin_char_t *) pcon->args[n].svalue.buf_addr);
            }
            else {
               pcon->args[n].cvalue.pstr = (void *) pcon->p_isc_so->p_CacheExStrNew((CACHE_EXSTRP) &(pcon->args[n].cvalue.zstr), pcon->args[n].svalue.len_used + 1);
               for (ne = 0; ne < pcon->args[n].svalue.len_used; ne ++) {
                  pcon->args[n].cvalue.zstr.str.ch[ne] = (char) pcon->args[n].svalue.buf_addr[ne];
               }
               pcon->args[n].cvalue.zstr.str.ch[ne] = (char) 0;
               pcon->args[n].cvalue.zstr.len = pcon->args[n].svalue.len_used;

               rc = pcon->p_isc_so->p_CachePushExStr((CACHE_EXSTRP) &(pcon->args[n].cvalue.zstr));
            }
         }
      }
      if (rc != CACHE_SUCCESS) {
         break;
      }
   }

   return rc;
}


int mgw_class_reference(DBXCON *pcon, short context)
{
   int n, rc, len, dsort, dtype, flags;
   unsigned int ne;
   char *p;

   rc = CACHE_SUCCESS;
   for (;;) {
      len = (int) mgw_get_block_size(&(pcon->input_str), pcon->offset, &dsort, &dtype);
      pcon->offset += 5;
/*
      printf("\r\nn=%d; len=%d; offset=%d; sort=%d; type=%d; str=%s;", pcon->argc, len, pcon->offset, dsort, dtype, pcon->input_str.buf_addr + pcon->offset);
*/
      if (dsort == DBX_DSORT_EOD) {
         break;
      }
      p = (char *) (pcon->input_str.buf_addr + pcon->offset);

      pcon->args[pcon->argc].type = dtype;
      pcon->args[pcon->argc].svalue.len_used = len;
      pcon->args[pcon->argc].svalue.len_alloc = len;
      pcon->args[pcon->argc].svalue.buf_addr = (char *) (pcon->input_str.buf_addr + pcon->offset);
      pcon->offset += len;
      n = pcon->argc;
      pcon->argc ++;
      if (pcon->argc > (DBX_MAXARGS - 1)) {
         break;
      }
      if (pcon->dbtype == DBX_DBTYPE_YOTTADB) {
         if (n > 0) {
            pcon->yargs[n - 1].len_used = pcon->args[n].svalue.len_used;
            pcon->yargs[n - 1].len_alloc = pcon->args[n].svalue.len_alloc;
            pcon->yargs[n - 1].buf_addr = (char *) pcon->args[n].svalue.buf_addr;
         }
         continue;
      }

      if (context == 0) { /* classmethod */
         if (n == 0) {
            continue;
         }
         else if (n == 1) {
            flags = 1;
            rc = pcon->p_isc_so->p_CachePushClassMethod(pcon->args[0].svalue.len_used, (const Callin_char_t *) pcon->args[0].svalue.buf_addr, pcon->args[1].svalue.len_used, (const Callin_char_t *) pcon->args[1].svalue.buf_addr, flags);
            continue;
         }
      }
      else if (context == 1) { /* method */
         if (n == 0) {
            continue;
         }
         else if (n == 1) {
            flags = 1;
            rc = pcon->p_isc_so->p_CachePushMethod((int) strtol(pcon->args[0].svalue.buf_addr, NULL, 10), pcon->args[1].svalue.len_used, (const Callin_char_t *) pcon->args[1].svalue.buf_addr, flags);
            continue;
         }
      }
      else if (context == 2) { /* property */
         if (n == 0) {
            continue;
         }
         else if (n == 1) {
            flags = 1;
            rc = pcon->p_isc_so->p_CachePushProperty((int) strtol(pcon->args[0].svalue.buf_addr, NULL, 10), pcon->args[1].svalue.len_used, (const Callin_char_t *) pcon->args[1].svalue.buf_addr);
            continue;
         }
      }
      else if (context == 2) { /* close instance */
         if (n == 0) {
            rc = pcon->p_isc_so->p_CacheCloseOref((int) strtol(pcon->args[0].svalue.buf_addr, NULL, 10));
            break;
         }
      }

      if (pcon->args[n].type == DBX_DTYPE_INT) {
         rc = pcon->p_isc_so->p_CachePushInt(pcon->args[n].num.int32);
      }
      else if (pcon->args[n].type == DBX_DTYPE_DOUBLE) {
         rc = pcon->p_isc_so->p_CachePushDbl(pcon->args[n].num.real);
      }
      else {
         if (pcon->args[n].svalue.len_used < DBX_MAXSIZE) {
            rc = pcon->p_isc_so->p_CachePushStr(pcon->args[n].svalue.len_used, (Callin_char_t *) pcon->args[n].svalue.buf_addr);
         }
         else {
            pcon->args[n].cvalue.pstr = (void *) pcon->p_isc_so->p_CacheExStrNew((CACHE_EXSTRP) &(pcon->args[n].cvalue.zstr), pcon->args[n].svalue.len_used + 1);
            for (ne = 0; ne < pcon->args[n].svalue.len_used; ne ++) {
               pcon->args[n].cvalue.zstr.str.ch[ne] = (char) pcon->args[n].svalue.buf_addr[ne];
            }
            pcon->args[n].cvalue.zstr.str.ch[ne] = (char) 0;
            pcon->args[n].cvalue.zstr.len = pcon->args[n].svalue.len_used;

            rc = pcon->p_isc_so->p_CachePushExStr((CACHE_EXSTRP) &(pcon->args[n].cvalue.zstr));
         }
      }
      if (rc != CACHE_SUCCESS) {
         break;
      }
   }

   return rc;
}


int mgw_function_reference(DBXCON *pcon, DBXFUN *pfun)
{
   int n, rc, len, dsort, dtype;
   unsigned int ne;
   char *p;

   rc = CACHE_SUCCESS;
   for (;;) {
      len = (int) mgw_get_block_size(&(pcon->input_str), pcon->offset, &dsort, &dtype);
      pcon->offset += 5;
/*
      printf("\r\nn=%d; len=%d; offset=%d; sort=%d; type=%d; str=%s;", pcon->argc, len, pcon->offset, dsort, dtype, pcon->input_str.buf_addr + pcon->offset);
*/
      if (dsort == DBX_DSORT_EOD) {
         break;
      }
      p = (char *) (pcon->input_str.buf_addr + pcon->offset);

      pcon->args[pcon->argc].type = dtype;
      pcon->args[pcon->argc].svalue.len_used = len;
      pcon->args[pcon->argc].svalue.len_alloc = len;
      pcon->args[pcon->argc].svalue.buf_addr = (char *) (pcon->input_str.buf_addr + pcon->offset);
      pcon->offset += len;
      n = pcon->argc;
      pcon->argc ++;
      if (pcon->argc > (DBX_MAXARGS - 1)) {
         break;
      }
      if (pcon->dbtype == DBX_DBTYPE_YOTTADB) {
         if (n > 0) {
            pcon->yargs[n - 1].len_used = pcon->args[n].svalue.len_used;
            pcon->yargs[n - 1].len_alloc = pcon->args[n].svalue.len_alloc;
            pcon->yargs[n - 1].buf_addr = (char *) pcon->args[n].svalue.buf_addr;
         }
      }

      if (n == 0) {
         strncpy(pfun->buffer, pcon->args[n].svalue.buf_addr, pcon->args[n].svalue.len_used);
         pfun->buffer[pcon->args[n].svalue.len_used] = '\0';
         pfun->label = pfun->buffer;
         pfun->routine = strstr(pfun->buffer, "^");
         *pfun->routine = '\0';
         pfun->routine ++;
         pfun->label_len = (int) strlen(pfun->label);
         pfun->routine_len = (int) strlen(pfun->routine);

         if (pcon->dbtype != DBX_DBTYPE_YOTTADB) {
            rc = pcon->p_isc_so->p_CachePushFunc(&(pfun->rflag), (int) pfun->label_len, (const Callin_char_t *) pfun->label, (int) pfun->routine_len, (const Callin_char_t *) pfun->routine);
         }

      }
      else {

         if (pcon->dbtype == DBX_DBTYPE_YOTTADB) {
            continue;
         }

         if (pcon->args[n].type == DBX_DTYPE_INT) {
            rc = pcon->p_isc_so->p_CachePushInt(pcon->args[n].num.int32);
         }
         else if (pcon->args[n].type == DBX_DTYPE_DOUBLE) {
            rc = pcon->p_isc_so->p_CachePushDbl(pcon->args[n].num.real);
         }
         else {
            if (pcon->args[n].svalue.len_used < DBX_MAXSIZE) {
               rc = pcon->p_isc_so->p_CachePushStr(pcon->args[n].svalue.len_used, (Callin_char_t *) pcon->args[n].svalue.buf_addr);
            }
            else {
               pcon->args[n].cvalue.pstr = (void *) pcon->p_isc_so->p_CacheExStrNew((CACHE_EXSTRP) &(pcon->args[n].cvalue.zstr), pcon->args[n].svalue.len_used + 1);
               for (ne = 0; ne < pcon->args[n].svalue.len_used; ne ++) {
                  pcon->args[n].cvalue.zstr.str.ch[ne] = (char) pcon->args[n].svalue.buf_addr[ne];
               }
               pcon->args[n].cvalue.zstr.str.ch[ne] = (char) 0;
               pcon->args[n].cvalue.zstr.len = pcon->args[n].svalue.len_used;

               rc = pcon->p_isc_so->p_CachePushExStr((CACHE_EXSTRP) &(pcon->args[n].cvalue.zstr));
            }
         }
      }
      if (rc != CACHE_SUCCESS) {
         break;
      }
   }

   return rc;
}


int mgw_add_block_size(DBXSTR *block, unsigned long offset, unsigned long data_len, int dsort, int dtype)
{
   block->buf_addr[offset + 0] = (unsigned char) (data_len >> 0);
   block->buf_addr[offset + 1] = (unsigned char) (data_len >> 8);
   block->buf_addr[offset + 2] = (unsigned char) (data_len >> 16);
   block->buf_addr[offset + 3] = (unsigned char) (data_len >> 24);
   block->buf_addr[offset + 4] = (unsigned char) ((dsort * 20) + dtype);

   return 1;
}


unsigned long mgw_get_block_size(DBXSTR *block, unsigned long offset, int *dsort, int *dtype)
{
   unsigned long data_len;
   unsigned char uc;

   data_len = 0;
   uc = (unsigned char) block->buf_addr[offset + 4];
   *dtype = uc % 20;
   *dsort = uc / 20;
   if (*dsort != DBX_DSORT_STATUS) {
      data_len = mgw_get_size((char *) block->buf_addr + offset);
   }

   /* printf("\r\n mgw_get_block_size %x:%x:%x:%x dlen=%lu; offset=%lu; type=%d (%x);\r\n", block->str[offset + 0], block->str[offset + 1], block->str[offset + 2], block->str[offset + 3], data_len, offset, *type, block->str[offset + 4]); */

   if (!DBX_DSORT_ISVALID(*dsort)) {
      *dsort = DBX_DSORT_INVALID;
   }

   return data_len;
}


unsigned long mgw_get_size(unsigned char *str)
{
   unsigned long size;

   size = ((unsigned char) str[0]) | (((unsigned char) str[1]) << 8) | (((unsigned char) str[2]) << 16) | (((unsigned char) str[3]) << 24);
   return size;
}



void * mgw_realloc(void *p, int curr_size, int new_size, short id)
{
   if (new_size >= curr_size) {
      if (p) {
         mgw_free((void *) p, 0);
      }
      /* printf("\r\n curr_size=%d; new_size=%d;\r\n", curr_size, new_size); */
      p = (void *) mgw_malloc(new_size, id);
      if (!p) {
         return NULL;
      }
   }
   return p;
}


void * mgw_malloc(int size, short id)
{
   void *p;

   p = (void *) malloc(size);

   /* printf("\nmgw_malloc: size=%d; id=%d; p=%p;", size, id, p); */

   return p;
}


int mgw_free(void *p, short id)
{
   /* printf("\nmgw_free: id=%d; p=%p;", id, p); */

   free((void *) p);

   return 0;
}


int mgw_lcase(char *string)
{
#ifdef _UNICODE

   CharLowerA(string);
   return 1;

#else

   int n, chr;

   n = 0;
   while (string[n] != '\0') {
      chr = (int) string[n];
      if (chr >= 65 && chr <= 90)
         string[n] = (char) (chr + 32);
      n ++;
   }
   return 1;

#endif
}


int mgw_create_string(DBXCON *pcon, void *data, short type)
{
   int len;
   DBXSTR *pstrobj_in;

   if (!data) {
      return -1;
   }

   len = 0;

   if (type == DBX_DTYPE_DBXSTR) {
      pstrobj_in = (DBXSTR *) data;
      type = DBX_DTYPE_STR;
      data = (void *) pstrobj_in->buf_addr;
   }

   if (type == DBX_DTYPE_STR) { 
      strcpy((char *) pcon->output_val.svalue.buf_addr + pcon->output_val.svalue.len_used, (char *) data);
      len = (int) strlen ((char *) data);
   }
   else if (type == DBX_DTYPE_INT) {
      sprintf((char *) pcon->output_val.svalue.buf_addr + pcon->output_val.svalue.len_used, "%d", (int) *((int *) data));
   }
   else {
      pcon->output_val.svalue.buf_addr[pcon->output_val.svalue.len_used] = '\0';
   }

   len = (int) strlen((char *) pcon->output_val.svalue.buf_addr + pcon->output_val.svalue.len_used);

   pcon->output_val.svalue.len_used += len;
   mgw_add_block_size(&(pcon->output_val.svalue), 0, (unsigned long) len, DBX_DSORT_DATA, DBX_DTYPE_DBXSTR);

   return (int) len;
}


int mgw_buffer_dump(DBXCON *pcon, void *buffer, unsigned int len, char *title, short mode)
{
   unsigned int n;
   unsigned char *p8;
   unsigned short c;

   p8 = NULL;

   p8 = (unsigned char *) buffer;

   if (pcon && pcon->p_debug->debug == 1) {
      fprintf(pcon->p_debug->p_fdebug, "%s (size=%d)\r\n", title, len);
      fflush(pcon->p_debug->p_fdebug);
   }
   else {
      printf("\nbuffer dump (title=%s; size=%d; mode=%d)...\n", title, len, mode);
   }

   for (n = 0; n < len; n ++) {
      c = p8[n];

      if (mode == 1) {
         if (pcon && pcon->p_debug->debug == 1)
            fprintf(pcon->p_debug->p_fdebug, "\\x%04x ", c);
         else
            printf("\\x%04x ", c);

         if (!((n + 1) % 8)) {
            if (pcon && pcon->p_debug->debug == 1)
               fprintf(pcon->p_debug->p_fdebug, "\r\n");
            else
               printf("\r\n");
         }
      }
      else {
         if ((c < 32) || (c > 126)) {
            if (pcon && pcon->p_debug->debug == 1)
               fprintf(pcon->p_debug->p_fdebug, "\\x%02x", c);
            else
               printf("\\x%02x", c);
         }
         else {
            if (pcon && pcon->p_debug->debug == 1)
               fprintf(pcon->p_debug->p_fdebug, "%c", (char) c);
            else
               printf("%c", (char) c);
         }
      }
   }

   if (pcon && pcon->p_debug->debug == 1) {
      fflush(pcon->p_debug->p_fdebug);
   }

   return 0;
}


int mgw_log_event(DBXDEBUG *p_debug, char *message, char *title, int level)
{
   if (p_debug && p_debug->debug == 1) {
      fprintf(p_debug->p_fdebug, "\r\n   >>> event: %s", title);
      fprintf(p_debug->p_fdebug, "\r\n       >>> %s", message);
      fflush(p_debug->p_fdebug);
   }
   return 1;
}


int mgw_pause(int msecs)
{
#if defined(_WIN32)

   Sleep((DWORD) msecs);

#else

#if 1
   unsigned int secs, msecs_rem;

   secs = (unsigned int) (msecs / 1000);
   msecs_rem = (unsigned int) (msecs % 1000);

   /* printf("\n   ===> msecs=%ld; secs=%ld; msecs_rem=%ld", msecs, secs, msecs_rem); */

   if (secs > 0) {
      sleep(secs);
   }
   if (msecs_rem > 0) {
      usleep((useconds_t) (msecs_rem * 1000));
   }

#else
   unsigned int secs;

   secs = (unsigned int) (msecs / 1000);
   if (secs == 0)
      secs = 1;
   sleep(secs);

#endif

#endif

   return 0;
}


DBXPLIB mgw_dso_load(char * library)
{
   DBXPLIB p_library;

#if defined(_WIN32)
   p_library = LoadLibraryA(library);
#else
   p_library = dlopen(library, RTLD_NOW);
#endif

   return p_library;
}


DBXPROC mgw_dso_sym(DBXPLIB p_library, char * symbol)
{
   DBXPROC p_proc;

#if defined(_WIN32)
   p_proc = GetProcAddress(p_library, symbol);
#else
   p_proc  = (void *) dlsym(p_library, symbol);
#endif

   return p_proc;
}



int mgw_dso_unload(DBXPLIB p_library)
{

#if defined(_WIN32)
   FreeLibrary(p_library);
#else
   dlclose(p_library); 
#endif

   return 1;
}


DBXTHID mgw_current_thread_id(void)
{
#if defined(_WIN32)
   return (DBXTHID) GetCurrentThreadId();
#else
   return (DBXTHID) pthread_self();
#endif
}


unsigned long mgw_current_process_id(void)
{
#if defined(_WIN32)
   return (unsigned long) GetCurrentProcessId();
#else
   return ((unsigned long) getpid());
#endif
}


int mgw_error_message(DBXCON *pcon, int error_code)
{
   int rc;

   if (pcon->dbtype == DBX_DBTYPE_YOTTADB) {
      rc = ydb_error_message(pcon, error_code);
   }
   else {
      rc = isc_error_message(pcon, error_code);
   }

   return rc;
}


int mgw_set_error_message(DBXCON *pcon)
{
   int len;

   len = (int) strlen(pcon->error);
   strcpy((char *) pcon->output_val.svalue.buf_addr + pcon->output_val.svalue.len_used, pcon->error);
   pcon->output_val.svalue.len_used += len;
   mgw_add_block_size(&(pcon->output_val.svalue), 0, (unsigned long) len, DBX_DSORT_ERROR, DBX_DTYPE_DBXSTR);

   return 0;
}


int mgw_cleanup(DBXCON *pcon)
{
   int n, rc;

   if (pcon->dbtype == DBX_DBTYPE_YOTTADB) {
      return 0;
   }

   for (n = 0; n < DBX_MAXARGS; n ++) {
      if (pcon->args[n].cvalue.pstr) {
         /* printf("\r\nmgw_cleanup %d &zstr=%p; pstr=%p;", n, &(pcon->cargs[n].zstr), pcon->cargs[n].pstr); */
         rc = pcon->p_isc_so->p_CacheExStrKill(&(pcon->args[n].cvalue.zstr));

         if (pcon->p_debug->debug == 1) {
            fprintf(pcon->p_debug->p_fdebug, "\r\n       >>> %d==CacheExStrKill(%p)", rc, &(pcon->args[n].cvalue.zstr));
            fflush(pcon->p_debug->p_fdebug);
         }

         pcon->args[n].cvalue.pstr = NULL;
      }
   }
   return 1;
}


int mgw_mutex_create(DBXMUTEX *p_mutex)
{
   int result;

   result = 0;
   if (p_mutex->created) {
      return result;
   }

#if defined(_WIN32)
   p_mutex->h_mutex = CreateMutex(NULL, FALSE, NULL);
   result = 0;
#else
   result = pthread_mutex_init(&(p_mutex->h_mutex), NULL);
#endif

   p_mutex->created = 1;
   p_mutex->stack = 0;
   p_mutex->thid = 0;

   return result;
}



int mgw_mutex_lock(DBXMUTEX *p_mutex, int timeout)
{
   int result;
   DBXTHID tid;
#ifdef _WIN32
   DWORD result_wait;
#endif

   result = 0;

   if (!p_mutex->created) {
      return -1;
   }

   tid = mgw_current_thread_id();
   if (p_mutex->thid == tid) {
      p_mutex->stack ++;
      /* printf("\r\n thread already owns lock : thid=%lu; stack=%d;\r\n", (unsigned long) tid, p_mutex->stack); */
      return 0; /* success - thread already owns lock */
   }

#if defined(_WIN32)
   if (timeout == 0) {
      result_wait = WaitForSingleObject(p_mutex->h_mutex, INFINITE);
   }
   else {
      result_wait = WaitForSingleObject(p_mutex->h_mutex, (timeout * 1000));
   }

   if (result_wait == WAIT_OBJECT_0) { /* success */
      result = 0;
   }
   else if (result_wait == WAIT_ABANDONED) {
      printf("\r\nmgw_mutex_lock: Returned WAIT_ABANDONED state");
      result = -1;
   }
   else if (result_wait == WAIT_TIMEOUT) {
      printf("\r\nmgw_mutex_lock: Returned WAIT_TIMEOUT state");
      result = -1;
   }
   else if (result_wait == WAIT_FAILED) {
      printf("\r\nmgw_mutex_lock: Returned WAIT_FAILED state: Error Code: %d", GetLastError());
      result = -1;
   }
   else {
      printf("\r\nmgw_mutex_lock: Returned Unrecognized state: %d", result_wait);
      result = -1;
   }
#else
   result = pthread_mutex_lock(&(p_mutex->h_mutex));
#endif

   p_mutex->thid = tid;
   p_mutex->stack = 0;

   return result;
}


int mgw_mutex_unlock(DBXMUTEX *p_mutex)
{
   int result;
   DBXTHID tid;

   result = 0;

   if (!p_mutex->created) {
      return -1;
   }

   tid = mgw_current_thread_id();
   if (p_mutex->thid == tid && p_mutex->stack) {
      /* printf("\r\n thread has stacked locks : thid=%lu; stack=%d;\r\n", (unsigned long) tid, p_mutex->stack); */
      p_mutex->stack --;
      return 0;
   }
   p_mutex->thid = 0;
   p_mutex->stack = 0;

#if defined(_WIN32)
   ReleaseMutex(p_mutex->h_mutex);
   result = 0;
#else
   result = pthread_mutex_unlock(&(p_mutex->h_mutex));
#endif /* #if defined(_WIN32) */

   return result;
}


int mgw_mutex_destroy(DBXMUTEX *p_mutex)
{
   int result;

   if (!p_mutex->created) {
      return -1;
   }

#if defined(_WIN32)
   CloseHandle(p_mutex->h_mutex);
   result = 0;
#else
   result = pthread_mutex_destroy(&(p_mutex->h_mutex));
#endif

   p_mutex->created = 0;

   return result;
}

int mgw_sleep(unsigned long msecs)
{
#if defined(_WIN32)

   Sleep((DWORD) msecs);

#else

#if 1
   unsigned int secs, msecs_rem;

   secs = (unsigned int) (msecs / 1000);
   msecs_rem = (unsigned int) (msecs % 1000);

   /* printf("\n   ===> msecs=%ld; secs=%ld; msecs_rem=%ld", msecs, secs, msecs_rem); */

   if (secs > 0) {
      sleep(secs);
   }
   if (msecs_rem > 0) {
      usleep((useconds_t) (msecs_rem * 1000));
   }

#else
   unsigned int secs;

   secs = (unsigned int) (msecs / 1000);
   if (secs == 0)
      secs = 1;
   sleep(secs);

#endif

#endif

   return 0;
}

