/*
**  Copyright 2004-2005  The Apache Software Foundation
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

#ifndef APREQ_XS_PREPERL_H
#define APREQ_XS_PREPERL_H

#define PERL_NO_GET_CONTEXT /* we want efficiency under ithreads */

/* some redefines needed for Win32 */
#ifdef WIN32
#   define uid_t perl_uid_t
#   define gid_t perl_gid_t
#   ifdef exit
#      define perl_exit exit
#      undef exit
#   endif
#endif

/* Undo httpd.h's strchr override. */
#ifdef AP_DEBUG
#    undef strchr
#endif

/**
 * @file apreq_xs_preperl.h
 * @brief XS include file for making Cookie.so and Request.so, for things
 *        that has to be included before EXTERN.h/perl.h/XSUB.h headers
 *
 */
/**
 * @defgroup XS Perl
 * @ingroup GLUE
 * @{
 */

/** @} */

#endif /* APREQ_XS_PREPERL_H */
