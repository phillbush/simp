#include <limits.h>
#include <stdlib.h>
#include <math.h>

#include "simp.h"

bool
simp_arithabs(Simp ctx, Simp *ret, Simp n)
{
	if (simp_issignum(n)) return simp_makesignum(
		ctx,
		ret,
		llabs(simp_getsignum(n))
	);
	if (simp_isreal(n)) return simp_makereal(
		ctx,
		ret,
		fabs(simp_getreal(n))
	);
	return false;
}

bool
simp_arithadd(Simp ctx, Simp *ret, Simp a, Simp b)
{
	if (simp_issignum(a) && simp_issignum(b)) return simp_makesignum(
		ctx,
		ret,
		simp_getsignum(a) + simp_getsignum(b)
	);
	if (simp_issignum(a) && simp_isreal(b)) return simp_makereal(
		ctx,
		ret,
		(double)simp_getsignum(a) + simp_getreal(b)
	);
	if (simp_isreal(a) && simp_issignum(b)) return simp_makereal(
		ctx,
		ret,
		simp_getreal(a) + (double)simp_getsignum(b)
	);
	if (simp_isreal(a) && simp_isreal(b)) return simp_makereal(
		ctx,
		ret,
		simp_getreal(a) + simp_getreal(b)
	);
	return false;
}

bool
simp_arithdiff(Simp ctx, Simp *ret, Simp a, Simp b)
{
	if (simp_issignum(a) && simp_issignum(b)) return simp_makesignum(
		ctx,
		ret,
		simp_getsignum(a) - simp_getsignum(b)
	);
	if (simp_issignum(a) && simp_isreal(b)) return simp_makereal(
		ctx,
		ret,
		(double)simp_getsignum(a) - simp_getreal(b)
	);
	if (simp_isreal(a) && simp_issignum(b)) return simp_makereal(
		ctx,
		ret,
		simp_getreal(a) - (double)simp_getsignum(b)
	);
	if (simp_isreal(a) && simp_isreal(b)) return simp_makereal(
		ctx,
		ret,
		simp_getreal(a) - simp_getreal(b)
	);
	return false;
}

bool
simp_arithmul(Simp ctx, Simp *ret, Simp a, Simp b)
{
	if (simp_issignum(a) && simp_issignum(b)) return simp_makesignum(
		ctx,
		ret,
		simp_getsignum(a) * simp_getsignum(b)
	);
	if (simp_issignum(a) && simp_isreal(b)) return simp_makereal(
		ctx,
		ret,
		(double)simp_getsignum(a) * simp_getreal(b)
	);
	if (simp_isreal(a) && simp_issignum(b)) return simp_makereal(
		ctx,
		ret,
		simp_getreal(a) * (double)simp_getsignum(b)
	);
	if (simp_isreal(a) && simp_isreal(b)) return simp_makereal(
		ctx,
		ret,
		simp_getreal(a) * simp_getreal(b)
	);
	return false;
}

bool
simp_arithdiv(Simp ctx, Simp *ret, Simp a, Simp b)
{
	if (simp_issignum(a) && simp_issignum(b)) return simp_makesignum(
		ctx,
		ret,
		simp_getsignum(a) / simp_getsignum(b)
	);
	if (simp_issignum(a) && simp_isreal(b)) return simp_makereal(
		ctx,
		ret,
		(double)simp_getsignum(a) / simp_getreal(b)
	);
	if (simp_isreal(a) && simp_issignum(b)) return simp_makereal(
		ctx,
		ret,
		simp_getreal(a) / (double)simp_getsignum(b)
	);
	if (simp_isreal(a) && simp_isreal(b)) return simp_makereal(
		ctx,
		ret,
		simp_getreal(a) / simp_getreal(b)
	);
	return false;
}

bool
simp_arithzero(Simp n)
{
	if (simp_issignum(n))
		return simp_getsignum(n) == 0;
	if (simp_isreal(n))
		return simp_getreal(n) == 0.0;
	return false;
}

int
simp_arithcmp(Simp a, Simp b)
{
	SimpInt n, m;
	double x, y;

	if (simp_issignum(a) && simp_issignum(b)) {
		n = simp_getsignum(a);
		m = simp_getsignum(b);
		if (n < m)
			return -1;
		if (n > m)
			return +1;
		return 0;
	}
	if (simp_issignum(a) && simp_isreal(b)) {
		x = (double)simp_getsignum(a);
		y = simp_getreal(b);
	} else if (simp_isreal(a) && simp_issignum(b)) {
		x = simp_getreal(a);
		y = (double)simp_getsignum(b);
	} else if (simp_isreal(a) && simp_isreal(b)) {
		x = simp_getreal(a);
		y = simp_getreal(b);
	} else {
		return 0;
	}
	if (x < y)
		return -1;
	if (x > y)
		return +1;
	return 0;
}
