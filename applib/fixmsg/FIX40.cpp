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

#include "fix_types.h"

#ifdef HAVE_CONFIG_H
    #include "ac_config.h"
#endif

const struct FIX_Tag fix40_std_tags[] =
{
        {1, ft_char},
        {2, ft_int},
        {3, ft_int},
        {4, ft_char},
        {5, ft_char},
        {6, ft_float},
        {7, ft_int},
        {8, ft_char},
        {9, ft_int},
        {10, ft_char},
        {11, ft_char},
        {12, ft_float},
        {13, ft_char},
        {14, ft_int},
        {15, ft_char},
        {16, ft_int},
        {17, ft_int},
        {18, ft_char},
        {19, ft_int},
        {20, ft_char},
        {21, ft_char},
        {22, ft_char},
        {23, ft_int},
        {24, ft_char},
        {25, ft_char},
        {26, ft_int},
        {27, ft_char},
        {28, ft_char},
        {29, ft_char},
        {30, ft_char},
        {31, ft_float},
        {32, ft_int},
        {33, ft_int},
        {34, ft_int},
        {35, ft_char},
        {36, ft_int},
        {37, ft_char},
        {38, ft_int},
        {39, ft_char},
        {40, ft_char},
        {41, ft_char},
        {42, ft_UTCTimestamp},
        {43, ft_char},
        {44, ft_float},
        {45, ft_int},
        {46, ft_char},
        {47, ft_char},
        {48, ft_char},
        {49, ft_char},
        {50, ft_char},
        {52, ft_UTCTimestamp},
        {53, ft_int},
        {54, ft_char},
        {55, ft_char},
        {56, ft_char},
        {57, ft_char},
        {58, ft_char},
        {59, ft_char},
        {60, ft_UTCTimestamp},
        {61, ft_char},
        {62, ft_UTCTimestamp},
        {63, ft_char},
        {64, ft_LocalMktDate},
        {65, ft_char},
        {66, ft_char},
        {67, ft_int},
        {68, ft_int},
        {69, ft_char},
        {70, ft_int},
        {71, ft_char},
        {72, ft_int},
        {73, ft_int},
        {74, ft_int},
        {75, ft_LocalMktDate},
        {76, ft_char},
        {77, ft_char},
        {78, ft_int},
        {79, ft_char},
        {80, ft_int},
        {81, ft_char},
        {82, ft_int},
        {83, ft_int},
        {84, ft_int},
        {85, ft_int},
        {86, ft_char},
        {87, ft_int},
        {88, ft_int},
        {89, ft_data},
        {90, ft_Length},
        {91, ft_data},
        {92, ft_char},
        {93, ft_Length},
        {94, ft_char},
        {95, ft_Length},
        {96, ft_data},
        {97, ft_char},
        {98, ft_int},
        {99, ft_float},
        {100, ft_char},
        {102, ft_int},
        {103, ft_int},
        {104, ft_char},
        {105, ft_char},
        {106, ft_char},
        {107, ft_char},
        {108, ft_int},
        {109, ft_char},
        {110, ft_int},
        {111, ft_int},
        {112, ft_char},
        {113, ft_char},
        {114, ft_char},
        {115, ft_char},
        {116, ft_char},
        {117, ft_char},
        {118, ft_float},
        {119, ft_float},
        {120, ft_char},
        {121, ft_char},
        {122, ft_UTCTimestamp},
        {123, ft_char},
        {124, ft_int},
        {125, ft_char},
        {126, ft_UTCTimestamp},
        {127, ft_char},
        {128, ft_char},
        {129, ft_char},
        {130, ft_char},
        {131, ft_char},
        {132, ft_float},
        {133, ft_float},
        {134, ft_int},
        {135, ft_int},
        {136, ft_int},
        {137, ft_float},
        {138, ft_char},
        {139, ft_char},
        {140, ft_float},
        {0, ft_int}, // terminating 0 tag
};

const struct FIX_Tag fix40_std_data_tags[] =
{
        {89, ft_data},
        {91, ft_data},
        {96, ft_data},
        {0, ft_int}, // terminating 0 tag
};
