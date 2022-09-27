/* Fast Modular Exponentiation */

#ifndef __MOD_H__
#define __MOD_H__

#include <stdint.h>

/* (a*b)%c */
uint64_t mulMod(uint64_t a, uint64_t b, uint64_t c)
{
    //return (a*b)%c;
    uint64_t ans = 0;
    while(b)
    {
        if(b&1)
            ans = (ans + a) % c;
        a = (a + a) % c;
        b >>= 1;
    }
    return ans;
}

/* (a^b)%c */
uint64_t powMod(uint64_t a, uint64_t b, uint64_t c)
{
    uint64_t ans = 1;
    uint64_t base = a%c;
    while(b)
    {
        if(b&1)
            ans = mulMod(ans, base, c);
        base = mulMod(base, base, c);
        b >>= 1;
    }
    return ans;
}

uint64_t mulMod32(uint64_t a, uint64_t b, uint64_t c)
{
    return (a*b)%c;
}

uint64_t powMod32(uint64_t a, uint64_t b, uint64_t c)
{
    uint64_t ans = 1;
    uint64_t base = a%c;
    while(b)
    {
        if(b&1)
            ans = (ans*base)%c;
        base = (base*base)%c;
        b >>= 1;
    }
    return ans;
}


#endif