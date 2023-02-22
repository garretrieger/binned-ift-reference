
#include <iostream>

#pragma once

/* 1, 2, and 4 byte big-endian machine independent input */
template<class T>
static void readObject(std::istream &f, T& o) {
    static_assert(sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4);
    uint32_t value;
    char b;

    switch (sizeof(T)) {
        case 1:
            f.get(b);
            *((uint8_t *)&o) = (uint8_t)b;
            break;
        case 2:
            f.get(b);
            value = (uint8_t)b;
            f.get(b);
            value = value << 8 | (uint8_t)b;
            *((uint16_t *)&o) = (uint16_t)value;
            break;
        case 4:
            f.get(b);
            value = (uint8_t)b;
            f.get(b);
            value = value << 8 | (uint8_t)b;
            f.get(b);
            value = value << 8 | (uint8_t)b;
            f.get(b);
            value = value << 8 | (uint8_t)b;
            *((uint32_t *)&o) = value;
            break;
    }
}

template<class T>
inline T readObject(std::istream &f) {
    T r;
    readObject(f, r);
    return r;
}

/* 1, 2, and 4 byte big-endian machine independent output */
template<class T>
static void writeObject(std::ostream &f, const T& o) {
    static_assert(sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4);
    char b;

    switch (sizeof(T)) {
        case 1:
            b = (uint8_t)(o & 0xFF);
            f.put(b);
            break;
        case 2:
            b = (uint8_t)(o >> 8 & 0xFF);
            f.put(b);
            b = (uint8_t)(o & 0xFF);
            f.put(b);
            break;
        case 4:
            b = (uint8_t)(o >> 24 & 0xFF);
            f.put(b);
            b = (uint8_t)(o >> 16 & 0xFF);
            f.put(b);
            b = (uint8_t)(o >> 8 & 0xFF);
            f.put(b);
            b = (uint8_t)(o & 0xFF);
            f.put(b);
            break;
    }
}

