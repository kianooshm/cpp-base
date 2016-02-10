#ifndef CPP_BASE_UTIL_TRIPLE_H_
#define CPP_BASE_UTIL_TRIPLE_H_

namespace cpp_base {

// A simple, handy class that simply extends std::pair<first, second> to a
// triple<first, second, third>. For more elements, consider std::tuple.
template<class T1, class T2, class T3>
struct triple {
    triple() : first(), second(), third() { }

    triple(const T1& __first, const T2& __second, const T3& __third)
            : first(__first), second(__second), third(__third) { }

    triple(const triple<T1, T2, T3>& t)
            : first(t.first), second(t.second), third(t.third) { }

    triple<T1, T2, T3>& operator = (const triple<T1, T2, T3>& t) {
        first = t.first;
        second = t.second;
        third = t.third;
        return *this;
    }

    inline bool operator == (const triple<T1, T2, T3>& t) const {
        return (first == t.first) && (second == t.second) && (third == t.third);
    }

    inline bool operator != (const triple<T1, T2, T3>& t) const {
        return (first != t.first) || (second != t.second) || (third != t.third);
    }

    inline bool operator < (const triple<T1, T2, T3>& t) const {
        if (first < t.first) {
            return true;
        } else if (first > t.first) {
            return false;
        } else if (second < t.second) {
            return true;
        } else if (second > t.second) {
            return false;
        }
        return (third < t.third);
    }

    T1 first;
    T2 second;
    T3 third;
};

// "triple"-equivalent of std::make_pair.
template<class T1, class T2, class T3>
inline triple<T1, T2, T3> make_triple(const T1& a, const T2& b, const T3& c) {
    return triple<T1, T2, T3>(a, b, c);
}

}  // namespace cpp_base

namespace __gnu_cxx {
// Enable use of "triple" as hash_map key.
template<class T1, class T2, class T3>
struct hash< cpp_base::triple<T1, T2, T3> > {
    size_t operator()(const cpp_base::triple<T1, T2, T3>& t) const {
        return hash<T1>()(t.first) ^ hash<T2>()(t.second) ^ hash<T3>()(t.third);
    }
};
}  // __gnu_cxx

#endif  // CPP_BASE_UTIL_TRIPLE_H_
