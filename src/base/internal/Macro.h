// Copyright (C) 2020 THL A29 Limited, a Tencent company. All rights reserved.
//
// Licensed under the BSD 3-Clause License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of the
// License at
//
// https://opensource.org/licenses/BSD-3-Clause
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations under
// the License.

#ifndef FLARE_BASE_INTERNAL_MACRO_H_
#define FLARE_BASE_INTERNAL_MACRO_H_

// Let me be clear: I'm really bad at playing with macros. Hence, implementation
// below can be (very) slow. Here I only care about correctness. Even making
// them correct costs me plenty of time.

// Concatenate token.
#define FLARE_INTERNAL_PP_CAT(X, Y) FLARE_INTERNAL_DETAIL_PP_CAT_IMPL(X, Y)

// Stringize token. Both "stringize" and "stringify" seems common. FWIW, C++
// spec uses "stringize", so we use the same term here.
//
// Shamelessly copied from https://stackoverflow.com/a/3419392
#define FLARE_INTERNAL_PP_STRINGIZE(X) FLARE_INTERNAL_PP_STRINGIZE_IMPL(X)

// Count number of arguments given.
#define FLARE_INTERNAL_PP_COUNT(...)                                           \
  FLARE_INTERNAL_PP_COUNT_IMPL(                                                \
      __VA_ARGS__, 64, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, \
      49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32,  \
      31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14,  \
      13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1)

// Get i-th element from a sequence.
#define FLARE_INTERNAL_PP_SEQ_AT(Index, Sequence) \
  FLARE_INTERNAL_PP_SEQ_AT_##Index##_IMPL Sequence

// Call `Callback` on each element in `...`, `Context` is passed to `Callback`
// as-is.
#define FLARE_INTERNAL_PP_FOR_EACH(Callback, Context, ...)    \
  FLARE_INTERNAL_PP_CAT(FLARE_INTERNAL_PP_FOR_EACH_,          \
                        FLARE_INTERNAL_PP_COUNT(__VA_ARGS__)) \
  (Callback, Context, __VA_ARGS__)

// Implementation goes below.

// FLARE_INTERNAL_PP_CAT
#define FLARE_INTERNAL_DETAIL_PP_CAT_IMPL_2(X, Y) X##Y
#define FLARE_INTERNAL_DETAIL_PP_CAT_IMPL(X, Y) \
  FLARE_INTERNAL_DETAIL_PP_CAT_IMPL_2(X, Y)

// FLARE_INTERNAL_PP_STRINGIZE
#define FLARE_INTERNAL_PP_STRINGIZE_IMPL(X) #X

// FLARE_INTERNAL_PP_COUNT
#define FLARE_INTERNAL_PP_COUNT_IMPL(                                          \
    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, \
    _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, \
    _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, \
    _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, \
    _62, _63, Count, ...)                                                      \
  Count

// Other implementations are automatically generated by `macro-inl_gen.py`.
#include "MacroInl.h"

#endif  // FLARE_BASE_INTERNAL_MACRO_H_
