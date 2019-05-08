/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2019, Gary Guo
 *
 * This header defines an utility class for arrays with fixed, but runtime-determined dynamic size.
 * Once the object is constructed, the size cannot be changed.
 */

#ifndef TLBSIM_DYN_BITSET_H
#define TLBSIM_DYN_BITSET_H

#include <cstring>

namespace tlbsim {

class DynBitset {
public:
    class reference {
        uint8_t* _bit;
        size_t _pos;

    public:
        reference(DynBitset& set, size_t pos) noexcept {
            _bit = &set._bits[pos >> 3];
            _pos = pos & 7;
        }
        reference(const reference&) = default;

        reference& operator=(bool value) noexcept {
            if (value) *_bit |= 1 << _pos;
            else *_bit &= ~(1 << _pos);
            return *this;
        }

        reference& operator=(const reference& value) noexcept {
            return *this = (bool)value;
        }

        operator bool() const noexcept {
            return (*_bit & (1 << _pos)) != 0;
        }

        bool operator ~() const noexcept {
            return !*this;
        }

        reference& flip() noexcept {
            *_bit ^= 1 << _pos;
            return *this;
        }
    };
private:
    uint8_t* _bits;
    size_t _size;

public:
    explicit DynBitset(size_t count) {
        allocate(count);
        memset(_bits, 0, (_size + 7) / 8);
    }

    DynBitset(const DynBitset& other) {
        allocate(other.size());
        memcpy(_bits, other._bits, (_size + 7) / 8);
    }

    DynBitset(DynBitset&& other) noexcept {
        _bits = other._bits;
        _size = other._size;
        other._bits = NULL;
    }

    ~DynBitset() {
        // Already moved
        if (!_bits) return;
        delete[] _bits;
    }

    DynBitset& operator =(const DynBitset& other) {
        if (other._size != _size) {
            throw std::logic_error("Size of DynBitset cannot be changed in runtime");
        }
        memcpy(_bits, other._bits, (_size + 7) / 8);
        return *this;
    }

    DynBitset& operator =(DynBitset&& other) {
        if (other._size != _size) {
            throw std::logic_error("Size of DynBitset cannot be changed in runtime");
        }
        std::swap(_bits, other._bits);
        return *this;
    }

private:
    void allocate(size_t size) noexcept {
        _bits = new uint8_t[(size + 7) / 8];
        _size = size;
    }

public:

    /* Element access */
    reference operator[](size_t pos) { return {*this, pos}; }

    /* Capacity */
    size_t size() const noexcept { return _size; }
};

}

#endif // TLBSIM_DYN_BITSET_H
