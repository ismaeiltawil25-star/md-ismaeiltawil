#pragma once

#include <cstdint>
#include <QString>
#include <compare>

struct PdfVersion {
    uint8_t major = 0;
    uint8_t minor = 0;

    [[nodiscard]] constexpr bool isValid() const noexcept {
        return major == 1 && minor <= 7;
    }

    [[nodiscard]] QString toString() const {
        return QStringLiteral("PDF-%1.%2").arg(major).arg(minor);
    }

    constexpr auto operator<=>(const PdfVersion &) const = default;
};
