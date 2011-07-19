/*
**  Copyright 2003-2005  The Apache Software Foundation
**
**  Licensed under the Apache License, Version 2.0 (the "License");
**  you may not use this file except in compliance with the License.
**  You may obtain a copy of the License at
**
**      http://www.apache.org/licenses/LICENSE-2.0
**
**  Unless required by applicable law or agreed to in writing, software
**  distributed under the License is distributed on an "AS IS" BASIS,
**  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
**  See the License for the specific language governing permissions and
**  limitations under the License.
*/

#ifndef APREQ_H
#define APREQ_H

#ifdef APREQ_DEBUG
#include <assert.h>
#endif

#include "apr_tables.h"
#include <stddef.h>

#ifdef  __cplusplus
 extern "C" {
#endif 

/**
 * @file apreq.h
 * @brief Main header file...
 * @ingroup libapreq2
 *
 * Define the generic APREQ_ macros and common data structures.
 */

#ifndef WIN32
#define APREQ_DECLARE(d)                APR_DECLARE(d)
#define APREQ_DECLARE_NONSTD(d)         APR_DECLARE_NONSTD(d)
#define APREQ_DECLARE_DATA
#else
#define APREQ_DECLARE(type)             __declspec(dllexport) type __stdcall
#define APREQ_DECLARE_NONSTD(type)      __declspec(dllexport) type
#define APREQ_DECLARE_DATA              __declspec(dllexport)
#endif

#define APREQ_DEFAULT_READ_BLOCK_SIZE   (64  * 1024)
#define APREQ_DEFAULT_READ_LIMIT        (64 * 1024 * 1024)
#define APREQ_DEFAULT_BRIGADE_LIMIT     (256 * 1024)
#define APREQ_DEFAULT_NELTS              8



#define APREQ_FLAGS_OFF(f, name) ((f) &= ~(name##_MASK << name##_BIT))
#define APREQ_FLAGS_ON(f, name)  ((f) |=  (name##_MASK << name##_BIT))
#define APREQ_FLAGS_GET(f, name) (((f) >> name##_BIT) & name##_MASK)
#define APREQ_FLAGS_SET(f, name, value)                 \
    ((f) = (((f) & ~(name##_MASK << name##_BIT))        \
            | ((name##_MASK & (value)) << name##_BIT)))


#define APREQ_CHARSET_BIT           0
#define APREQ_CHARSET_MASK        255

#define APREQ_TAINTED_BIT           8
#define APREQ_TAINTED_MASK          1

#define APREQ_COOKIE_VERSION_BIT   11
#define APREQ_COOKIE_VERSION_MASK   3

#define APREQ_COOKIE_SECURE_BIT    13
#define APREQ_COOKIE_SECURE_MASK    1

/** Character encodings. */
typedef enum {
    APREQ_CHARSET_ASCII  =0,
    APREQ_CHARSET_LATIN1 =1, /* ISO-8859-1   */
    APREQ_CHARSET_CP1252 =2, /* Windows-1252 */
    APREQ_CHARSET_UTF8   =8
} apreq_charset_t;


/** @enum apreq_join_t Join type */
typedef enum { 
    APREQ_JOIN_AS_IS,      /**< Join the strings without modification */
    APREQ_JOIN_ENCODE,     /**< Url-encode the strings before joining them */
    APREQ_JOIN_DECODE,     /**< Url-decode the strings before joining them */
    APREQ_JOIN_QUOTE       /**< Quote the strings, backslashing existing quote marks. */
} apreq_join_t;

/** @enum apreq_match_t Match type */
typedef enum {
    APREQ_MATCH_FULL,       /**< Full match only. */
    APREQ_MATCH_PARTIAL     /**< Partial matches are ok. */
} apreq_match_t;

/** @enum apreq_expires_t Expiration date format */
typedef enum {
    APREQ_EXPIRES_HTTP,       /**< Use date formatting consistent with RFC 2616 */
    APREQ_EXPIRES_NSCOOKIE    /**< Use format consistent with Netscape's Cookie Spec */
} apreq_expires_t;


/** @brief libapreq's pre-extensible string type */
typedef struct apreq_value_t {
    char             *name;    /**< value name */
    apr_size_t        nlen;    /**< length of name */
    apr_size_t        dlen;    /**< length of data */
    char              data[1]; /**< value data  */
} apreq_value_t;

static APR_INLINE
void apreq_value_table_add(const apreq_value_t *v, apr_table_t *t) {
    apr_table_addn(t, v->name, v->data);
}

#define apreq_attr_to_type(T,A,P) ( (T*) ((char*)(P)-offsetof(T,A)) )

/**
 * Initialize libapreq2. Applications (except apache modules using
 * mod_apreq) should call this exactly once before they use any
 * libapreq2 modules.  If you want to modify the list of default parsers
 * with apreq_register_parser(), please use apreq_pre_initialize()
 * and apreq_post_initialize() instead.
 *
 * @param pool a base pool persisting while libapreq2 is used
 * @remarks after you detroy the pool, you have to call this function again
 *    with a new pool if you still plan to use libapreq2
 */
APREQ_DECLARE(apr_status_t) apreq_initialize(apr_pool_t *pool);


/**
 * Pre-initialize libapreq2. Applications (except apache modules using
 * mod_apreq2) should call this exactly once before they register custom
 * parsers with libapreq2. mod_apreq2 does this automatically during the
 * post-config phase, so modules that need call apreq_register_parser should
 * create a post-config hook using APR_HOOK_MIDDLE.
 *
 * @param pool a base pool persisting while libapreq2 is used
 * @remarks after you detroyed the pool, you have to call this function again
 *    with a new pool if you still plan to use libapreq2
 */
APREQ_DECLARE(apr_status_t) apreq_pre_initialize(apr_pool_t *pool);

/**
 * Post-initialize libapreq2. Applications (except apache modules using
 * mod_apreq2) should this exactly once before they use any
 * libapreq2 modules for parsing.
 *
 * @param pool the same pool that was used in apreq_pre_initialize().
 */
APREQ_DECLARE(apr_status_t) apreq_post_initialize(apr_pool_t *pool);


#ifdef __cplusplus
 }
#endif

#endif /* APREQ_H */
