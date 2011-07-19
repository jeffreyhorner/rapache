#define CORE_PRIVATE
#include "http_config.h"
#undef CORE_PRIVATE

extern module MODULE_VAR_EXPORT apreq_module;

struct dir_config {
    const char         *temp_dir;
    apr_uint64_t        read_limit;
    apr_size_t          brigade_limit;
};
