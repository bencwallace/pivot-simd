#include <boost/preprocessor/repetition/repeat_from_to.hpp>

#include "lattice.h"

namespace pivot {

template <> point<2>::point() : coords_(_mm_setzero_si128()) {}

template <> point<2>::point(__m128i coords) : coords_(coords) {}

template <> point<2>::point(const std::array<int, 2> &coords) : coords_(_mm_setr_epi32(coords[0], coords[1], 0, 0)) {}

template <> point<2> point<2>::unit(int i) {
  point p;
  p.coords_ = insert_epi32(p.coords_, 1, i);
  return p;
}

template <> int point<2>::operator[](int i) const { return extract_epi32(coords_, i); }

template <> bool point<2>::operator==(const point &p) const {
  return _mm_movemask_epi8(_mm_cmpeq_epi32(coords_, p.coords_)) == 0xFFFF;
}

template <> bool point<2>::operator!=(const point &p) const { return !(*this == p); }

template <> point<2> point<2>::operator+(const point<2> &p) const { return _mm_add_epi32(coords_, p.coords_); }

template <> point<2> point<2>::operator-(const point &p) const { return _mm_sub_epi32(coords_, p.coords_); }

template <> point<2> &point<2>::operator*=(int k) {
  coords_ = _mm_mullo_epi32(coords_, _mm_set1_epi32(k));
  return *this;
}

// template <> int point<2>::norm() const;

template <> std::string point<2>::to_string() const {
  std::string s = "(";
  s += std::to_string(extract_epi32(coords_, 0));
  s += ", ";
  s += std::to_string(extract_epi32(coords_, 1));
  s += ")";
  return s;
}

} // namespace pivot
