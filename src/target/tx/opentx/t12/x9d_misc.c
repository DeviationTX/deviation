#include "common.h"

#define __SECTION_USED(s)  __attribute__ ((section(s), used))
__SECTION_USED(".fwversiondata")   const char firmware_version[] = "opentx-" HGVERSION;
void TxName(u8 *var, int len)
{
    strlcpy((char *)var, "T12", len);
}
