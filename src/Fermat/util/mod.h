#ifndef _MOD_H_
#define _MOD_H_

/* Fast Modular Exponentiation */

#include <cstdint>
/* (a*b)%c */
inline uint64_t mulMod(uint64_t a, uint64_t b, uint64_t c)
{
    //return (a*b)%c;
    uint64_t ans = 0;
    while(b)
    {
        if(b&1) {
            ans = (ans + a);
            if (ans & (1LL << 63))
                ans = ans - c;
        }
        a <<= 1;
        a = a - c; // a = (a + a) % c
        b >>= 1;
    }
    return ans;
}

/* (a^b)%c */
inline uint64_t powMod(uint64_t a, uint64_t b, uint64_t c)
{
    uint64_t ans = 1;
    uint64_t base = a; // a %c
    while(b)
    {
        if(b&1)
            ans = mulMod(ans, base, c);
        base = mulMod(base, base, c);
        b >>= 1;
    }
    return ans;
}

inline uint64_t mulMod32(uint64_t a, uint64_t b, uint64_t c)
{
    return (a*b)%c;
}

inline uint64_t powMod32(uint64_t a, uint64_t b, uint64_t c)
{
    uint64_t ans = 1;
    uint64_t base = a; // a % c
    while(b)
    {
        if(b&1)
            ans = (ans*base)%c;
        base = (base*base)%c;
        b >>= 1;
    }
    return ans;
}

inline uint32_t powMod16(uint32_t a, uint32_t b, uint32_t c) {
    uint32_t ans = 1;
    uint32_t base = a;
    while (b) {
        if (b&1) {
            ans = (ans * base) % c;
        }
        base = (base*base)%c;
        b >>= 1;
    }
    return ans;
}

#if 1

inline void powModBatch(uint64_t *a, uint64_t b, uint64_t *c, uint64_t p) 
{
    for(int i=0;i<8;i++)
    {
        c[i] = powMod32(a[i], b, p);
    }
}

#else

#include <immintrin.h>
void mulModSIMD1(uint64_t *a, uint64_t *b, uint64_t *c, __m512i _p, __m512d _k)
{
    __m512i _a = _mm512_loadu_si512((__m512i*)(a));
    __m512i _b = _mm512_loadu_si512((__m512i*)(b));
    __m512i _d = _mm512_mullo_epi64(_a, _b);
    __m512d _d1=_mm512_cvtepi64_pd(_d);
    __m512d _d2=_mm512_mul_pd(_d1, _k);
    __m512i _e = _mm512_cvttpd_epi64(_d2); // e = int(float(d)/float(p));
    __m512i _d3= _mm512_mullo_epi64(_e, _p);
    __m512i _c = _mm512_sub_epi64(_d, _d3);
    _mm512_storeu_si512((__m512i*)(c), _c);        
}
void mulModSIMD2(uint64_t *a,uint64_t *c, __m512i _p, __m512d _k)
{
    __m512i _a = _mm512_loadu_si512((__m512i*)(a));
    __m512d _d1=_mm512_cvtepi64_pd(_a);
    __m512d _d2=_mm512_mul_pd(_d1, _k);
     __m512i _e = _mm512_cvttpd_epi64(_d2); // e = int(float(d)/float(p));
    __m512i _d3= _mm512_mullo_epi64(_e, _p);
     __m512i _c = _mm512_sub_epi64(_a, _d3);
    _mm512_storeu_si512((__m512i*)(c), _c);
}
void powModBatch(uint64_t *a, uint64_t b, uint64_t *c, uint64_t p) 
{
    __m512i _p = _mm512_set1_epi64(p);
    __m512d _k = _mm512_set1_pd(1.0d / p);
    mulModSIMD2(a, base, _p, _k);
    while(b)
    {
        if(b&1)
            mulModSIMD1(c, base, c, _p, _k);
        mulModSIMD1(base, base, base);
        b>>= 1;
    }
}
#endif


#endif