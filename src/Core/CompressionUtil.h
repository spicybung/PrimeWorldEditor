#ifndef COMPRESSIONUTIL_H
#define COMPRESSIONUTIL_H

#include <cstdint>

namespace CompressionUtil
{
    // Decompression
    bool DecompressZlib(uint8_t *pSrc, uint32_t SrcLen, uint8_t *pDst, uint32_t DstLen, uint32_t& rTotalOut);
    bool DecompressLZO(uint8_t* pSrc, uint32_t SrcLen, uint8_t* pDst, uint32_t DstLen, uint32_t& rTotalOut);
    bool DecompressSegmentedData(uint8_t *pSrc, uint32_t SrcLen, uint8_t *pDst, uint32_t DstLen);

    // Compression
    bool CompressZlib(uint8_t *pSrc, uint32_t SrcLen, uint8_t *pDst, uint32_t DstLen, uint32_t& rTotalOut);
    bool CompressLZO(uint8_t* pSrc, uint32_t SrcLen, uint8_t* pDst, uint32_t DstLen, uint32_t& rTotalOut);
    bool CompressSegmentedData(uint8_t *pSrc, uint32_t SrcLen, uint8_t *pDst, uint32_t& rTotalOut, bool IsZlib, bool AllowUncompressedSegments);
    bool CompressZlibSegmented(uint8_t *pSrc, uint32_t SrcLen, uint8_t *pDst, uint32_t& rTotalOut, bool AllowUncompressedSegments);
    bool CompressLZOSegmented(uint8_t *pSrc, uint32_t SrcLen, uint8_t *pDst, uint32_t& rTotalOut, bool AllowUncompressedSegments);
}

#endif // COMPRESSIONUTIL_H
