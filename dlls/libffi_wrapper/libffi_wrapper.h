#ifndef libffi_wrapper_H
#define libffi_wrapper_H

#include "ffi.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "../../object_system/object.h"
#include "../../datatypes/map.h"
#include "../../runtime/import_dll.h"

object duck_module_init(executor* Ex);

#endif