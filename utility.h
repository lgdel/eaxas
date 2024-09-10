#pragma once
#include <stdint.h>
#include <fstream>

using namespace std;

#ifdef _MSC_VER

#define BSWAP32(x) _byteswap_ulong(x)
#define BSWAP16(x) _byteswap_ushort(x)

// #else ...

#endif

inline int16_t clipInt16(int a)
{
    if ((a + 0x8000U) & ~0xFFFF) return (a >> 31) ^ 0x7FFF;
    else                      return a;
}

template <typename T>
inline T readValue(fstream& fs) {
    T res;
    fs.read((char*)&res, sizeof(T));
    return res;
}

template <typename T>
inline void bufferWriteValue(uint8_t* &buf, T val) {
    *reinterpret_cast<T*>(buf) = val;
    buf += sizeof(T);
}

inline void bufferWrite16BEUnalign(uint8_t* &buf, uint16_t val) {
    uint16_t valueBE = BSWAP16(val);
    memcpy(buf, &valueBE, 2);
    buf += 2;
}
