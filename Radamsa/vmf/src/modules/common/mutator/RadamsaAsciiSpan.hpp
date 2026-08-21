#pragma once

#include "RuntimeException.hpp"
#include "VmfRand.hpp"
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace vmf::ascii_detail
{
using Byte = uint8_t;

struct TextSpan
{
    size_t begin;
    size_t end;
};

struct DataSpan
{
    bool texty;
    std::vector<TextSpan> elements;
};

struct Edit
{
    size_t begin{0u};
    size_t end{0u};
    std::vector<Byte> replacement;
};

inline const std::vector<std::vector<Byte>>& sillyStrings()
{
    static const std::vector<std::string> strings = {
        "%n", "%n", "%s", "%d", "%p", "%#x", "\\00", "aaaa%d%n",
        "`xcalc`", ";xcalc", "$(xcalc)", "!xcalc", "\"xcalc", "'xcalc",
        "\\x00", "\\r\\n", "\\r", "\\n", "\\x0a", "\\x0d",
        "NaN", "+inf", "$PATH", "$!!", "!!", "&#000;", "\\u0000",
        "$&", "$+", "$`", "$'", "$1"
    };
    static const std::vector<std::vector<Byte>> bytes = [] {
        std::vector<std::vector<Byte>> result;
        result.reserve(strings.size());
        for (const std::string& value : strings)
            result.emplace_back(value.begin(), value.end());
        return result;
    }();
    return bytes;
}

class SpanAscii
{
public:
    static std::optional<SpanAscii> parse(const std::vector<Byte>& input)
    {
        constexpr size_t minimumTexty{6u};
        size_t firstEnd{0u};
        while (firstEnd < input.size() && isTexty(input[firstEnd]))
            ++firstEnd;
        if (firstEnd < minimumTexty)
            return std::nullopt;

        SpanAscii parsed(input);
        parsed.addTextRun(0u, firstEnd);
        size_t position{firstEnd};
        while (position < input.size())
        {
            size_t runEnd{position};
            while (runEnd < input.size() && isTexty(input[runEnd]))
                ++runEnd;
            if (runEnd - position >= minimumTexty)
            {
                parsed.addTextRun(position, runEnd);
                position = runEnd;
                continue;
            }

            // Rust coalesces all material that cannot begin a six-byte text
            // run into one Data::Bytes candidate.
            position += runEnd > position ? runEnd - position : 1u;
            while (position < input.size())
            {
                runEnd = position;
                while (runEnd < input.size() && isTexty(input[runEnd]))
                    ++runEnd;
                if (runEnd - position >= minimumTexty)
                    break;
                position += runEnd > position ? runEnd - position : 1u;
            }
            parsed.chunks.push_back(DataSpan{false, {}});
        }
        return parsed;
    }

    void mutate(VmfRand* random, size_t insertionCap)
    {
        // Rust chooses from every Data chunk and retries when it finds Bytes.
        size_t chunkIndex;
        do
        {
            chunkIndex = static_cast<size_t>(random->randBetween(
                0ul, static_cast<unsigned long>(chunks.size() - 1u)));
        } while (!chunks[chunkIndex].texty);

        const auto& elements = chunks[chunkIndex].elements;
        const size_t elementIndex = static_cast<size_t>(random->randBetween(
            0ul, static_cast<unsigned long>(elements.size() - 1u)));
        const TextSpan& element{elements[elementIndex]};
        apply(element,
              static_cast<size_t>(random->randBetween(
                  0ul, static_cast<unsigned long>(element.end - element.begin))),
              static_cast<size_t>(random->randBetween(0ul, 2ul)),
              random, insertionCap);
    }

    size_t serializedSize() const noexcept
    {
        return source.size() - (edit.end - edit.begin) + edit.replacement.size();
    }

    void serialize(char* output) const
    {
        if (edit.begin != 0u)
            std::memcpy(output, source.data(), edit.begin);
        if (!edit.replacement.empty())
            std::memcpy(output + edit.begin, edit.replacement.data(),
                        edit.replacement.size());
        const size_t suffixSize{source.size() - edit.end};
        if (suffixSize != 0u)
            std::memcpy(output + edit.begin + edit.replacement.size(),
                        source.data() + edit.end, suffixSize);
    }

private:
    explicit SpanAscii(const std::vector<Byte>& input) : source(input) {}

    static bool isTexty(Byte value) noexcept
    {
        return value == 9u || value == 10u || value == 13u ||
               (value >= 32u && value <= 126u);
    }

    void addTextRun(size_t begin, size_t end)
    {
        DataSpan chunk{true, {}};
        size_t position{begin};
        size_t plainBegin{begin};
        while (position < end)
        {
            const Byte delimiter{source[position]};
            if (delimiter != 39u && delimiter != 34u)
            {
                ++position;
                continue;
            }
            size_t closing{position + 1u};
            while (closing < end)
            {
                if (source[closing] == 92u)
                {
                    if (closing + 1u >= end)
                    {
                        closing = end;
                        break;
                    }
                    closing += 2u;
                }
                else if (source[closing] == delimiter)
                    break;
                else
                    ++closing;
            }
            if (closing == end)
            {
                ++position;
                continue;
            }
            if (plainBegin < position)
                chunk.elements.push_back(TextSpan{plainBegin, position});
            chunk.elements.push_back(TextSpan{position + 1u, closing});
            position = closing + 1u;
            plainBegin = position;
        }
        if (plainBegin < end)
            chunk.elements.push_back(TextSpan{plainBegin, end});
        chunks.push_back(std::move(chunk));
    }

    void apply(const TextSpan& element, size_t byteIndex, size_t mutationType,
               VmfRand* random, size_t insertionCap)
    {
        std::vector<Byte> replacement;
        if (mutationType < 2u)
        {
            const auto& values = sillyStrings();
            const size_t count = static_cast<size_t>(random->randBetween(1ul, 19ul));
            replacement.reserve(count * 8u);
            for (size_t i = 0u; i < count; ++i)
            {
                const auto& value = values[static_cast<size_t>(random->randBetween(
                    0ul, static_cast<unsigned long>(values.size() - 1u)))];
                replacement.insert(replacement.end(), value.begin(), value.end());
            }
        }
        else
        {
            const size_t choice = static_cast<size_t>(random->randBetween(0ul, 10ul));
            static constexpr size_t counts[] = {
                127u, 128u, 255u, 256u, 16383u,
                16384u, 32767u, 32768u, 65535u, 65536u
            };
            size_t count = choice < 10u ? counts[choice] :
                static_cast<size_t>(random->randBetween(0ul, 1023ul));
            count = std::min(count, insertionCap);
            replacement.assign(count, Byte(10u));
        }

        const size_t absolute{element.begin + byteIndex};
        edit = Edit{absolute, mutationType == 1u ? element.end : absolute,
                    std::move(replacement)};
    }

    const std::vector<Byte>& source;
    std::vector<DataSpan> chunks;
    Edit edit;
};
}
