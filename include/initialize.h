#ifndef VNVPAPI_H
#define VNVPAPI_H

#include "VnV.h"
#include "papi.h"
#include <iostream>
#include <sys/utsname.h>

#define PNAME Papi

namespace VnVPapi {
    
    static bool PAPI_IS_INITAILIZED = false;
    bool InitalizePAPI();
}

#endif