#ifndef CDOLHEADER_H
#define CDOLHEADER_H

#include <Common/Log.h>
#include <Common/FileIO/IOutputStream.h>
#include <Common/FileIO/IInputStream.h>

#include <array>

class SDolHeader
{
public:
    static constexpr size_t kNumTextSections = 7;
    static constexpr size_t kNumDataSections = 11;
    static constexpr size_t kNumSections = kNumTextSections + kNumDataSections;

    struct Section
    {
        uint32_t Offset{};
        uint32_t BaseAddress{};
        uint32_t Size{};

        bool IsEmpty() const {
            return Size == 0;
        }
    };

    std::array<Section, kNumSections> Sections;
    uint32_t BssAddress{};
    uint32_t BssSize{};
    uint32_t EntryPoint{};

    explicit SDolHeader(IInputStream& rInput)
    {
        for (auto& section : Sections)
        {
            section.Offset = rInput.ReadU32();
        }
        for (auto& section : Sections)
        {
            section.BaseAddress = rInput.ReadU32();
        }
        for (auto& section : Sections)
        {
            section.Size = rInput.ReadU32();
        }
        BssAddress = rInput.ReadU32();
        BssSize = rInput.ReadU32();
        EntryPoint = rInput.ReadU32();
    }

    void Write(IOutputStream& rOutput) const
    {
        for (const auto& section : Sections)
        {
            rOutput.WriteU32(section.Offset);
        }
        for (const auto& section : Sections)
        {
            rOutput.WriteU32(section.BaseAddress);
        }
        for (const auto& section : Sections)
        {
            rOutput.WriteU32(section.Size);
        }
        rOutput.WriteU32(BssAddress);
        rOutput.WriteU32(BssSize);
        rOutput.WriteU32(EntryPoint);
    }

    bool AddTextSection(uint32_t address, uint32_t fileOffset, uint32_t size)
    {
        if ((size & 0x1f) != 0)
        {
            NLog::Warn("Unable to add text section: Size not 32-bit aligned.");
            return false;
        }

        for (size_t i = 0; i < kNumTextSections; ++i)
        {
            if (Sections[i].IsEmpty())
            {
                Sections[i].BaseAddress = address;
                Sections[i].Offset = fileOffset;
                Sections[i].Size = size;
                return true;
            }
        }

        NLog::Warn("Unable to add text section: no empty section found.");
        return false;
    }

    uint32_t OffsetForAddress(uint32_t address) const
    {
        for (const auto& sec : Sections)
        {
            if (address > sec.BaseAddress && address < sec.BaseAddress + sec.Size)
            {
                return sec.Offset + (address - sec.BaseAddress);
            }
        }
        NLog::Warn("Unable to add section for address: {:x}", address);
        return 0;
    }
};

#endif // SDOLHEADER_H
