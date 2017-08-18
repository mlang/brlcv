#if !defined(BrlCV_IMPL_PTR)
#define BrlCV_IMPL_PTR

#include <experimental/propagate_const>
#include <memory>

namespace BrlCV {

template<typename> struct impl_ptr {
  struct implementation;
  template<typename> class base;

  using shared = base<std::shared_ptr<implementation>>;
  using unique = base<std::unique_ptr<implementation>>;
};

template<typename Derived> template<typename Pointer> class impl_ptr<Derived>::base {
  std::experimental::propagate_const<Pointer> impl;
  template<typename T> struct is_shared_ptr : std::false_type {};
  template<typename T> struct is_shared_ptr<std::shared_ptr<T>> : std::true_type {};
  template<typename T> struct is_unique_ptr : std::false_type {};
  template<typename T, typename D>
  struct is_unique_ptr<std::unique_ptr<T, D>> : std::true_type {};
  static constexpr auto shared = is_shared_ptr<Pointer>::value,
                        unique = is_unique_ptr<Pointer>::value;
protected:
  using implementation = typename impl_ptr<Derived>::implementation;
  using impl_ptr = base;
  template<typename... Args> explicit base(Args&&... args) : impl {
    [&args...] {
      if constexpr (shared) {
        return std::make_shared<implementation>(std::forward<Args>(args)...);
      }
      if constexpr (unique) {
        return std::make_unique<implementation>(std::forward<Args>(args)...);
      }
    }()
  }
  {}
  implementation       *operator->()       { return impl.get(); }
  implementation const *operator->() const { return impl.get(); }
  implementation       &operator*()       { return *impl; }
  implementation const &operator*() const { return *impl; }

public:
  void swap(Derived& other) { impl.swap(other.impl); }
};

} // namespace BrlCV

#endif // BrlCV_IMPL_PTR
