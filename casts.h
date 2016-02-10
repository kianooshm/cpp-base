// Copyright 2011 Google Inc. All Rights Reserved.
//
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

// Various Google-specific casting templates.
//
// This code is compiled directly on many platforms, including client
// platforms like Windows, Mac, and embedded systems.  Before making
// any changes here, make sure that you're not breaking any platforms.
//

#ifndef CPP_BASE_CASTS_H_
#define CPP_BASE_CASTS_H_

#include <assert.h>         // for use with down_cast<>
#include <stdio.h>

#include "cpp-base/macros.h"
#include "cpp-base/template_util.h"
#include "cpp-base/type_traits.h"

namespace cpp_base {

// Use implicit_cast as a safe version of static_cast or const_cast
// for implicit conversions. For example:
// - Upcasting in a type hierarchy.
// - Performing arithmetic conversions (int32 to int64, int to double, etc.).
// - Adding const or volatile qualifiers.
//
// In general, implicit_cast can be used to convert this code
//   To to = from;
//   DoSomething(to);
// to this
//   DoSomething(implicit_cast<To>(from));
//
// cpp_base::identity_ is used to make a non-deduced context, which
// forces all callers to explicitly specify the template argument.
template<typename To>
inline To implicit_cast(typename cpp_base::identity_<To>::type to) {
  return to;
}

// This version of implicit_cast is used when two template arguments
// are specified. It's obsolete and should not be used.
template<typename To, typename From>
inline To implicit_cast(typename cpp_base::identity_<From>::type const &f) {
  return f;
}

// When you upcast (that is, cast a pointer from type Foo to type
// SuperclassOfFoo), it's fine to use implicit_cast<>, since upcasts
// always succeed.  When you downcast (that is, cast a pointer from
// type Foo to type SubclassOfFoo), static_cast<> isn't safe, because
// how do you know the pointer is really of type SubclassOfFoo?  It
// could be a bare Foo, or of type DifferentSubclassOfFoo.  Thus,
// when you downcast, you should use this macro.  In debug mode, we
// use dynamic_cast<> to double-check the downcast is legal (we die
// if it's not).  In normal mode, we do the efficient static_cast<>
// instead.  Thus, it's important to test in debug mode to make sure
// the cast is legal!
//    This is the only place in the code we should use dynamic_cast<>.
// In particular, you SHOULDN'T be using dynamic_cast<> in order to
// do RTTI (eg code like this:
//    if (dynamic_cast<Subclass1>(foo)) HandleASubclass1Object(foo);
//    if (dynamic_cast<Subclass2>(foo)) HandleASubclass2Object(foo);
// You should design the code some other way not to need this.

template<typename To, typename From>     // use like this: down_cast<T*>(foo);
inline To down_cast(From* f) {                   // so we only accept pointers
  // Ensures that To is a sub-type of From *.  This test is here only
  // for compile-time type checking, and has no overhead in an
  // optimized build at run-time, as it will be optimized away
  // completely.

  // TODO(user): This should use COMPILE_ASSERT.
  if (false) {
    implicit_cast<From*, To>(NULL);
  }

  // uses RTTI in dbg and fastbuild. asserts are disabled in opt builds.
  assert(f == NULL || dynamic_cast<To>(f) != NULL);
  return static_cast<To>(f);
}

// Overload of down_cast for references. Use like this: down_cast<T&>(foo).
// The code is slightly convoluted because we're still using the pointer
// form of dynamic cast. (The reference form throws an exception if it
// fails.)
//
// There's no need for a special const overload either for the pointer
// or the reference form. If you call down_cast with a const T&, the
// compiler will just bind From to const T.
template<typename To, typename From>
inline To down_cast(From& f) {    // NOLINT
  COMPILE_ASSERT(cpp_base::is_reference<To>::value,
                 target_type_not_a_reference);
  typedef typename cpp_base::remove_reference<To>::type* ToAsPointer;
  if (false) {
    // Compile-time check that To inherits from From. See above for details.
    implicit_cast<From*, ToAsPointer>(NULL);
  }

  assert(dynamic_cast<ToAsPointer>(&f) != NULL);  // RTTI: debug mode only
  return static_cast<To>(f);
}

}  // namespace cpp_base

#endif  // CPP_BASE_CASTS_H_
