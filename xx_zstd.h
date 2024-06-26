﻿#pragma once
#include <xx_data.h>
#include <zstd.h>

namespace xx {

    // .exe + 50k
    inline void ZstdDecompress(std::string_view const& src, Data& dst) {
        auto&& siz = ZSTD_getFrameContentSize(src.data(), src.size());
        if (ZSTD_CONTENTSIZE_UNKNOWN == siz) throw std::logic_error("ZstdDecompress error: unknown content size.");
        if (ZSTD_CONTENTSIZE_ERROR == siz) throw std::logic_error("ZstdDecompress read content size error.");
        dst.Resize(siz);
        if (0 == siz) return;
        siz = ZSTD_decompress(dst.buf, siz, src.data(), src.size());
        if (ZSTD_isError(siz)) throw std::logic_error("ZstdDecompress decompress error.");
        dst.Resize(siz);
    }

    inline void TryZstdDecompress(Data& d) {
        if (d.len >= 4) {
            if (d[0] == 0x28 && d[1] == 0xB5 && d[2] == 0x2F && d[3] == 0xFD) {
                Data d2;
                ZstdDecompress(d, d2);
                std::swap(d, d2);
            }
        }
    }

    // .exe + 320k
    template<int level = 3, bool doShrink = true>
    inline void ZstdCompress(std::string_view const& src, Data& dst) {
        auto req = ZSTD_compressBound(src.size());
        dst.Resize(req);
        dst.len = ZSTD_compress(dst.buf, dst.cap, src.data(), src.size(), level);
        if (doShrink) {
            dst.Shrink();
        }
    }

}
