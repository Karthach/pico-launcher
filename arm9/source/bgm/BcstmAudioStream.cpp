#include "common.h"
#include <stdint.h>
#include <string.h>
#include <algorithm>
#include <memory>
#include "BcstmAudioStream.h"

static bool rangeFits(u64 offset, u64 length, u64 totalSize)
{
    return offset <= totalSize && length <= totalSize - offset;
}

static u32 makeBcstmSignature()
{
    return (u32)'C' | ((u32)'S' << 8) | ((u32)'T' << 16) | ((u32)'M' << 24);
}

bool BcstmAudioStream::Open(const TCHAR* filePath)
{
    if (_audioFile.Open(filePath, FA_OPEN_EXISTING | FA_READ) != FR_OK)
        return false;

    return TryLoadBcstm();
}

bool BcstmAudioStream::Open(const FastFileRef& fastFileRef)
{
    _audioFile.Open(fastFileRef, FA_OPEN_EXISTING | FA_READ);
    return TryLoadBcstm();
}

bool BcstmAudioStream::TryLoadBcstm()
{
    const u32 fileSize = _audioFile.GetSize();
    if (fileSize < sizeof(bcstm_header_t))
        return false;

    if (_audioFile.CreateClusterTable(_clusterTable, sizeof(_clusterTable)) != FR_OK)
        return false;

    bcstm_header_t header;
    u32 bytesRead = 0;
    if (_audioFile.Read(&header, sizeof(bcstm_header_t), bytesRead) != FR_OK ||
        bytesRead != sizeof(bcstm_header_t))
    {
        return false;
    }

    if (header.signature != makeBcstmSignature() ||
        header.fileSize < sizeof(bcstm_header_t) || header.fileSize > fileSize ||
        header.infoBlockRef.size < sizeof(bcstm_info_t) ||
        header.infoBlockRef.size < 8 ||
        header.dataBlockRef.size < 8 ||
        !rangeFits(header.infoBlockRef.offset, header.infoBlockRef.size, header.fileSize) ||
        !rangeFits(header.dataBlockRef.offset, header.dataBlockRef.size, header.fileSize))
    {
        return false;
    }

    if (_audioFile.Seek(header.infoBlockRef.offset) != FR_OK)
        return false;

    bcstm_info_t info;
    if (_audioFile.Read(&info, sizeof(bcstm_info_t), bytesRead) != FR_OK ||
        bytesRead != sizeof(bcstm_info_t))
    {
        return false;
    }

    const u64 infoPayloadSize = header.infoBlockRef.size - 8;
    if (info.streamInfoRef.offset > infoPayloadSize ||
        sizeof(bcstm_info_stream_t) > infoPayloadSize - info.streamInfoRef.offset)
    {
        return false;
    }

    if (_audioFile.Seek(header.infoBlockRef.offset + 8 + info.streamInfoRef.offset) != FR_OK)
        return false;

    if (_audioFile.Read(&_streamInfo, sizeof(bcstm_info_stream_t), bytesRead) != FR_OK ||
        bytesRead != sizeof(bcstm_info_stream_t))
    {
        return false;
    }

    if (_streamInfo.format != BCSTM_FORMAT_DSP_ADPCM ||
        _streamInfo.nrChannels == 0 || _streamInfo.nrChannels > 2 ||
        _streamInfo.sampleRate == 0 || _streamInfo.sampleRate > 192000 ||
        _streamInfo.nrBlocks == 0 || _streamInfo.blockSize < 8 || _streamInfo.blockSize > 0x10000 ||
        _streamInfo.blockSampleCount == 0 || _streamInfo.lastBlockSampleCount == 0 ||
        _streamInfo.lastBlockPaddedSize < 8 ||
        _streamInfo.lastBlockPaddedSize > _streamInfo.blockSize ||
        _streamInfo.lastBlockSize > _streamInfo.lastBlockPaddedSize)
    {
        return false;
    }

    const u64 samplesPerBlock = (u64)(_streamInfo.blockSize / 8) * 14;
    const u64 samplesInLastBlock = (u64)(_streamInfo.lastBlockPaddedSize / 8) * 14;
    if (_streamInfo.blockSampleCount > samplesPerBlock ||
        _streamInfo.lastBlockSampleCount > samplesInLastBlock)
    {
        return false;
    }

    const u64 totalSamples = (u64)(_streamInfo.nrBlocks - 1) * _streamInfo.blockSampleCount +
        _streamInfo.lastBlockSampleCount;
    if (totalSamples == 0 || totalSamples > UINT32_MAX)
        return false;

    if (_streamInfo.loop && (_streamInfo.loopEnd <= _streamInfo.loopStart ||
        _streamInfo.loopEnd > totalSamples))
    {
        return false;
    }

    const u64 dataOffset = (u64)header.dataBlockRef.offset + 8 + _streamInfo.dataRef.offset;
    const u64 dataRelativeOffset = 8 + (u64)_streamInfo.dataRef.offset;
    const u64 dataBytesRequired = (u64)(_streamInfo.nrBlocks - 1) * _streamInfo.blockSize *
        _streamInfo.nrChannels + (u64)_streamInfo.lastBlockPaddedSize * _streamInfo.nrChannels;
    if (dataRelativeOffset > header.dataBlockRef.size ||
        dataBytesRequired > header.dataBlockRef.size - dataRelativeOffset ||
        !rangeFits(dataOffset, dataBytesRequired, header.fileSize))
    {
        return false;
    }
    _dataOffset = (u32)dataOffset;

    _channels = std::min<u32>(_streamInfo.nrChannels, 2);

    if (_streamInfo.format == BCSTM_FORMAT_DSP_ADPCM)
    {
        u64 channelTableOffset = info.channelInfoRef.offset;
        if (channelTableOffset > infoPayloadSize ||
            sizeof(bcstm_ref_table_t) > infoPayloadSize - channelTableOffset)
        {
            return false;
        }

        u32 tableSize = sizeof(bcstm_ref_table_t) + sizeof(bcstm_ref_t) * (_streamInfo.nrChannels - 1);
        if (tableSize > infoPayloadSize - channelTableOffset)
            return false;
        auto channelInfoRefTab = std::unique_ptr<u8[]>(new u8[tableSize]);

        if (_audioFile.Seek(header.infoBlockRef.offset + 8 + info.channelInfoRef.offset) != FR_OK)
            return false;

        if (_audioFile.Read(channelInfoRefTab.get(), tableSize, bytesRead) != FR_OK ||
            bytesRead != tableSize)
        {
            return false;
        }

        auto channelInfoRefTabPtr = reinterpret_cast<bcstm_ref_table_t*>(channelInfoRefTab.get());
        if (channelInfoRefTabPtr->count < _streamInfo.nrChannels)
            return false;
        for (u32 i = 0; i < _channels; i++)
        {
            u64 channelRelativeOffset = (u64)info.channelInfoRef.offset +
                channelInfoRefTabPtr->references[i].offset;
            if (channelRelativeOffset > infoPayloadSize ||
                sizeof(bcstm_info_channel_t) > infoPayloadSize - channelRelativeOffset)
                return false;

            u64 channelOffset = header.infoBlockRef.offset + 8 + channelRelativeOffset;
            if (!rangeFits(channelOffset, sizeof(bcstm_info_channel_t), header.fileSize))
                return false;
            u32 offset = (u32)channelOffset;
            if (_audioFile.Seek(offset) != FR_OK)
                return false;

            bcstm_info_channel_t channel;
            if (_audioFile.Read(&channel, sizeof(bcstm_info_channel_t), bytesRead) != FR_OK ||
                bytesRead != sizeof(bcstm_info_channel_t))
            {
                return false;
            }

            u64 codecInfoRelativeOffset = channelRelativeOffset + channel.codecInfoRef.offset;
            if (codecInfoRelativeOffset > infoPayloadSize ||
                sizeof(bcstm_dspadpcm_t) > infoPayloadSize - codecInfoRelativeOffset)
                return false;

            u64 codecInfoOffset = header.infoBlockRef.offset + 8 + codecInfoRelativeOffset;
            if (!rangeFits(codecInfoOffset, sizeof(bcstm_dspadpcm_t), header.fileSize) ||
                _audioFile.Seek((u32)codecInfoOffset) != FR_OK)
                return false;

            if (_audioFile.Read(&_dspAdpcmInfo[i], sizeof(bcstm_dspadpcm_t), bytesRead) != FR_OK ||
                bytesRead != sizeof(bcstm_dspadpcm_t))
            {
                return false;
            }
        }
    }

    for (u32 i = 0; i < _channels; i++)
    {
        _adpcmBlocks[i] = std::unique_ptr<u8[]>(new(cache_align) u8[_streamInfo.blockSize]);
        if (_streamInfo.format == BCSTM_FORMAT_DSP_ADPCM)
        {
            _dspAdpcmContexts[i].coefTable = _dspAdpcmInfo[i].coefs;
            _dspAdpcmContexts[i].frameData[12] = _dspAdpcmInfo[i].context.last2;
            _dspAdpcmContexts[i].frameData[13] = _dspAdpcmInfo[i].context.last1;
        }
    }

    _blockNumber = 0;
    _sampleNumberInBlock = 0;

    if (_streamInfo.loop)
    {
        _loopBlockNumber = _streamInfo.loopStart / _streamInfo.blockSampleCount;
        _loopBlockStartSample = _streamInfo.loopStart % _streamInfo.blockSampleCount;

        _loopEndBlockNumber = (_streamInfo.loopEnd - 1) / _streamInfo.blockSampleCount;
        _loopEndBlockEndSample = (_streamInfo.loopEnd - 1) % _streamInfo.blockSampleCount;
    }

    return true;
}

void BcstmAudioStream::Close()
{
    _audioFile.Close();
    _adpcmBlocks[0].reset();
    _adpcmBlocks[1].reset();
}

void BcstmAudioStream::ReadSamples(s16* left, s16* right, u32 count)
{
    u32 remaining = count;
    s16* curLeft = left;
    s16* curRight = right;
    do
    {
        if (_sampleNumberInBlock == 0)
        {
            if (!FetchBlock())
            {
                memset(curLeft, 0, remaining * sizeof(s16));
                memset(curRight, 0, remaining * sizeof(s16));
                return;
            }
        }

        u32 totalSamplesInBlock = GetTotalSamplesInCurrentBlock();
        u32 samplesToDecode = std::min(totalSamplesInBlock - _sampleNumberInBlock, remaining);
        DecodeAdpcmSamples(_dspAdpcmContexts[0], curLeft, samplesToDecode);
        if (_channels == 2)
            DecodeAdpcmSamples(_dspAdpcmContexts[1], curRight, samplesToDecode);
        curLeft += samplesToDecode;
        curRight += samplesToDecode;
        remaining -= samplesToDecode;
        _sampleNumberInBlock += samplesToDecode;
        if (_sampleNumberInBlock == totalSamplesInBlock)
        {
            _blockNumber++;
            _sampleNumberInBlock = 0;
        }
    } while (remaining != 0);

    // when mono, copy the samples from the left to the right channel
    if (_channels == 1)
        memcpy(right, left, count * sizeof(s16));
}

u32 BcstmAudioStream::GetTotalSamplesInCurrentBlock()
{
    u32 totalSamplesInBlock = _blockNumber == _streamInfo.nrBlocks - 1 ? _streamInfo.lastBlockSampleCount : _streamInfo.blockSampleCount;
    if (_streamInfo.loop && _blockNumber == _loopEndBlockNumber)
        totalSamplesInBlock = _loopEndBlockEndSample + 1;
    return totalSamplesInBlock;
}

bool BcstmAudioStream::FetchBlock()
{
    bool looped = false;
    if (_blockNumber == _streamInfo.nrBlocks || (_streamInfo.loop && _blockNumber == _loopEndBlockNumber + 1))
    {
        looped = true;
        _blockNumber = _streamInfo.loop ? _loopBlockNumber : 0;
    }

    // fetch block
    u32 blockOffset = _dataOffset + _streamInfo.blockSize * (_blockNumber * _streamInfo.nrChannels);
    if (_audioFile.Seek(blockOffset) != FR_OK)
        return false;

    for (u32 i = 0; i < _channels; i++)
    {
        u32 blockSize = _blockNumber == _streamInfo.nrBlocks - 1
            ? _streamInfo.lastBlockPaddedSize : _streamInfo.blockSize;
        u32 bytesRead = 0;
        if (_audioFile.Read(_adpcmBlocks[i].get(), blockSize, bytesRead) != FR_OK ||
            bytesRead != blockSize)
        {
            return false;
        }
        _dspAdpcmContexts[i].SetData(_adpcmBlocks[i].get());
    }

    if (looped)
    {
        _sampleNumberInBlock = _loopBlockStartSample;
        if (_streamInfo.format == BCSTM_FORMAT_DSP_ADPCM)
        {
            if (_blockNumber == 0)
            {
                for (u32 i = 0; i < _channels; i++)
                {
                    _dspAdpcmContexts[i].frameData[12] = _dspAdpcmInfo[i].context.last2;
                    _dspAdpcmContexts[i].frameData[13] = _dspAdpcmInfo[i].context.last1;
                }
            }
            else
            {
                u32 frameOffset = _loopBlockStartSample % 14u;
                u32 frame = _loopBlockStartSample / 14u;
                for (u32 i = 0; i < _channels; i++)
                {
                    _dspAdpcmContexts[i].SetData(_adpcmBlocks[i].get() + frame * 8);
                    if (frameOffset != 0)
                    {
                        DecodeLoopAdpcmFrame(
                            _dspAdpcmContexts[i],
                            _dspAdpcmInfo[i].loopContext.predictorAndScale,
                            _dspAdpcmInfo[i].loopContext.last1,
                            _dspAdpcmInfo[i].loopContext.last2,
                            _loopBlockStartSample % 14u);
                    }
                    else
                    {
                        _dspAdpcmContexts[i].frameData[12] = _dspAdpcmInfo[i].loopContext.last2;
                        _dspAdpcmContexts[i].frameData[13] = _dspAdpcmInfo[i].loopContext.last1;
                    }
                }
            }
        }
    }
    return true;
}

extern "C" void dspadpcm_decode(DspAdpcmContext* context);

void BcstmAudioStream::DecodeAdpcmSamples(DspAdpcmContext& context, s16* dst, u32 count)
{
    do
    {
        if (context.frameIdx == 0)
            dspadpcm_decode(&context);
        u32 toCopy = std::min<u32>(14 - context.frameIdx, count);
        memcpy(dst, &context.frameData[context.frameIdx], toCopy * sizeof(s16));
        context.frameIdx += toCopy;
        if (context.frameIdx == 14)
            context.frameIdx = 0;
        count -= toCopy;
        dst += toCopy;
    } while (count != 0);
}

// ITCM_CODE void BcstmAudioStream::DecodeAdpcmFrame(DspAdpcmContext& context)
// {
//     const u8* data = context.data;
//     u32 scaleCoef = *data++;
//     u32 scale = (scaleCoef & 0xF) + 11;
//     u32 coefIdx = scaleCoef >> 4;
//     s16 coef1 = context.coefTable[coefIdx * 2];
//     s16 coef2 = context.coefTable[coefIdx * 2 + 1];
//     s16 last2 = context.frameData[12];
//     s16 last1 = context.frameData[13];

//     for (u32 j = 0; j < 7; j++)
//     {
//         u32 dataValue = *data++;
//         int high = (int)(dataValue << 24) >> 28;
//         int val = ((high << scale) + (coef1 * last1 + coef2 * last2) + 1024) >> 11;
//         int sample = std::clamp(val, -0x8000, 0x7FFF);
//         context.frameData[j * 2] = sample;
//         last2 = last1;
//         last1 = sample;

//         int low = (int)(dataValue << 28) >> 28;
//         val = ((low << scale) + (coef1 * last1 + coef2 * last2) + 1024) >> 11;
//         sample = std::clamp(val, -0x8000, 0x7FFF);
//         context.frameData[j * 2 + 1] = sample;
//         last2 = last1;
//         last1 = sample;
//     }

//     context.frameIdx = 0;
//     context.data = data;
// }

ITCM_CODE void BcstmAudioStream::DecodeLoopAdpcmFrame(
    DspAdpcmContext& context, u32 scaleCoef, s16 last1, s16 last2, u32 sampleOffset)
{
    const u8* data = context.data;
    data += 2 + (sampleOffset >> 1);
    u32 scale = (scaleCoef & 0xF) + 11;
    u32 coefIdx = scaleCoef >> 4;
    s16 coef1 = context.coefTable[coefIdx * 2];
    s16 coef2 = context.coefTable[coefIdx * 2 + 1];
    if ((int)sampleOffset - 2 >= 0)
        context.frameData[sampleOffset - 2] = last2;
    if ((int)sampleOffset - 1 >= 0)
        context.frameData[sampleOffset - 1] = last1;

    for (u32 j = sampleOffset; j < 14; j += 2)
    {
        u32 dataValue = *data++;
        if ((j & 1) == 0)
        {
            int high = (int)(dataValue << 24) >> 28;
            int val = ((high << scale) + (coef1 * last1 + coef2 * last2) + 1024) >> 11;
            int sample = std::clamp(val, -0x8000, 0x7FFF);
            context.frameData[j] = sample;
            last2 = last1;
            last1 = sample;
        }
        else
        {
            j--;
        }

        int low = (int)(dataValue << 28) >> 28;
        int val = ((low << scale) + (coef1 * last1 + coef2 * last2) + 1024) >> 11;
        int sample = std::clamp(val, -0x8000, 0x7FFF);
        context.frameData[j + 1] = sample;
        last2 = last1;
        last1 = sample;
    }

    context.frameIdx = sampleOffset;
    context.data = data;
}
