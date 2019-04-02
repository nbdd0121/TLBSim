/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2019, Gary Guo
 *
 * This header defines an utility class for arrays with fixed, but runtime-determined dynamic size.
 * Once the object is constructed, the size cannot be changed.
 */

#ifndef TLBSIM_DYN_ARRAY_H
#define TLBSIM_DYN_ARRAY_H

#include <memory>
#include <initializer_list>
#include <iterator>

namespace tlbsim {

template<typename T>
class DynArray {
private:
    using storage = std::aligned_storage_t<sizeof(T), alignof(T)>;
public:
    using size_type = size_t;
    using difference_type = std::ptrdiff_t;
    using value_type = T;
    using iterator = T*;
    using const_iterator = const T*;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    using reference = T&;
    using const_reference = const T&;
private:
    T* _begin;
    size_t _size;

public:
    explicit DynArray(size_type count) {
        allocate(count);
        std::uninitialized_default_construct_n(_begin, count);
    }

    explicit DynArray(size_type count, const T& value) {
        allocate(count);
        std::uninitialized_fill_n(_begin, count, value);
    }

    DynArray(const DynArray& other) {
        allocate(other.size());
        std::uninitialized_copy(other.begin(), other.end(), _begin);
    }

    DynArray(std::initializer_list<T> init) {
        allocate(init.size());
        std::uninitialized_move(init.begin(), init.end(), _begin);
    }

    DynArray(DynArray&& other) noexcept {
        _begin = other._begin;
        _size = other._size;
        other._begin = NULL;
    }

    ~DynArray() {
        // Already moved
        if (!_begin) return;
        std::destroy(_begin, _begin + _size);
        delete[] (storage*)_begin;
    }

    DynArray& operator =(const DynArray& other) {
        if (other._size != _size) {
            throw std::logic_error("Size of DynArray cannot be changed in runtime");
        }
        for (size_t i = 0; i < _size; i++) {
            _begin[i] = other._begin[i];
        }
    }

    DynArray& operator =(DynArray&& other) {
        if (other._size != _size) {
            throw std::logic_error("Size of DynArray cannot be changed in runtime");
        }
        std::swap(_begin, other._begin);
        return *this;
    }

private:
    void allocate(size_type size) noexcept {
        _begin = reinterpret_cast<T*>(new storage[size]);
        _size = size;
    }

public:

    /* Element access */
    reference operator[](size_type pos) { return _begin[pos]; }
    const_reference operator[](size_type pos) const { return _begin[pos]; }
    reference front() { return *_begin; }
    const_reference front() const { return *_begin; }
    reference back() { return *(_begin + (_size - 1)); }
    const_reference back() const { return *(_begin + (_size - 1)); }
    T* data() noexcept { return _begin; }
    const T* data() const noexcept { return _begin; }

    /* Iterators */
    iterator begin() noexcept { return _begin; }
    const_iterator begin() const noexcept { return _begin; }
    const_iterator cbegin() const noexcept { return _begin; }
    iterator end() noexcept { return _begin + _size; }
    const_iterator end() const noexcept { return _begin + _size; }
    const_iterator cend() const noexcept { return _begin + _size; }
    reverse_iterator rbegin() noexcept { return reverse_iterator(_begin + _size); }
    const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(_begin + _size); }
    const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(_begin + _size); }
    reverse_iterator rend() noexcept { return reverse_iterator(_begin); }
    const_reverse_iterator rend() const noexcept { return const_reverse_iterator(_begin + _size); }
    const_reverse_iterator crend() const noexcept { return const_reverse_iterator(_begin + _size); }

    /* Capacity */
    bool empty() const noexcept { return _size != 0; }
    size_type size() const noexcept { return _size; }
    size_type max_size() const { return _size; }
};

}

#endif // TLBSIM_DYN_ARRAY_H
