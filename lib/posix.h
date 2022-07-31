#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define ALIGN   ((4 > sizeof(void *)) ? 4 : sizeof(void *))

typedef void *simpptr_t;

typedef uint8_t usimpbyte_t;

#if INT32_MAX >= INTPTR_MAX
typedef int32_t   simpint_t;
#define SIMP_MAX  INT32_MAX
#define SIMP_MIN  INT32_MIN
#else
typedef intptr_t  simpint_t;
#define SIMP_MAX  INTPTR_MAX
#define SIMP_MIN  INTPTR_MIN
#endif

#if UINT32_MAX >= UINTPTR_MAX
typedef uint32_t  usimpint_t;
#define USIMP_MAX UINT32_MAX
#else
typedef uintptr_t usimpint_t;
#define USIMP_MAX UINTPTR_MAX
#endif

#define REALLOC(p, n)   (realloc((p), (n)))
#define GETC(p)         (fgetc((FILE *)(p)))
#define UNGETC(p, c)    (ungetc((int)(c), (FILE *)(p)))
#define PRINT(p, ...)   (fprintf((FILE *)(p), __VA_ARGS__))
#define MALLOC(n)       (malloc((n)))
#define MEMCPY(d, s, n) (memcpy((d), (s), (n)))

static inline void *
MALLOCALIGN(size_t n)
{
	void *p;

	return (posix_memalign(&p, ALIGN, n) == 0) ? p : NULL;
}
