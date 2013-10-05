#ifndef __FILE_OPERATION_
#define __FILE_OPERATION_

#include "error_code.h"
#include <tchar.h>

error_code_t goto_path(_TCHAR* path);
_TCHAR* find_file();
void stop_find_file();

#endif