#pragma once

// Like std::unique_ptr, but doesn't strictly assume pointers
// and takes the deleter as an auto-typed template value parameter.
// This makes it way easier to naturally wrap at least Windows APIs.
template <typename T, auto deleter, T default_value = nullptr>
struct Unique {
  T value = default_value;
  Unique() = default;
  Unique(T value) : value{value} {}
  ~Unique() { deleter(value); }
  Unique(const Unique&) = delete;
  Unique& operator=(const Unique&) = delete;
  Unique(Unique&& other) { assign(other.release()); }
  Unique& operator=(Unique&& other) {
    assign(other.release());
    return *this;
  }

  operator T() const { return value; }
  Unique& operator=(T new_value) {
    assign(new_value);
    return *this;
  }

  T release() { return std::exchange(value, default_value); }

  void assign(T new_value) {
    if (value != new_value) {
      deleter(std::exchange(value, new_value));
    }
  }
};