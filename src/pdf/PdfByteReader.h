#pragma once

#include <QByteArrayView>
#include <optional>
#include <cstdint>

class PdfByteReader {
public:
    explicit PdfByteReader(QByteArrayView data) noexcept
        : m_data(data), m_pos(0) {}

    std::optional<uint8_t> readByte() noexcept {
        if (m_pos >= m_data.size()) return std::nullopt;
        auto byte = static_cast<uint8_t>(static_cast<unsigned char>(m_data[static_cast<qsizetype>(m_pos)]));
        ++m_pos;
        return byte;
    }

    std::optional<QByteArrayView> readBytes(qsizetype count) noexcept {
        if (count < 0) return std::nullopt;
        if (count == 0) return QByteArrayView{};
        if (m_pos + count > m_data.size()) return std::nullopt;
        QByteArrayView result = m_data.sliced(static_cast<qsizetype>(m_pos), static_cast<qsizetype>(count));
        m_pos += count;
        return result;
    }

    std::optional<uint8_t> peekByte() const noexcept {
        if (m_pos >= m_data.size()) return std::nullopt;
        return static_cast<uint8_t>(static_cast<unsigned char>(m_data[static_cast<qsizetype>(m_pos)]));
    }

    void skipBytes(qsizetype count) noexcept {
        if (count <= 0) return;
        m_pos += count;
        if (m_pos > m_data.size()) m_pos = m_data.size();
    }

    void skipAsciiWhitespace() noexcept {
        static constexpr auto isPdfWhitespace = [](char b) constexpr noexcept -> bool {
            auto c = static_cast<unsigned char>(b);
            return c == 0x00 || c == 0x09 || c == 0x0A || c == 0x0C || c == 0x0D || c == 0x20;
        };
        while (m_pos < m_data.size() && isPdfWhitespace(m_data[static_cast<qsizetype>(m_pos)])) {
            ++m_pos;
        }
    }

    void skipWhitespaceAndComments() noexcept {
        static constexpr auto isPdfWhitespace = [](char b) constexpr noexcept -> bool {
            auto c = static_cast<unsigned char>(b);
            return c == 0x00 || c == 0x09 || c == 0x0A || c == 0x0C || c == 0x0D || c == 0x20;
        };
        for (;;) {
            while (m_pos < m_data.size() && isPdfWhitespace(m_data[static_cast<qsizetype>(m_pos)])) {
                ++m_pos;
            }
            if (m_pos < m_data.size() && m_data[static_cast<qsizetype>(m_pos)] == '%') {
                while (m_pos < m_data.size()) {
                    auto c = static_cast<unsigned char>(m_data[static_cast<qsizetype>(m_pos)]);
                    if (c == 0x0A || c == 0x0D) break;
                    ++m_pos;
                }
                if (m_pos < m_data.size()) {
                    auto c = static_cast<unsigned char>(m_data[static_cast<qsizetype>(m_pos)]);
                    if (c == 0x0D) {
                        ++m_pos;
                        if (m_pos < m_data.size() && static_cast<unsigned char>(m_data[static_cast<qsizetype>(m_pos)]) == 0x0A) {
                            ++m_pos;
                        }
                    } else {
                        ++m_pos;
                    }
                }
            } else {
                break;
            }
        }
    }

    void seek(qsizetype pos) noexcept {
        if (pos < 0) pos = 0;
        if (pos > m_data.size()) pos = m_data.size();
        m_pos = pos;
    }

    [[nodiscard]] qsizetype tell() const noexcept { return m_pos; }

    [[nodiscard]] bool eof() const noexcept { return m_pos >= m_data.size(); }

    [[nodiscard]] qsizetype remaining() const noexcept {
        return m_data.size() - m_pos;
    }

    [[nodiscard]] QByteArrayView readLine() noexcept {
        if (m_pos >= m_data.size()) return QByteArrayView{};
        qsizetype start = m_pos;
        while (m_pos < m_data.size()) {
            auto c = static_cast<unsigned char>(m_data[static_cast<qsizetype>(m_pos)]);
            if (c == 0x0A) {
                qsizetype lineEnd = m_pos;
                ++m_pos;
                return m_data.sliced(static_cast<qsizetype>(start), static_cast<qsizetype>(lineEnd - start));
            }
            if (c == 0x0D) {
                qsizetype lineEnd = m_pos;
                ++m_pos;
                if (m_pos < m_data.size() && static_cast<unsigned char>(m_data[static_cast<qsizetype>(m_pos)]) == 0x0A) {
                    ++m_pos;
                }
                return m_data.sliced(static_cast<qsizetype>(start), static_cast<qsizetype>(lineEnd - start));
            }
            ++m_pos;
        }
        return m_data.sliced(static_cast<qsizetype>(start), static_cast<qsizetype>(m_pos - start));
    }

    [[nodiscard]] QByteArrayView viewAt(qsizetype pos, qsizetype count) const noexcept {
        if (pos < 0 || count < 0) return QByteArrayView{};
        if (count == 0) return QByteArrayView{};
        if (pos + count > m_data.size()) return QByteArrayView{};
        return m_data.sliced(static_cast<qsizetype>(pos), static_cast<qsizetype>(count));
    }

    [[nodiscard]] QByteArrayView dataView() const noexcept { return m_data; }

private:
    QByteArrayView m_data;
    qsizetype m_pos = 0;
};
