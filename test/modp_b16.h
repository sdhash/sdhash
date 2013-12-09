/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*- */
/* vi: set expandtab shiftwidth=4 tabstop=4: */

/**
 * \file modp_b16.h
 * \brief High performance encoding and decoding of base 16
 *        (hexadecimal 0-9, A-F)
 *
 */

/*
 * <PRE>
 * MODP_B16 -- High performance base16 (hex) Encoder/Decoder
 * http://code.google.com/p/stringencoders/
 *
 * Copyright &copy; 2005-2007, Nick Galbreath -- nickg [at] modp [dot] com
 * All rights reserved.
 *
 * Released under bsd license.  See modp_b16.c for details.
 * </PRE>
 *
 */

#ifndef COM_MODP_STRINGENCODERS_B16
#define COM_MODP_STRINGENCODERS_B16

#include "modp_stdint.h"
//#include "extern_c_begin.h"

/**
 * encode a string into hex (base 16, 0-9,a-f)
 *
 * \param[out] dest the output string.  Must have at least modp_b16_encode_len
 *   bytes allocated
 * \param[in] str the input string
 * \param[in] len of the input string
 * \return strlen of dest
 */
size_t modp_b16_encode(char* dest, const char* str, size_t len);

/**
 * Decode a hex-encoded string.
 *
 * \param[out] dest output, must have at least modp_b16_decode_len bytes allocated,
 *   input must be a mutliple of 2, and be different than the source buffer.
 * \param[in] src the hex encoded source
 * \param[in] len the length of the source
 * \return the length of the the output, or -1 if an error
 */
size_t modp_b16_decode(char* dest, const char* src, size_t len);

/**
 * Encode length.
 * 2 x the length of A, round up the next high mutliple of 2
 * +1 for null byte added
 */
#define modp_b16_encode_len(A) (2*A + 1)

/**
 * Encode string length
 */
#define modp_b16_encode_strlen(A) (2*A)

/**
 * Decode string length
 */
#define modp_b16_decode_len(A) ((A + 1)/ 2)

//#include "extern_c_end.h"

#ifdef __cplusplus
#include <cstring>
#include <string>

namespace modp {

    inline std::string b16_encode(const char* s, size_t len)
    {
        std::string x(modp_b16_encode_len(len), '\0');
        size_t d = modp_b16_encode(const_cast<char*>(x.data()), s, len);
        if (d == (size_t)-1) {
            x.clear();
        } else {
            x.erase(d, std::string::npos);
        }
        return x;
    }

    inline std::string b16_encode(const char* s)
    {
        return b16_encode(s, strlen(s));
    }

    inline std::string b16_encode(const std::string& s)
    {
        return b16_encode(s.data(), s.size());
    }

    /**
     * hex encode a string (self-modified)
     * \param[in,out] s the input string to be encoded
     * \return a reference to the input string.
     */
    inline std::string& b16_encode(std::string& s)
    {
        std::string x(b16_encode(s.data(), s.size()));
        s.swap(x);
        return s;
    }

    inline std::string b16_decode(const char* s, size_t len)
    {
        std::string x(len / 2 + 1, '\0');
        size_t d = modp_b16_decode(const_cast<char*>(x.data()), s, len);
        if (d == (size_t)-1) {
            x.clear();
        } else {
            x.erase(d, std::string::npos);
        }
        return x;
    }

    inline std::string b16_decode(const char* s)
    {
        return b16_decode(s, strlen(s));
    }

    /**
     * Decode a hex-encoded string.  On error, input string is cleared.
     * This function does not allocate memory.
     *
     * \param[in,out] s the input string
     * \return a reference to the input string
     */
    inline std::string& b16_decode(std::string& s)
    {
        std::string x(b16_decode(s.data(), s.size()));
        s.swap(x);
        return s;
    }

    inline std::string b16_decode(const std::string& s)
    {
        return b16_decode(s.data(), s.size());
    }

} /* namespace modp */

#endif  /* ifdef __cplusplus */

#endif  /* ifndef modp_b16 */

