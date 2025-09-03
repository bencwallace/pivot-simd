#include <boost/preprocessor/repetition/repeat_from_to.hpp>

#include "lattice.h"

namespace pivot {

template <> int point<2>::operator[](int i) const;

interval::interval() : interval(0, 0) {}

interval::interval(int left, int right) : left_(left), right_(right) {}

bool interval::operator==(const interval &other) const {
  return (left_ == other.left_ && right_ == other.right_) || (empty() && other.empty());
}

bool interval::operator!=(const interval &other) const { return !(*this == other); }

bool interval::empty() const { return left_ > right_; }

std::string interval::to_string() const {
  std::string result = "[";
  result.append(std::to_string(left_)).append(", ").append(std::to_string(right_)).append("]");
  return result;
}

template <> box<2> &box<2>::operator+=(const point<2> &p) {
  __m128i offset = _mm_shuffle_epi32(p.data(), _MM_SHUFFLE(1, 0, 1, 0));
  data_ = _mm_add_epi32(data_, offset);
  return *this;
}

template <> box<2> &box<2>::operator-=(const point<2> &b) {
  __m128i offset = _mm_shuffle_epi32(b.data(), _MM_SHUFFLE(1, 0, 1, 0));
  data_ = _mm_sub_epi32(data_, offset);
  return *this;
}

template <>
box<2>::box(const std::array<interval, 2> &intervals)
    : data_(_mm_setr_epi32(intervals[0].left_, intervals[1].left_, intervals[0].right_, intervals[1].right_)) {}

template <> box<2>::box(std::span<const point<2>> points) {
  std::array<int, 2> min;
  std::array<int, 2> max;
  min.fill(std::numeric_limits<int>::max());
  max.fill(std::numeric_limits<int>::min());
  for (const auto &p : points) {
    for (int i = 0; i < 2; ++i) { // TODO: vectorize (not urgent)
      min[i] = std::min(min[i], p[i]);
      max[i] = std::max(max[i], p[i]);
    }
  }
  // anchor at (1, 0, ..., 0)
  data_ = insert_epi32(data_, min[0] - points[0][0] + 1, 0);
  data_ = insert_epi32(data_, max[0] - points[0][0] + 1, 2);
  data_ = insert_epi32(data_, min[1] - points[0][1], 1);
  data_ = insert_epi32(data_, max[1] - points[0][1], 3);
}

template <> bool box<2>::operator==(const box &b) const {
  return _mm_movemask_epi8(_mm_cmpeq_epi32(data_, b.data_)) == 0xFFFF;
}

template <> bool box<2>::operator!=(const box &b) const { return !(*this == b); }

template <> interval box<2>::operator[](int i) const {
  return {int32_t(extract_epi32(data_, i)), int32_t(extract_epi32(data_, i + 2))};
}

template <> bool box<2>::empty() const {
  __m128i swapped = _mm_shuffle_epi32(data_, _MM_SHUFFLE(1, 0, 3, 2));
  __m128i cmp = _mm_cmpgt_epi32(data_, swapped);
  auto result = _mm_cvtsi128_si64(cmp); // Only check the lower 64 bits
  return result != 0;
}

template <> box<2> box<2>::operator|(const box<2> &b) const {
  // TODO: Look into using `_mm_maskz_{min,max}_epi32`
  // For example: `__m128i mins = _mm_min_epi32(0b0011, data, b.data)`
  // For now, benchmarks are crashing when I enable AVX512 in the build
  __m128i mins = _mm_min_epi32(data_, b.data_);
  __m128i maxs = _mm_max_epi32(data_, b.data_);
  return _mm_blend_epi32(mins, maxs, 0b1100);
}

template <> box<2> box<2>::operator&(const box<2> &b) const {
  // TODO: See comment under `operator|`
  __m128i mins = _mm_min_epi32(data_, b.data_);
  __m128i maxs = _mm_max_epi32(data_, b.data_);
  return _mm_blend_epi32(mins, maxs, 0b0011);
}

template <> std::string box<2>::to_string() const {
  std::string s = "";
  s += interval(extract_epi32(data_, 0), extract_epi32(data_, 2)).to_string();
  s += " x ";
  s += interval(extract_epi32(data_, 1), extract_epi32(data_, 3)).to_string();
  return s;
}

} // namespace pivot
