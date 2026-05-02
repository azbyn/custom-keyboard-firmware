#pragma once
#include <stdint.h>

template<size_t Size>
class BitArray {
    uint8_t _data[(Size+7)/8] = {};

public:
    BitArray() {}

    static constexpr size_t actualCapacity() { return sizeof(_data) * 8; }
    static constexpr size_t sizeBytes() { return sizeof(_data); }
    static constexpr size_t sizeBits() { return Size; }

    constexpr const uint8_t* data() const { return _data; }


    bool get(size_t i) const {
        if (i > actualCapacity()) return false;// shouldn't happen

        size_t byte_i = i / 8;
        return _data[byte_i] & (1 << (i % 8));
    }
    void set(size_t i, bool state) {
        if (i > actualCapacity()) return;// shouldn't happen
        size_t byte_i = i / 8;
        uint8_t mask = (1 << (i % 8));

        this->_data[byte_i] = ((this->_data[byte_i] & ~mask)//unset bit
                            | (state << (i % 8)) //set bit if needed
                        );
    }
};