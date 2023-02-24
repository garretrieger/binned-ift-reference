
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

struct simpleibuf : std::streambuf {
    simpleibuf(char *buf=nullptr, std::streamsize n=0) {
        setg(buf, buf, buf + n);
    }
    std::streambuf *setbuf(char *buf, std::streamsize n) override {
        setg(buf, buf, buf+n);
        return this;
    }
    std::streampos seekpos(std::streampos sp,
                           std::ios_base::openmode which = std::ios_base::in | std::ios_base::out) override {
        if (sp < 0 || sp > (egptr() - eback()))
            return -1;
        char *p = eback() + sp;
        if (which & std::ios_base::in) {
            setg(eback(), p, egptr());
        }
        return sp;
    }
    std::streampos seekoff(std::streamoff so, std::ios_base::seekdir way,
                           std::ios_base::openmode which = std::ios_base::in | std::ios_base::out) override {
        std::streampos sp;
        if (way == std::ios_base::beg)
            sp = so;
        else if (way == std::ios_base::end) {
            sp = so + (egptr() - eback());
        } else {
            assert(way == std::ios_base::cur);
            sp = so + (gptr() - eback());
        }
        return seekpos(sp, which);
    }
    virtual const char *bufinfo(size_t &length) {
        const char *c = eback();
        length = egptr() - c;
        return c;
    }
};

struct simpleistream: private virtual simpleibuf, public std::istream {
public:
    simpleistream(const char *buf=nullptr, size_t length=0) :
        simpleibuf(const_cast<char *>(buf), length),
        std::ios(static_cast<std::streambuf *>(&sb)),
        std::istream(static_cast<std::streambuf *>(&sb)) {}
    virtual const char *bufinfo(size_t &length) {
        return sb.bufinfo(length);
    }
    virtual const char *bufinfo() {
        size_t length;
        return sb.bufinfo(length);
    }
private:
    simpleibuf sb;
};

struct simplebuf : std::streambuf {
    simplebuf(char *buf=nullptr, size_t length=0) {
        setg(buf, buf, buf + length);
        setp(buf, buf + length);
    }
    std::streambuf *setbuf(char *buf, std::streamsize n) override {
        setg(buf, buf, buf + n);
        setp(buf, buf + n);
        return this;
    }
    std::streampos seekpos(std::streampos sp,
                           std::ios_base::openmode which = std::ios_base::in | std::ios_base::out) override {
        if (sp < 0 || sp > (egptr() - eback()))
            return -1;
        if (which & std::ios_base::in) {
            setg(eback(), eback() + sp, egptr());
        }
        if (which & std::ios_base::out) {
            setp(eback(), egptr());
            pbump(sp);
        }
        return sp;
    }
    std::streampos seekoff(std::streamoff so, std::ios_base::seekdir way,
                           std::ios_base::openmode which = std::ios_base::in | std::ios_base::out) override {
        std::streampos sp;
        if (way == std::ios_base::beg)
            sp = so;
        else if (way == std::ios_base::end) {
            sp = so + (egptr() - eback());
        } else {
            assert(way == std::ios_base::cur);
            sp = so + (gptr() - eback());
        }
        return seekpos(sp, which);
    }
    virtual char *bufinfo(size_t &length) {
        char *c = eback();
        length = egptr() - c;
        return c;
    }
};

struct simplestream: public std::iostream {
public:
    simplestream(char *buf=nullptr, size_t length=0) :
        sb(buf, length),
        std::ios(static_cast<std::streambuf *>(&sb)),
        std::iostream(static_cast<std::streambuf *>(&sb)) {}
    virtual char *bufinfo(size_t &length) {
        return sb.bufinfo(length);
    }
    virtual char *bufinfo() {
        size_t length;
        return sb.bufinfo(length);
    }
private:
    simplebuf sb;
};
