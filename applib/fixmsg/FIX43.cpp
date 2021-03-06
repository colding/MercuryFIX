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

const struct FIX_Tag fix43_std_tags[] =
{
        {1, ft_String},
        {2, ft_String},
        {3, ft_String},
        {4, ft_char},
        {5, ft_String},
        {6, ft_Price},
        {7, ft_SeqNum},
        {8, ft_String},
        {9, ft_Length},
        {10, ft_String},
        {11, ft_String},
        {12, ft_Amt},
        {13, ft_char},
        {14, ft_Qty},
        {15, ft_Currency},
        {16, ft_SeqNum},
        {17, ft_String},
        {18, ft_MultipleCharValue},
        {19, ft_String},
        {21, ft_char},
        {22, ft_String},
        {23, ft_String},
        {25, ft_char},
        {26, ft_String},
        {27, ft_String},
        {28, ft_char},
        {29, ft_char},
        {30, ft_Exchange},
        {31, ft_Price},
        {32, ft_Qty},
        {33, ft_NumInGroup},
        {34, ft_SeqNum},
        {35, ft_String},
        {36, ft_SeqNum},
        {37, ft_String},
        {38, ft_Qty},
        {39, ft_char},
        {40, ft_char},
        {41, ft_String},
        {42, ft_UTCTimestamp},
        {43, ft_Boolean},
        {44, ft_Price},
        {45, ft_SeqNum},
        {47, ft_char},
        {48, ft_String},
        {49, ft_String},
        {50, ft_String},
        {52, ft_UTCTimestamp},
        {53, ft_Qty},
        {54, ft_char},
        {55, ft_String},
        {56, ft_String},
        {57, ft_String},
        {58, ft_String},
        {59, ft_char},
        {60, ft_UTCTimestamp},
        {61, ft_char},
        {62, ft_UTCTimestamp},
        {63, ft_char},
        {64, ft_LocalMktDate},
        {65, ft_String},
        {66, ft_String},
        {67, ft_int},
        {68, ft_int},
        {69, ft_String},
        {70, ft_String},
        {71, ft_char},
        {72, ft_String},
        {73, ft_NumInGroup},
        {74, ft_int},
        {75, ft_LocalMktDate},
        {77, ft_char},
        {78, ft_NumInGroup},
        {79, ft_String},
        {80, ft_Qty},
        {81, ft_char},
        {82, ft_NumInGroup},
        {83, ft_int},
        {84, ft_Qty},
        {87, ft_int},
        {88, ft_int},
        {89, ft_data},
        {90, ft_Length},
        {91, ft_data},
        {93, ft_Length},
        {94, ft_char},
        {95, ft_Length},
        {96, ft_data},
        {97, ft_Boolean},
        {98, ft_int},
        {99, ft_Price},
        {100, ft_Exchange},
        {102, ft_int},
        {103, ft_int},
        {104, ft_char},
        {106, ft_String},
        {107, ft_String},
        {108, ft_int},
        {110, ft_Qty},
        {111, ft_Qty},
        {112, ft_String},
        {113, ft_Boolean},
        {114, ft_Boolean},
        {115, ft_String},
        {116, ft_String},
        {117, ft_String},
        {118, ft_Amt},
        {119, ft_Amt},
        {120, ft_Currency},
        {121, ft_Boolean},
        {122, ft_UTCTimestamp},
        {123, ft_Boolean},
        {124, ft_NumInGroup},
        {126, ft_UTCTimestamp},
        {127, ft_char},
        {128, ft_String},
        {129, ft_String},
        {130, ft_Boolean},
        {131, ft_String},
        {132, ft_Price},
        {133, ft_Price},
        {134, ft_Qty},
        {135, ft_Qty},
        {136, ft_NumInGroup},
        {137, ft_Amt},
        {138, ft_Currency},
        {139, ft_char},
        {140, ft_Price},
        {141, ft_Boolean},
        {142, ft_String},
        {143, ft_String},
        {144, ft_String},
        {145, ft_String},
        {146, ft_NumInGroup},
        {147, ft_String},
        {148, ft_String},
        {149, ft_String},
        {150, ft_char},
        {151, ft_Qty},
        {152, ft_Qty},
        {153, ft_Price},
        {154, ft_Amt},
        {155, ft_float},
        {156, ft_char},
        {157, ft_int},
        {158, ft_Percentage},
        {159, ft_Amt},
        {160, ft_char},
        {161, ft_String},
        {162, ft_String},
        {163, ft_char},
        {164, ft_String},
        {165, ft_char},
        {167, ft_String},
        {168, ft_UTCTimestamp},
        {169, ft_int},
        {170, ft_String},
        {171, ft_String},
        {172, ft_int},
        {173, ft_String},
        {174, ft_String},
        {175, ft_String},
        {176, ft_String},
        {177, ft_String},
        {178, ft_String},
        {179, ft_String},
        {180, ft_String},
        {181, ft_String},
        {182, ft_String},
        {183, ft_String},
        {184, ft_String},
        {185, ft_String},
        {186, ft_String},
        {187, ft_String},
        {188, ft_Price},
        {189, ft_Price},
        {190, ft_Price},
        {191, ft_Price},
        {192, ft_Qty},
        {193, ft_LocalMktDate},
        {194, ft_Price},
        {195, ft_Price},
        {196, ft_String},
        {197, ft_int},
        {198, ft_String},
        {199, ft_NumInGroup},
        {200, ft_MonthYear},
        {202, ft_Price},
        {203, ft_int},
        {206, ft_char},
        {207, ft_Exchange},
        {208, ft_Boolean},
        {209, ft_int},
        {210, ft_Qty},
        {211, ft_Price},
        {212, ft_Length},
        {213, ft_data},
        {214, ft_String},
        {215, ft_NumInGroup},
        {216, ft_int},
        {217, ft_String},
        {218, ft_Price},
        {219, ft_char},
        {220, ft_Currency},
        {221, ft_String},
        {222, ft_String},
        {223, ft_Percentage},
        {224, ft_UTCDateOnly},
        {225, ft_UTCDateOnly},
        {226, ft_int},
        {227, ft_Percentage},
        {228, ft_float},
        {229, ft_UTCDateOnly},
        {230, ft_UTCDateOnly},
        {231, ft_float},
        {232, ft_NumInGroup},
        {233, ft_String},
        {234, ft_String},
        {235, ft_String},
        {236, ft_Percentage},
        {237, ft_Amt},
        {238, ft_Amt},
        {239, ft_String},
        {240, ft_UTCDateOnly},
        {241, ft_UTCDateOnly},
        {242, ft_UTCDateOnly},
        {243, ft_String},
        {244, ft_int},
        {245, ft_Percentage},
        {246, ft_float},
        {247, ft_UTCDateOnly},
        {248, ft_UTCDateOnly},
        {249, ft_UTCDateOnly},
        {250, ft_String},
        {251, ft_int},
        {252, ft_Percentage},
        {253, ft_float},
        {254, ft_UTCDateOnly},
        {255, ft_String},
        {256, ft_String},
        {257, ft_String},
        {258, ft_Boolean},
        {259, ft_UTCDateOnly},
        {260, ft_Price},
        {262, ft_String},
        {263, ft_char},
        {264, ft_int},
        {265, ft_int},
        {266, ft_Boolean},
        {267, ft_NumInGroup},
        {268, ft_NumInGroup},
        {269, ft_char},
        {270, ft_Price},
        {271, ft_Qty},
        {272, ft_UTCDateOnly},
        {273, ft_UTCTimeOnly},
        {274, ft_char},
        {275, ft_Exchange},
        {276, ft_MultipleStringValue},
        {277, ft_MultipleStringValue},
        {278, ft_String},
        {279, ft_char},
        {280, ft_String},
        {281, ft_char},
        {282, ft_String},
        {283, ft_String},
        {284, ft_String},
        {285, ft_char},
        {286, ft_MultipleStringValue},
        {287, ft_int},
        {288, ft_String},
        {289, ft_String},
        {290, ft_int},
        {291, ft_MultipleStringValue},
        {292, ft_MultipleStringValue},
        {293, ft_Qty},
        {294, ft_Qty},
        {295, ft_NumInGroup},
        {296, ft_NumInGroup},
        {297, ft_int},
        {298, ft_int},
        {299, ft_String},
        {300, ft_int},
        {301, ft_int},
        {302, ft_String},
        {303, ft_int},
        {304, ft_int},
        {305, ft_String},
        {306, ft_String},
        {307, ft_String},
        {308, ft_Exchange},
        {309, ft_String},
        {310, ft_String},
        {311, ft_String},
        {312, ft_String},
        {313, ft_MonthYear},
        {315, ft_int},
        {316, ft_Price},
        {317, ft_char},
        {320, ft_String},
        {321, ft_int},
        {322, ft_String},
        {323, ft_int},
        {324, ft_String},
        {325, ft_Boolean},
        {326, ft_int},
        {327, ft_char},
        {328, ft_Boolean},
        {329, ft_Boolean},
        {330, ft_Qty},
        {331, ft_Qty},
        {332, ft_Price},
        {333, ft_Price},
        {334, ft_int},
        {335, ft_String},
        {336, ft_String},
        {337, ft_String},
        {338, ft_int},
        {339, ft_int},
        {340, ft_int},
        {341, ft_UTCTimestamp},
        {342, ft_UTCTimestamp},
        {343, ft_UTCTimestamp},
        {344, ft_UTCTimestamp},
        {345, ft_UTCTimestamp},
        {346, ft_int},
        {347, ft_String},
        {348, ft_Length},
        {349, ft_data},
        {350, ft_Length},
        {351, ft_data},
        {352, ft_Length},
        {353, ft_data},
        {354, ft_Length},
        {355, ft_data},
        {356, ft_Length},
        {357, ft_data},
        {358, ft_Length},
        {359, ft_data},
        {360, ft_Length},
        {361, ft_data},
        {362, ft_Length},
        {363, ft_data},
        {364, ft_Length},
        {365, ft_data},
        {366, ft_Price},
        {367, ft_UTCTimestamp},
        {368, ft_int},
        {369, ft_SeqNum},
        {370, ft_UTCTimestamp},
        {371, ft_int},
        {372, ft_String},
        {373, ft_int},
        {374, ft_char},
        {375, ft_String},
        {376, ft_String},
        {377, ft_Boolean},
        {378, ft_int},
        {379, ft_String},
        {380, ft_int},
        {381, ft_Amt},
        {382, ft_NumInGroup},
        {383, ft_Length},
        {384, ft_NumInGroup},
        {385, ft_char},
        {386, ft_NumInGroup},
        {387, ft_Qty},
        {388, ft_char},
        {389, ft_Price},
        {390, ft_String},
        {391, ft_String},
        {392, ft_String},
        {393, ft_int},
        {394, ft_int},
        {395, ft_int},
        {396, ft_Amt},
        {397, ft_Amt},
        {398, ft_NumInGroup},
        {399, ft_int},
        {400, ft_String},
        {401, ft_int},
        {402, ft_Percentage},
        {403, ft_Percentage},
        {404, ft_Amt},
        {405, ft_Percentage},
        {406, ft_Amt},
        {407, ft_Percentage},
        {408, ft_Amt},
        {409, ft_int},
        {410, ft_Percentage},
        {411, ft_Boolean},
        {412, ft_Amt},
        {413, ft_Percentage},
        {414, ft_int},
        {415, ft_int},
        {416, ft_int},
        {417, ft_int},
        {418, ft_char},
        {419, ft_char},
        {420, ft_NumInGroup},
        {421, ft_Country},
        {422, ft_int},
        {423, ft_int},
        {424, ft_Qty},
        {425, ft_Qty},
        {426, ft_Price},
        {427, ft_int},
        {428, ft_NumInGroup},
        {429, ft_int},
        {430, ft_int},
        {431, ft_int},
        {432, ft_LocalMktDate},
        {433, ft_char},
        {434, ft_char},
        {435, ft_Percentage},
        {436, ft_float},
        {437, ft_Qty},
        {438, ft_UTCTimestamp},
        {441, ft_int},
        {442, ft_char},
        {443, ft_UTCTimestamp},
        {444, ft_String},
        {445, ft_Length},
        {446, ft_data},
        {447, ft_char},
        {448, ft_String},
        {449, ft_UTCDateOnly},
        {450, ft_UTCTimeOnly},
        {451, ft_Price},
        {452, ft_int},
        {453, ft_NumInGroup},
        {454, ft_NumInGroup},
        {455, ft_String},
        {456, ft_String},
        {457, ft_NumInGroup},
        {458, ft_String},
        {459, ft_String},
        {460, ft_int},
        {461, ft_String},
        {462, ft_int},
        {463, ft_String},
        {464, ft_Boolean},
        {465, ft_int},
        {466, ft_String},
        {467, ft_String},
        {468, ft_char},
        {469, ft_float},
        {470, ft_Country},
        {471, ft_String},
        {472, ft_String},
        {473, ft_NumInGroup},
        {474, ft_String},
        {475, ft_Country},
        {476, ft_String},
        {477, ft_int},
        {478, ft_Currency},
        {479, ft_Currency},
        {480, ft_char},
        {481, ft_char},
        {482, ft_String},
        {483, ft_UTCTimestamp},
        {484, ft_char},
        {485, ft_float},
        {486, ft_LocalMktDate},
        {487, ft_char},
        {488, ft_String},
        {489, ft_String},
        {490, ft_LocalMktDate},
        {491, ft_String},
        {492, ft_int},
        {493, ft_String},
        {494, ft_String},
        {495, ft_int},
        {496, ft_String},
        {497, ft_char},
        {498, ft_String},
        {499, ft_String},
        {500, ft_String},
        {501, ft_String},
        {503, ft_LocalMktDate},
        {504, ft_LocalMktDate},
        {505, ft_String},
        {506, ft_char},
        {507, ft_int},
        {508, ft_String},
        {509, ft_String},
        {510, ft_NumInGroup},
        {511, ft_String},
        {512, ft_Percentage},
        {513, ft_String},
        {514, ft_char},
        {515, ft_UTCTimestamp},
        {516, ft_Percentage},
        {517, ft_char},
        {518, ft_NumInGroup},
        {519, ft_int},
        {520, ft_float},
        {521, ft_Currency},
        {522, ft_int},
        {523, ft_String},
        {524, ft_String},
        {525, ft_char},
        {526, ft_String},
        {527, ft_String},
        {528, ft_char},
        {529, ft_MultipleStringValue},
        {530, ft_char},
        {531, ft_char},
        {532, ft_char},
        {533, ft_int},
        {534, ft_NumInGroup},
        {535, ft_String},
        {536, ft_String},
        {537, ft_int},
        {538, ft_int},
        {539, ft_NumInGroup},
        {540, ft_Amt},
        {541, ft_LocalMktDate},
        {542, ft_LocalMktDate},
        {543, ft_String},
        {544, ft_char},
        {545, ft_String},
        {546, ft_MultipleStringValue},
        {547, ft_Boolean},
        {548, ft_String},
        {549, ft_int},
        {550, ft_int},
        {551, ft_String},
        {552, ft_NumInGroup},
        {553, ft_String},
        {554, ft_String},
        {555, ft_NumInGroup},
        {556, ft_Currency},
        {557, ft_int},
        {558, ft_NumInGroup},
        {559, ft_int},
        {560, ft_int},
        {561, ft_Qty},
        {562, ft_Qty},
        {563, ft_int},
        {564, ft_char},
        {565, ft_int},
        {566, ft_Price},
        {567, ft_int},
        {568, ft_String},
        {569, ft_int},
        {570, ft_Boolean},
        {571, ft_String},
        {572, ft_String},
        {573, ft_char},
        {574, ft_String},
        {575, ft_Boolean},
        {576, ft_int},
        {577, ft_int},
        {578, ft_String},
        {579, ft_String},
        {580, ft_NumInGroup},
        {581, ft_int},
        {582, ft_int},
        {583, ft_String},
        {584, ft_String},
        {585, ft_int},
        {586, ft_UTCTimestamp},
        {587, ft_char},
        {588, ft_LocalMktDate},
        {589, ft_char},
        {590, ft_char},
        {591, ft_char},
        {592, ft_Country},
        {593, ft_String},
        {594, ft_String},
        {595, ft_String},
        {596, ft_Country},
        {597, ft_String},
        {598, ft_String},
        {599, ft_String},
        {600, ft_String},
        {601, ft_String},
        {602, ft_String},
        {603, ft_String},
        {604, ft_NumInGroup},
        {605, ft_String},
        {606, ft_String},
        {607, ft_int},
        {608, ft_String},
        {609, ft_String},
        {610, ft_MonthYear},
        {611, ft_LocalMktDate},
        {612, ft_Price},
        {613, ft_char},
        {614, ft_float},
        {615, ft_Percentage},
        {616, ft_Exchange},
        {617, ft_String},
        {618, ft_Length},
        {619, ft_data},
        {620, ft_String},
        {621, ft_Length},
        {622, ft_data},
        {623, ft_float},
        {624, ft_char},
        {625, ft_String},
        {626, ft_int},
        {627, ft_NumInGroup},
        {628, ft_String},
        {629, ft_UTCTimestamp},
        {630, ft_SeqNum},
        {631, ft_Price},
        {632, ft_Percentage},
        {633, ft_Percentage},
        {634, ft_Percentage},
        {635, ft_String},
        {636, ft_Boolean},
        {637, ft_Price},
        {638, ft_int},
        {639, ft_Price},
        {640, ft_Price},
        {641, ft_Price},
        {642, ft_Price},
        {643, ft_Price},
        {644, ft_String},
        {645, ft_Price},
        {646, ft_Price},
        {647, ft_Qty},
        {648, ft_Qty},
        {649, ft_String},
        {650, ft_Boolean},
        {651, ft_Price},
        {652, ft_Qty},
        {654, ft_String},
        {655, ft_String},
        {656, ft_float},
        {657, ft_float},
        {658, ft_int},
        {659, ft_String},
        {0, ft_int}, // terminating 0 tag
};

const struct FIX_Tag fix43_std_data_tags[] =
{
        {89, ft_data},
        {91, ft_data},
        {96, ft_data},
        {213, ft_data},
        {349, ft_data},
        {351, ft_data},
        {353, ft_data},
        {355, ft_data},
        {357, ft_data},
        {359, ft_data},
        {361, ft_data},
        {363, ft_data},
        {365, ft_data},
        {446, ft_data},
        {619, ft_data},
        {622, ft_data},
        {0, ft_int}, // terminating 0 tag
};
