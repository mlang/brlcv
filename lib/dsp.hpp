#if !defined(BrlCV_DSP_HPP)
#define BrlCV_DSP_HPP

#include <bitset>
#include <gsl/gsl>

namespace BrlCV {

// https://en.wikipedia.org/wiki/Moving_average#Exponential_moving_average
template<typename T>
class ExponentiallyWeightedMovingAverage {
  T Average;
  T const Weight, OneMinusWeight;

public:
  explicit ExponentiallyWeightedMovingAverage(T Weight, T Initial = T(0))
  : Average(Initial), Weight(Weight), OneMinusWeight(T(1) - Weight) {}

  ExponentiallyWeightedMovingAverage &operator()(T Value) {
    Average = Value * Weight + Average * OneMinusWeight;

    return *this;
  }

  operator T() const noexcept { return Average; }
};

template<typename T> using EWMA = ExponentiallyWeightedMovingAverage<T>;

template<std::size_t N>
class FairSegmentation {
  std::size_t SegmentSize{0};
  std::bitset<N> Set;

public:
  FairSegmentation() : Set(0) {}
  explicit FairSegmentation(std::size_t Size) : SegmentSize(Size / N), Set(0) {
    auto const R = Size % N;
    for (auto I = 0; I < R; ++I) {
      Set.set(N*I/R);
    }
    Ensures(Set.count() == R);
  }

  FairSegmentation &operator=(std::size_t Size) {
    SegmentSize = Size / N;
    Set.reset();
    auto const R = Size % N;
    for (auto I = 0; I < R; ++I) {
      Set.set(N*I/R);
    }
    Ensures(Set.count() == R);
  }

  constexpr bool empty() const noexcept { return N == 0; }
  constexpr std::size_t size() const noexcept { return N; }
  
  std::size_t operator[](std::size_t Position) const {
    Expects(Position < N);
    return SegmentSize + Set.test(Position);
  }
};

} // namespace BrlCV

#endif // BrlCV_DSP_HPP
