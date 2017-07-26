#if !defined(BrlCV_DSP_HPP)
#define BrlCV_DSP_HPP

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

} // namespace BrlCV

#endif // BrlCV_DSP_HPP
