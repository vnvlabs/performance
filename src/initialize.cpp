
#include "initialize.h"

bool VnVPapi::InitalizePAPI() {
  if (!VnVPapi::PAPI_IS_INITAILIZED) {
    int retVal = PAPI_library_init(PAPI_VER_CURRENT);
    VnVPapi::PAPI_IS_INITAILIZED = (retVal == PAPI_VER_CURRENT);
  }
  return VnVPapi::PAPI_IS_INITAILIZED;
}

