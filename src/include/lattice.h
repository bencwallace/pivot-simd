#pragma once

#include <algorithm>
#include <immintrin.h>
#include <limits>
#include <random>
#include <span>
#include <string>
#include <vector>

#include <boost/operators.hpp>

static inline uint32_t extract_epi32(__m128i data, size_t i) {
  switch (i) {
  case 0:
    return _mm_extract_epi32(data, 0);
  case 1:
    return _mm_extract_epi32(data, 1);
  case 2:
    return _mm_extract_epi32(data, 2);
  case 3:
    return _mm_extract_epi32(data, 3);
  default:
    return 0;
  }
}

static inline __m128i insert_epi32(__m128i data, int val, size_t i) {
  switch (i) {
  case 0:
    return _mm_insert_epi32(data, val, 0);
  case 1:
    return _mm_insert_epi32(data, val, 1);
  case 2:
    return _mm_insert_epi32(data, val, 2);
  case 3:
    return _mm_insert_epi32(data, val, 3);
  default:
    return data;
  }
}

inline __m128i permutevar_epi32(__m128i data, __m128i perm) {
  return _mm_castps_si128(_mm_permutevar_ps(_mm_castsi128_ps(data), perm));
}

// Given a vector (a, b, c, d) representing potentially unsorted intervals
// {a, c} and {b, d}, return [min(a, c), min(b, d), max(a, c), max(b, d)]
// representing intervals [min(a, c), max(a, c)] and [min(b, d), max(b, d)].
inline __m128i sort_bounds(__m128i pairs) {
  __m128i swapped = _mm_shuffle_epi32(pairs, _MM_SHUFFLE(1, 0, 3, 2));
  __m128i minima = _mm_min_epi32(pairs, swapped);
  __m128i maxima = _mm_max_epi32(pairs, swapped);
  return _mm_blend_epi32(minima, maxima, 0b1100);
}

namespace pivot {

template <int Dim> struct box;

/**
 * @brief Represents a closed interval [left_, right_].
 *
 * If left_ > right_, the interval is empty.
 */
struct interval {
  int left_;
  int right_;

  interval();
  interval(int left, int right);

  bool operator==(const interval &i) const;

  bool operator!=(const interval &i) const;

  bool empty() const;

  std::string to_string() const;
};

template <int Dim> class point : boost::multipliable<point<Dim>, int> {

public:
  point();

  point(__m128i coords);

  point(const std::array<int, Dim> &coords);

  static point unit(int i);

  __m128i data() const { return coords_; }

  int operator[](int i) const;

  bool operator==(const point &p) const;

  bool operator!=(const point &p) const;

  point operator+(const point &p) const;

  point operator-(const point &p) const;

  point &operator*=(int k);

  int norm() const;

  std::string to_string() const;

private:
  __m128i coords_;
};

template <typename S, typename T, T Dim> point(std::array<S, Dim>) -> point<Dim>;

template <int Dim> struct box : boost::additive<box<Dim>, point<Dim>> {
  __m128i data_; // x_min, y_min, x_max, y_max

  box() = delete;

  box(__m128i data) : data_(data) {}

  box(const std::array<interval, Dim> &intervals);

  box(std::span<const point<Dim>> points);

  __m128i data() const { return data_; }

  std::array<interval, 2> intervals() const {
    return {interval{static_cast<int>(extract_epi32(data_, 0)), static_cast<int>(extract_epi32(data_, 2))},
            interval{static_cast<int>(extract_epi32(data_, 1)), static_cast<int>(extract_epi32(data_, 3))}};
  }

  bool operator==(const box &b) const;

  bool operator!=(const box &b) const;

  interval operator[](int i) const;

  bool empty() const;

  box<Dim> &operator+=(const point<Dim> &b);

  box<Dim> &operator-=(const point<Dim> &b);

  box operator|(const box &b) const;

  box operator&(const box &b) const;

  std::string to_string() const;
};

template <typename S, typename T, S Dim, T N> box(std::array<point<Dim>, N>) -> box<Dim>;

template <int Dim> class transform {

public:
  transform();

  transform(const std::array<int, Dim> &perm, const std::array<int, Dim> &signs);

  transform(const point<Dim> &p, const point<Dim> &q);

  template <typename Gen> static transform rand(Gen &gen) {
    static std::bernoulli_distribution flip_;

    std::array<int, Dim> perm;
    std::array<int, Dim> signs;
    for (int i = 0; i < Dim; ++i) {
      perm[i] = i;
      signs[i] = 2 * flip_(gen) - 1;
    }
    std::shuffle(perm.begin(), perm.end(), gen);
    return transform(perm, signs);
  }

  static transform rand();

  bool operator==(const transform &t) const;

  point<Dim> operator*(const point<Dim> &p) const;

  transform operator*(const transform &t) const;

  box<Dim> operator*(const box<Dim> &b) const;

  bool is_identity() const;

  transform inverse() const;

  std::array<std::array<int, Dim>, Dim> to_matrix() const;

  std::string to_string() const;

private:
  __m128i perm_;
  __m128i signs_;

  transform(__m128i perm, __m128i signs) : perm_(perm), signs_(signs) {}
};

} // namespace pivot
