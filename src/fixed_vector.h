#pragma once
#include <stdint.h>
#include <stddef.h>

template<typename T, size_t Size>
class FixedVector {
    T _data[Size];
    size_t _len;

public:
    FixedVector(): _len{0}, _data{0} {}
    //i could make an initialiser list constructor but i don't need to use it so i won't

    const T* begin() const noexcept { return _data; }
    const T* end() const noexcept { return _data + _len; }

    size_t size() const { return _len; }
    bool empty() const { return _len == 0; }

    T& operator[](size_t i) { return _data[i]; }
    const T& operator[](size_t i) const { return _data[i]; }


    /// @brief 
    /// @param x 
    /// @return if no space is left, return false
    bool push_back(const T& x) {
        if (_len +1 >= Size) return false;
        _data[_len++] = x;
        return true;
    }

    /// @brief 
    /// @param ptr 
    /// @param len 
    /// @return if no space is left, return false
    bool push_n(const T* ptr, size_t len) {
        if (_len + len >= Size) return false;

        for (auto i = 0; i < len; ++i) {
            _data[_len+i] = ptr[i];
        }
        _len += len;
        return true;
    } 

    /// @brief 
    /// @return returns the default T object if no elements are present
    /// You probablly don't want that, so check before popping
    T pop_back() {
        if (_len == 0) return T{};
        return _data[--_len];
    }

    /// @brief 
    /// @return returns the default T object if no elements are present
    /// You probablly don't want that, so check before popping
    T pop_front() {
        if (_len == 0) return T{};

        T res = _data[0];
        _len--;

        for (auto i = 0; i < _len; ++i) {
            _data[i] = _data[i+1];
        }
        return res;
    }
};