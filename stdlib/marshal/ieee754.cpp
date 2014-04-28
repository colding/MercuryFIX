/*
 *    Copyright (C) 2013, Jules Colding <jcolding@gmail.com>.
 *
 *    All Rights Reserved.
 */

/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     (1) Redistributions of source code must retain the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer.
 *
 *     (2) Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *
 *     (3) Neither the name of the copyright holder nor the names of
 *     its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written
 *     permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifdef HAVE_CONFIG_H
    #include "ac_config.h"
#endif
#include "ieee754.h"

static inline uint64_t
pack754(long double f,
        unsigned int bits,
        unsigned int expbits)
{
        long double fnorm;
        int shift;
        int64_t sign;
        int64_t exp;
        uint64_t significand;
        unsigned int significand_bits = bits - expbits - 1; // -1 for sign bit

        if (!f) {
                return 0; // get this special case out of the way
        }

        // check sign and begin normalization
        if (0 > f) {
                sign = 1;
                fnorm = (-1)*f;
        } else {
                sign = 0;
                fnorm = f;
        }

        // get the normalized form of f and track the exponent
        shift = 0;
        while (2.0 <= fnorm) {
                fnorm /= 2.0;
                shift++;
        }
        while (1.0 > fnorm) {
                fnorm *= 2.0;
                shift--;
        }
        fnorm = fnorm - 1.0;

        // calculate the binary form (non-float) of the significand data
        significand = (uint64_t)(fnorm * ((1LL<<significand_bits) + 0.5f));

        // get the biased exponent
        exp = shift + ((1<<(expbits-1)) - 1); // shift + bias

        // return the final answer
        return (sign<<(bits-1)) | (exp<<(bits-expbits-1)) | significand;
}

static inline long double
unpack754(uint64_t i,
          unsigned int bits,
          unsigned int expbits)
{
        long double retv;
        int64_t shift;
        unsigned int bias;
        unsigned int significand_bits = bits - expbits - 1; // -1 for sign bit

        if (!i) {
                return 0.0;
        }

        // pull the significand
        retv = (i & ((1LL << significand_bits) - 1)); // mask
        retv /= (1LL << significand_bits); // convert back to float
        retv += 1.0f; // add the one back on

        // deal with the exponent
        bias = (1 << (expbits - 1)) - 1;
        shift = ((i >> significand_bits) & ((1LL << expbits) - 1)) - bias;
        while (shift > 0) {
                retv *= 2.0;
                shift--;
        }
        while (shift < 0) {
                retv /= 2.0;
                shift++;
        }

        // sign it
        retv *= (i >> (bits - 1)) & 1 ? - 1.0 : 1.0;

        return retv;
}

uint32_t
pack754_32(float val)
{
        return pack754(val, 32, 8);
}

uint64_t
pack754_64(double val)
{
        return pack754(val, 64, 11);
}

float
unpack754_32(uint32_t val)
{
        return (float)unpack754(val, 32, 8);
}

double
unpack754_64(uint64_t val)
{
        return (double)unpack754(val, 64, 11);
}
