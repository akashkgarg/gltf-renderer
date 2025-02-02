#include <memory>

/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <ImageDecoder.h>

#include <cstdint>
#include <cstring> // for memcmp
#include <iostream> // for cerr
#include <limits>
#include <memory>
#include <sstream>

// for ntohs
#if defined(WIN32)
#    include <Winsock2.h>
#    include <utils/unwindows.h>
#else
#    include <arpa/inet.h>
#endif

#include <math/vec3.h>
#include <math/vec4.h>

#include <vector>

#include <image/ColorTransform.h>
#include <image/ImageOps.h>

#include <HDRDecoder.h>

namespace image {

LinearImage ImageDecoder::decode(std::istream& stream, const std::string& sourceName,
        ColorSpace sourceSpace) {

    Format format = Format::NONE;

    std::streampos pos = stream.tellg();
    char buf[16];
    stream.read(buf, sizeof(buf));

    if (HDRDecoder::checkSignature(buf)) {
        format = Format::HDR;
    } 

    stream.seekg(pos);

    std::unique_ptr<Decoder> decoder;
    switch (format) {
        case Format::NONE:
            return LinearImage();
        case Format::HDR:
            decoder.reset(HDRDecoder::create(stream));
            decoder->setColorSpace(ColorSpace::LINEAR);
            break;
    }

    return decoder->decode();
}

// -----------------------------------------------------------------------------------------------

static inline float read32(std::istream& istream) {
    uint32_t data;
    istream.read(reinterpret_cast<char*>(&data), sizeof(uint32_t));
    data = ntohl(data);
    return *reinterpret_cast<float*>(&data);
}

static inline float read16(std::istream& istream) {
    uint16_t data;
    istream.read(reinterpret_cast<char*>(&data), sizeof(uint16_t));
    return static_cast<float>(ntohs(data)) / std::numeric_limits<uint16_t>::max();
}

// -----------------------------------------------------------------------------------------------

} // namespace image
