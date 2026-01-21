#ifndef SCRIPT_CINSTANCEID_H
#define SCRIPT_CINSTANCEID_H

#include <compare>
#include <cstddef>
#include <cstdint>
#include <functional>

// CScriptObject instance ID
class CInstanceID
{
public:
    using value_type = uint32_t;

    constexpr CInstanceID() = default;
    constexpr explicit CInstanceID(value_type id) : mId(id) {}

    constexpr CInstanceID& operator=(value_type id)
    {
        mId = id;
        return *this;
    }

    [[nodiscard]] constexpr auto operator<=>(const CInstanceID&) const = default;

    [[nodiscard]] constexpr bool IsInvalid() const { return mId == UINT32_MAX; }
    [[nodiscard]] constexpr bool IsValid() const { return !IsInvalid(); }
    constexpr void Invalidate() { mId = UINT32_MAX; }

    [[nodiscard]] constexpr value_type Value() const { return mId; }
    [[nodiscard]] constexpr uint8_t Layer() const { return uint8_t((mId >> 26U) & 0x3FU); }
    [[nodiscard]] constexpr uint16_t Area() const { return uint16_t((mId >> 16U) & 0x3FFU); }
    [[nodiscard]] constexpr uint16_t Id() const { return uint16_t(mId & 0xFFFFU); }

private:
    value_type mId = UINT32_MAX;
};

// Basic fmt compatibility. Allows it to be formatted as its contained value type.
constexpr auto format_as(const CInstanceID& id)
{
    return id.Value();
}

template <>
struct std::hash<CInstanceID>
{
    size_t operator()(const CInstanceID& id) const noexcept
    {
        return hash<CInstanceID::value_type>()(id.Value());
    }
};

#endif
