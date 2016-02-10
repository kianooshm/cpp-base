// Copyright 2010-2014 Google
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef CPP_BASE_UTIL_MATH_UTIL_H_
#define CPP_BASE_UTIL_MATH_UTIL_H_

#include "cpp-base/util/math_limits.h"
#include <glog/logging.h>
#include <math.h>
#include <algorithm>
#include <vector>

#include "cpp-base/integral_types.h"
#include "cpp-base/macros.h"

namespace cpp_base {

class MathUtil {
 public:
  // Largest of two values.
  // Works correctly for special floating point values.
  // Note: 0.0 and -0.0 are not differentiated by Max (Max(0.0, -0.0) is -0.0),
  // which should be OK because, although they (can) have different
  // bit representation, they are observably the same when examined
  // with arithmetic and (in)equality operators.
  template<typename T>
  static T Max(const T x, const T y) {
    return MathLimits<T>::IsNaN(x) || x > y ? x : y;
  }

  // Smallest of two values
  // Works correctly for special floating point values.
  // Note: 0.0 and -0.0 are not differentiated by Min (Min(-0.0, 0.0) is 0.0),
  // which should be OK: see the comment for Max above.
  template<typename T>
  static T Min(const T x, const T y) {
    return MathLimits<T>::IsNaN(x) || x < y ? x : y;
  }

  // Absolute value of x
  // Works correctly for unsigned types and
  // for special floating point values.
  // Note: 0.0 and -0.0 are not differentiated by Abs (Abs(0.0) is -0.0),
  // which should be OK: see the comment for Max above.
  template<typename T>
  static T Abs(const T x) {
    return x > T(0) ? x : -x;
  }

  // The sign of x
  // (works for unsigned types and special floating point values as well):
  //   -1 if x < 0,
  //   +1 if x > 0,
  //    0 if x = 0.
  //  nan if x is nan.
  template<typename T>
  static T Sign(const T x) {
    return MathLimits<T>::IsNaN(x) ? x :
        (x == T(0) ? T(0) : (x > T(0) ? T(1) : T(-1)));
  }

  // Returns the square of x
  template <typename T>
  static T Square(const T x) {
    return x * x;
  }

  // Absolute value of the difference between two numbers.
  // Works correctly for signed types and special floating point values.
  template<typename T>
  static typename MathLimits<T>::UnsignedType AbsDiff(const T x, const T y) {
    return x > y ? x - y : y - x;
  }

  // CAVEAT: Floating point computation instability for x86 CPUs
  // can frequently stem from the difference of when floating point values
  // are transferred from registers to memory and back,
  // which can depend simply on the level of optimization.
  // The reason is that the registers use a higher-precision representation.
  // Hence, instead of relying on approximate floating point equality below
  // it might be better to reorganize the code with volatile modifiers
  // for the floating point variables so as to control when
  // the loss of precision occurs.

  // If two (usually floating point) numbers are within a certain
  // absolute margin of error.
  // NOTE: this "misbehaves" is one is trying to capture provisons for errors
  // that are relative, i.e. larger if the numbers involved are larger.
  // Consider using WithinFraction or WithinFractionOrMargin below.
  //
  // This and other Within* NearBy* functions below
  // work correctly for signed types and special floating point values.
  template<typename T>
  static bool WithinMargin(const T x, const T y, const T margin) {
    DCHECK_GE(margin, 0);
    // this is a little faster than x <= y + margin  &&  x >= y - margin
    return AbsDiff(x, y) <= margin;
  }

  // If two (usually floating point) numbers are within a certain
  // fraction of their magnitude.
  // CAVEAT: Although this works well if computation errors are relative
  // both for large magnitude numbers > 1 and for small magnitude numbers < 1,
  // zero is never within a fraction of any
  // non-zero finite number (fraction is required to be < 1).
  template<typename T>
  static bool WithinFraction(const T x, const T y, const T fraction);

  // If two (usually floating point) numbers are within a certain
  // fraction of their magnitude or within a certain absolute margin of error.
  // This is the same as the following but faster:
  //   WithinFraction(x, y, fraction)  ||  WithinMargin(x, y, margin)
  // E.g. WithinFraction(0.0, 1e-10, 1e-5) is false but
  //      WithinFractionOrMargin(0.0, 1e-10, 1e-5, 1e-5) is true.
  template<typename T>
  static bool WithinFractionOrMargin(const T x, const T y,
                                     const T fraction, const T margin);

  // NearBy* functions below are geared as replacements for CHECK_EQ()
  // over floating-point numbers.

  // Same as WithinMargin(x, y, MathLimits<T>::kStdError)
  // Works as == for integer types.
  template<typename T>
  static bool NearByMargin(const T x, const T y) {
    return AbsDiff(x, y) <= MathLimits<T>::kStdError;
  }

  // Same as WithinFraction(x, y, MathLimits<T>::kStdError)
  // Works as == for integer types.
  template<typename T>
  static bool NearByFraction(const T x, const T y) {
    return WithinFraction(x, y, MathLimits<T>::kStdError);
  }

  // Same as WithinFractionOrMargin(x, y, MathLimits<T>::kStdError,
  //                                      MathLimits<T>::kStdError)
  // Works as == for integer types.
  template<typename T>
  static bool NearByFractionOrMargin(const T x, const T y) {
    return WithinFractionOrMargin(x, y, MathLimits<T>::kStdError,
                                        MathLimits<T>::kStdError);
  }

  // Tests whether two values are close enough to each other to be considered
  // equal. This function is intended to be used mainly as a replacement for
  // equality tests of floating point values in CHECK()s, and as a replacement
  // for equality comparison against golden files. It is the same as == for
  // integer types. The purpose of AlmostEquals() is to avoid false positive
  // error reports in unit tests and regression tests due to minute differences
  // in floating point arithmetic (for example, due to a different compiler).
  //
  // We cannot use simple equality to compare floating point values
  // because floating point expressions often accumulate inaccuracies, and
  // new compilers may introduce further variations in the values.
  //
  // Two values x and y are considered "almost equals" if:
  // (a) Both values are very close to zero: x and y are in the range
  //     [-standard_error, standard_error]
  //     Normal calculations producing these values are likely to be dealing
  //     with meaningless residual values.
  // -or-
  // (b) The difference between the values is small:
  //     abs(x - y) <= standard_error
  // -or-
  // (c) The values are finite and the relative difference between them is
  //     small:
  //     abs (x - y) <= standard_error * max(abs(x), abs(y))
  // -or-
  // (d) The values are both positive infinity or both negative infinity.
  //
  // Cases (b) and (c) are the same as MathUtils::NearByFractionOrMargin(x, y),
  // for finite values of x and y.
  //
  // standard_error is the corresponding MathLimits<T>::kStdError constant.
  // It is equivalent to 5 bits of mantissa error. See
  //
  // Caveat:
  // AlmostEquals() is not appropriate for checking long sequences of
  // operations where errors may cascade (like extended sums or matrix
  // computations), or where significant cancellation may occur
  // (e.g., the expression (x+y)-x, where x is much larger than y).
  // Both cases may produce errors in excess of standard_error.
  // In any case, you should not test the results of calculations which have
  // not been vetted for possible cancellation errors and the like.
  template<typename T>
  static bool AlmostEquals(const T x, const T y) {
    if (x == y)  // Covers +inf and -inf, and is a shortcut for finite values.
      return true;
    if (!MathLimits<T>::IsFinite(x) || !MathLimits<T>::IsFinite(y))
      return false;

    if (MathUtil::Abs<T>(x) <= MathLimits<T>::kStdError &&
        MathUtil::Abs<T>(y) <= MathLimits<T>::kStdError)
      return true;

    return MathUtil::NearByFractionOrMargin<T>(x, y);
  }

  // CeilOfRatio<IntegralType>
  // FloorOfRatio<IntegralType>
  //   Returns the ceil (resp. floor) of the ratio of two integers.
  //
  //   IntegralType: any integral type, whether signed or not.
  //   numerator: any integer: positive, negative, or zero.
  //   denominator: a non-zero integer, positive or negative.
  template <typename IntegralType>
  static IntegralType CeilOfRatio(IntegralType numerator,
                                  IntegralType denominator) {
    DCHECK_NE(0, denominator);
    const IntegralType rounded_toward_zero = numerator / denominator;
    const IntegralType intermediate_product = rounded_toward_zero * denominator;
    const bool needs_adjustment =
        (rounded_toward_zero >= 0) &&
        ((denominator > 0 && numerator > intermediate_product) ||
         (denominator < 0 && numerator < intermediate_product));
    const IntegralType adjustment = static_cast<IntegralType>(needs_adjustment);
    const IntegralType ceil_of_ratio = rounded_toward_zero + adjustment;
    return ceil_of_ratio;
  }
  template <typename IntegralType>
  static IntegralType FloorOfRatio(IntegralType numerator,
                                   IntegralType denominator) {
    DCHECK_NE(0, denominator);
    const IntegralType rounded_toward_zero = numerator / denominator;
    const IntegralType intermediate_product = rounded_toward_zero * denominator;
    const bool needs_adjustment =
        (rounded_toward_zero <= 0) &&
        ((denominator > 0 && numerator < intermediate_product) ||
         (denominator < 0 && numerator > intermediate_product));
    const IntegralType adjustment = static_cast<IntegralType>(needs_adjustment);
    const IntegralType floor_of_ratio = rounded_toward_zero - adjustment;
    return floor_of_ratio;
  }

  // Returns the greatest common divisor of two unsigned integers x and y
  static unsigned int GCD(unsigned int x, unsigned int y) {
    while (y != 0) {
      unsigned int r = x % y;
      x = y;
      y = r;
    }
    return x;
  }

  // Returns the least common multiple of two unsigned integers.  Returns zero
  // if either is zero.
  static unsigned int LeastCommonMultiple(unsigned int a, unsigned int b) {
    if (a > b) {
      return (a / MathUtil::GCD(a, b)) * b;
    } else if (a < b) {
      return (b / MathUtil::GCD(b, a)) * a;
    } else {
      return a;
    }
  }

  // Euclid's Algorithm.
  // Returns: the greatest common divisor of two unsigned integers x and y
  static int64 GCD64(int64 x, int64 y) {
    DCHECK_GE(x, 0);
    DCHECK_GE(y, 0);
    while (y != 0) {
      int64 r = x % y;
      x = y;
      y = r;
    }
    return x;
  }
};

template<typename T>
bool MathUtil::WithinFraction(const T x, const T y, const T fraction) {
  // not just "0 <= fraction" to fool the compiler for unsigned types
  DCHECK((0 < fraction || 0 == fraction)  &&  fraction < 1);

  // Template specialization will convert the if() condition to a constant,
  // which will cause the compiler to generate code for either the "if" part
  // or the "then" part.  In this way we avoid a compiler warning
  // about a potential integer overflow in crosstool v12 (gcc 4.3.1).
  if (MathLimits<T>::kIsInteger) {
    return x == y;
  } else {
    // IsFinite checks are to make kPosInf and kNegInf not within fraction
    return (MathLimits<T>::IsFinite(x) || MathLimits<T>::IsFinite(y)) &&
           (AbsDiff(x, y) <= fraction * Max(Abs(x), Abs(y)));
  }
}

template<typename T>
bool MathUtil::WithinFractionOrMargin(const T x, const T y,
                                      const T fraction, const T margin) {
  // Not just "0 <= fraction" to fool the compiler for unsigned types.
  DCHECK((T(0) < fraction || T(0) == fraction) &&
         fraction < T(1) &&
         margin >= T(0));

  // Template specialization will convert the if() condition to a constant,
  // which will cause the compiler to generate code for either the "if" part
  // or the "then" part.  In this way we avoid a compiler warning
  // about a potential integer overflow in crosstool v12 (gcc 4.3.1).
  if (MathLimits<T>::kIsInteger) {
    return x == y;
  } else {
    // IsFinite checks are to make kPosInf and kNegInf not within fraction
    return (MathLimits<T>::IsFinite(x) || MathLimits<T>::IsFinite(y)) &&
           (AbsDiff(x, y) <= Max(margin, fraction * Max(Abs(x), Abs(y))));
  }
}

}  // namespace cpp_base

#endif  // CPP_BASE_UTIL_MATH_UTIL_H_
