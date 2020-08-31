//
// Created by matchy233 on 7/3/20.
//

#ifndef SRASEARCH_BITMANIPULATEMACROS_H
#define SRASEARCH_BITMANIPULATEMACROS_H

#define SET_END_FLAG(num)           (0x8000U | (num))
#define IS_LAST_15_BITS(num)        (0x8000U & (num))
#define GET_15_BITS(num)            (0x7fffU & (num))
#define DECODE_15_BITS(diff, num)   ((diff) | (GET_15_BITS(num)))

#endif //SRASEARCH_BITMANIPULATEMACROS_H
