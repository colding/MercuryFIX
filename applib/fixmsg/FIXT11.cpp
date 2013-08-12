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

const struct FIX_Tag fixt11_std_tags[] = 
{
        {7, ft_SeqNum},
        {8, ft_String},
        {9, ft_Length},
        {10, ft_String},
        {16, ft_SeqNum},
        {34, ft_SeqNum},
        {35, ft_String},
        {36, ft_SeqNum},
        {43, ft_Boolean},
        {45, ft_SeqNum},
        {49, ft_String},
        {50, ft_String},
        {52, ft_UTCTimestamp},
        {56, ft_String},
        {57, ft_String},
        {58, ft_String},
        {89, ft_data},
        {90, ft_Length},
        {91, ft_data},
        {93, ft_Length},
        {95, ft_Length},
        {96, ft_data},
        {97, ft_Boolean},
        {98, ft_int},
        {108, ft_int},
        {112, ft_String},
        {115, ft_String},
        {116, ft_String},
        {122, ft_UTCTimestamp},
        {123, ft_Boolean},
        {128, ft_String},
        {129, ft_String},
        {141, ft_Boolean},
        {142, ft_String},
        {143, ft_String},
        {144, ft_String},
        {145, ft_String},
        {212, ft_Length},
        {213, ft_data},
        {347, ft_String},
        {354, ft_Length},
        {355, ft_data},
        {369, ft_SeqNum},
        {371, ft_int},
        {372, ft_String},
        {373, ft_int},
        {383, ft_Length},
        {384, ft_NumInGroup},
        {385, ft_char},
        {464, ft_Boolean},
        {553, ft_String},
        {554, ft_String},
        {627, ft_NumInGroup},
        {628, ft_String},
        {629, ft_UTCTimestamp},
        {630, ft_SeqNum},
        {789, ft_SeqNum},
        {1128, ft_String},
        {1129, ft_String},
        {1130, ft_String},
        {1131, ft_String},
        {1137, ft_String},
        {0, ft_int}, // terminating 0 tag
};

const struct FIX_Tag fixt11_std_data_tags[] =
{
        {89, ft_data},
        {91, ft_data},
        {96, ft_data},
        {213, ft_data},
        {355, ft_data},
        {0, ft_int}, // terminating 0 tag
};
