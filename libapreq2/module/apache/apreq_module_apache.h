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

#ifndef APREQ_APACHE_H
#define APREQ_APACHE_H

#include "apreq_module.h"
#include <httpd.h>

#ifdef  __cplusplus
 extern "C" {
#endif

/**
 * Create an apreq handle which communicates with an Apache 1.3.x
 * request_rec.
 */
APREQ_DECLARE(apreq_handle_t*) apreq_handle_apache(request_rec *r);


APREQ_DECLARE(apr_pool_t *) apreq_handle_apache_pool(apreq_handle_t *req);

APREQ_DECLARE(apr_bucket_alloc_t *)
    apreq_handle_apache_bucket_alloc(apreq_handle_t *req);

#ifdef __cplusplus
 }
#endif

#endif
