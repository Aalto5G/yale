#ifndef _YALEUINT_H_
#define _YALEUINT_H_

#include <stdint.h>

typedef uint8_t yale_uint_t;

// Don't use this, use YALE_UINT_MAX_LEGAL instead
#define YALE_UINT_MAX UINT8_MAX

// This plus 1 must be multiple of 64, this must be at least 255
#define YALE_UINT_MAX_LEGAL UINT8_MAX

#endif
