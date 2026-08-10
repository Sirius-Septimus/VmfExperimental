/* =============================================================================
 * Vader Modular Fuzzer (VMF)
 * Copyright (c) 2021-2026 The Charles Stark Draper Laboratory, Inc.
 * <vmf@draper.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 (only) as 
 * published by the Free Software Foundation.
 *  
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *  
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *  
 * @license GPL-2.0-only <https://spdx.org/licenses/GPL-2.0-only.html>
 * ===========================================================================*/

#pragma once

#include "RadamsaMutatorBase.hpp"
#include "VmfRand.hpp"

#include <vector>

namespace vmf
{
/**
 * @brief Base class for all Radamsa Line Mutators. A line is defined as a collection of bytes
 * that is ended by a newline character.
 */
class RadamsaLineMutatorBase: public RadamsaMutatorBase
{
public:
    /**
     * @struct Line
     * 
     * @brief This struct contains the information about a specific line contained in a data buffer.
     */
    struct Line
    {
        Line() = default; 
        ~Line() = default;

        Line(Line&&) = default; /**<Copy constructor for Line type*/
        Line(const Line&) = default; /**<const Copy constructor for Line type*/

        Line& operator=(Line&&) = default; /**<Move constructor for Line type*/
        Line& operator=(const Line&) = default; /**<const Move constructor for Line type*/

        bool operator==(const Line &other) const { /**<Equality operator for Line type*/
            return (IsValid == other.IsValid && 
                    StartIndex == other.StartIndex && 
                    Size == other.Size); 
        }
        bool operator!=(const Line &other) const { return !(*this == other); } /**<Identity operator for Line type*/

        bool IsValid{false}; /**<Is the line valid*/
        size_t StartIndex{0};/**<Starting Index of the line*/
        size_t Size{0}; /**<Size of the line*/
    };
    /**
     * @struct LineVector
     * 
     * @brief A struct that contains the content of a line. 
     */
    struct LineVector
    {
        ~LineVector() = default;
        LineVector() = default;

        /** 
        * @brief Constructor for LineVector type
        * */
        LineVector(
            const char* const buffer,
            const Line& lineData)
        {
            Data = std::make_unique<char[]>(lineData.Size);
            memcpy(Data.get(), &buffer[lineData.StartIndex], lineData.Size);

            Size = lineData.Size;
        }
        /** 
        * @brief Copy constructor for LineVector type
        * */
        LineVector(const LineVector &other) noexcept
        {
            // Copy Data

            const size_t& size{other.Size};

            Data = std::make_unique<char[]>(size);
            memcpy(Data.get(), other.Data.get(), size);

            Size = size;
        }

        /**
         * @brief Move constructor operator for LineVector. Transfers ownership of `other.Data` via std::unique_ptr's move-constructor. 
         * After the move, `other.Data == nullptr` and `other.Size == 0`.
         */
        LineVector(LineVector&& other) noexcept
            : Data{std::move(other.Data)}, Size{other.Size}
        {
            other.Size = 0u;
        }

        /**
         * @brief Copy assignment operator for LineVector.
         */
        LineVector& operator=(const LineVector& other)
        {
            // Copy Data

            const size_t& size{other.Size};

            Data = std::make_unique<char[]>(size);
            memcpy(Data.get(), other.Data.get(), size);

            Size = size;

            return *this;
        }

        /** 
        * @brief Move assignment operator for LineVector. Transfers ownership of `other.Data` via std::unique_ptr's move-assignment.
        * After the move, `other.Data == nullptr` and `other.Size == 0`.
        */
        LineVector& operator=(LineVector&& other) noexcept
        {
            if (this != &other)
            {
                Data = std::move(other.Data);
                Size = other.Size;
                other.Size = 0u;
            }
            return *this;
        }

        /**
         * @brief Equality operator for LineVector.
         */
        bool operator==(const LineVector &other) const { 
            return (Size == other.Size && 
                    (memcmp(Data.get(), other.Data.get(), other.Size) == 0)); 
        }

        /**
         * @brief Inequality operator for LineVector.
         */
        bool operator!=(const LineVector &other) const { return !(*this == other); } // we may want to change this to test for equality instead of identity

        std::unique_ptr<char[]> Data{nullptr}; /**<Raw data pointer */
        size_t Size{0u}; /**<Size of the data */
    };
    /**
     * @struct LineList
     * 
     * @brief A struct that defines a group of Lines and their data vectors in contiguous order. 
     */
    struct LineList
    {
        ~LineList() = default;
        LineList() = default;

        /**
         * @brief Constructor for a LineList using a raw data buffer. 
         */
        LineList(
            const char* const buffer,
            const std::vector<Line>& lineData) noexcept
        {
            const size_t numberOfElements{lineData.size()};

            auto data{std::make_unique<LineVector[]>(numberOfElements)};

            for(size_t it{0u}; it < numberOfElements; ++it)
            {
                const Line& line{lineData.at(it)};

                data[it] = LineVector{buffer,line};

                Capacity += line.Size;
            }

            NumberOfElements = numberOfElements;
            Data = std::move(data);
        }

        /**
         * @brief Copy constructor for a LineList. 
         */
        LineList(const LineList& other) noexcept
        {
            // Copy Data

            const size_t& numberOfElements{other.NumberOfElements};

            NumberOfElements = numberOfElements;
            Capacity = other.Capacity;

            Data = std::make_unique<LineVector[]>(numberOfElements);

            for (size_t it{0u}; it < numberOfElements; ++it)
            {
                Data[it] = other.Data[it];
            }
        }
        /**
        * @brief Move Constructor for LineList. 
        * 
        * Transfers ownership of `other.Data` via std::unique_ptr's move-constructor.
        * After the move, `other.Data == nullptr`, `other.Capacity == 0`, and `other.NumberOfElements == 0`.
        */ 
        LineList(LineList&& other) noexcept
            : Data{std::move(other.Data)},
              NumberOfElements{other.NumberOfElements},
              Capacity{other.Capacity}
        {
            other.NumberOfElements = 0u;
            other.Capacity = 0u;
        }

        /**
         * @brief Copy assignment operator for LineList.
         */
        LineList& operator=(const LineList& other)
        {
            // Copy Data

            const size_t& numberOfElements{other.NumberOfElements};

            NumberOfElements = numberOfElements;
            Capacity = other.Capacity;

            Data = std::make_unique<LineVector[]>(numberOfElements);
            size_t count = numberOfElements;
            for(size_t i=0; i<count; i++)
            {
                Data[i] = other.Data[i];
            }

            return *this;
        }


        /**
         * @brief Move-assigns by transferring ownership of `other.Data` via std::unique_ptr's move-assignment.
         */
        LineList& operator=(LineList&& other) noexcept
        {
            if (this != &other)
            {
                Data = std::move(other.Data);
                NumberOfElements = other.NumberOfElements;
                Capacity = other.Capacity;
                other.NumberOfElements = 0u;
                other.Capacity = 0u;
            }
            return *this;
        }

        /**
         * @brief Comparison operator for LineList Type.
         */
        bool operator==(const LineList& other) const
        {
            auto compareLineVectors{
                [&](const LineVector* const lhs, const LineVector* const rhs) -> bool
                {
                    if (lhs != nullptr && rhs != nullptr)
                    {
                        for (size_t it{0u}; it < lhs->Size; ++it)
                            if (lhs->Data[it] != rhs->Data[it])
                                return false;

                        return true;
                    }
                    else if (lhs == nullptr && rhs == nullptr)
                    {
                        return true;
                    }
                    else
                    {
                        return false;
                    }
                }
            };

            return ((NumberOfElements == other.NumberOfElements) &&
                    (Capacity == other.Capacity) &&
                    (compareLineVectors(Data.get(), other.Data.get())));
        }

        /**
         * @brief Inequality operator for LineList Type.
         */
        bool operator!=(const LineList& other) const { return !(*this == other); }

        std::unique_ptr<LineVector[]> Data{nullptr}; /**<Pointer to line data */
        size_t NumberOfElements{0u}; /**<Number of lines currently in the vector*/
        size_t Capacity{0u}; /**<Number of lines that can be held in this vector */
    };

    RadamsaLineMutatorBase() = default;
    virtual ~RadamsaLineMutatorBase() = default;

    /**
     * @brief Obtains data of a line at a specific index in a data buffer.
     * 
     * @param buffer Input buffer to search
     * @param size The size of the input buffer
     * @param lineIndex Line iandex to find data for
     * @param numberOfLinesAfterIndex The number of lines that comes after the target index
     */
    Line GetLineData(
                     const char* const buffer,
                     const size_t size,
                     const size_t lineIndex,
                     const size_t numberOfLinesAfterIndex)
    {
        constexpr size_t minimumSize{1u};

        if (size < minimumSize)
            throw RuntimeException{"The buffer's minimum size must be greater than or equal to 1", RuntimeException::USAGE_ERROR};

        if (buffer == nullptr)
            throw RuntimeException{"Input buffer is null", RuntimeException::UNEXPECTED_ERROR};

        const size_t totalNumberOfLines{
                                    GetNumberOfLinesAfterIndex(
                                                            buffer,
                                                            size,
                                                            0u)};
        if (lineIndex >= totalNumberOfLines)
            throw RuntimeException{"Line index exceeds the maximum number of lines", RuntimeException::UNEXPECTED_ERROR};

        if (numberOfLinesAfterIndex > totalNumberOfLines)
            throw RuntimeException{"Number of lines after index exceeds the maximum number of lines", RuntimeException::UNEXPECTED_ERROR};

        const size_t lineOffset{totalNumberOfLines - numberOfLinesAfterIndex};

        constexpr size_t lower{0u};
        const size_t upper{totalNumberOfLines - 1u};
        const size_t maximumLineIndex{
                                std::clamp(
                                        lineIndex + lineOffset,
                                        lower,
                                        upper)};

        Line lineData;

        for(size_t it{0u}, reverseLineIndex{maximumLineIndex}; it < size; ++it)
        {
            if(reverseLineIndex == 0u)
            {
                if(!lineData.IsValid)
                {
                    lineData.StartIndex = it;
                    lineData.IsValid = true;
                }

                ++lineData.Size;

                if(buffer[it] == '\n')
                    break;
            }
            else
            {
                if(buffer[it] == '\n')
                    --reverseLineIndex;
            }
        }

        return lineData;
    }

    /**
     * @brief Obtains all line ranges in a buffer using a single forward scan.
     *
     * Only newline-terminated segments are returned. Trailing bytes without a
     * newline are intentionally ignored to match rusty-radamsa.
     *
     * @param buffer Input buffer to search
     * @param size The size of the input buffer
     */
    std::vector<Line> GetAllLineData(
                                   const char* const buffer,
                                   const size_t size)
    {
        constexpr size_t minimumSize{1u};

        if (size < minimumSize)
            throw RuntimeException{"The buffer's minimum size must be greater than or equal to 1", RuntimeException::USAGE_ERROR};

        if (buffer == nullptr)
            throw RuntimeException{"Input buffer is null", RuntimeException::UNEXPECTED_ERROR};

        // Match rusty-radamsa: only newline-terminated lines are materialized.
        // Trailing bytes without a newline are intentionally ignored.
        std::vector<Line> lines;
        lines.reserve(8u);

        Line currentLine;
        currentLine.IsValid = true;
        currentLine.StartIndex = 0u;

        for (size_t it{0u}; it < size; ++it)
        {
            ++currentLine.Size;

            if (buffer[it] == '\n')
            {
                lines.push_back(currentLine);

                currentLine = Line{};
                currentLine.IsValid = true;
                currentLine.StartIndex = it + 1u;
            }
        }

        return lines;
    }

    /**
     * @brief Computes the total byte length of a line list.
     *
     * The result is the serialized byte count without any added terminator.
     *
     * @param lines The line metadata to sum
     */
    size_t GetAllLineDataSize(const std::vector<Line>& lines) const
    {
        size_t total{0u};
        for (const Line& line : lines)
        {
            total += line.Size;
        }
        return total;
    }

    /**
     * @brief Computes the serialized byte length of a line list in the provided order.
     *
     * @param lines The line metadata to sum
     * @param lineOrder The order in which the lines will be serialized
     */
    size_t GetAllLineDataSize(const std::vector<Line>& lines, const std::vector<size_t>& lineOrder) const
    {
        size_t total{0u};
        for (const size_t lineIndex : lineOrder)
        {
            total += lines.at(lineIndex).Size;
        }
        return total;
    }

    /**
     * @brief Copies a sequence of lines into an output buffer in the provided order.
     *
     * @param originalBuffer Source buffer that contains the line bytes
     * @param lines Line metadata describing the copied ranges
     * @param lineOrder Order in which to serialize the lines
     * @param newBuffer Destination buffer that receives the serialized line data
     */
    template<typename TOrderContainer>
    void CopyAllLineDataToBuffer(
        const char* const originalBuffer,
        const std::vector<Line>& lines,
        const TOrderContainer& lineOrder,
        char* newBuffer) const
    {
        size_t nextBufferIndex{0u};
        for (const size_t lineIndex : lineOrder)
        {
            const Line& line{lines.at(lineIndex)};
            memcpy(
                newBuffer + nextBufferIndex,
                originalBuffer + line.StartIndex,
                line.Size
            );
            nextBufferIndex += line.Size;
        }
    }

    /**
     * @brief Obtains the number of newline-terminated lines after a specific index.
     *
     * @param buffer Data buffer to search through
     * @param size Size of the provided data buffer
     * @param index Index to start the line search at
     */
    size_t GetNumberOfLinesAfterIndex(
                                      const char* const buffer,
                                      const size_t size,
                                      const size_t index)
    {
        constexpr size_t minimumSize{1u};

        if (size < minimumSize)
            throw RuntimeException{"The buffer's minimum size must be greater than or equal to 1", RuntimeException::USAGE_ERROR};

        if (index > size - 1u)
            throw RuntimeException{"Index is out of bounds", RuntimeException::INDEX_OUT_OF_RANGE};

        if (buffer == nullptr)
            throw RuntimeException{"Input buffer is null", RuntimeException::UNEXPECTED_ERROR};

        size_t numberOfLines{0u};

        for(size_t it{index}; it < size; ++it)
            if(buffer[it] == '\n')
                ++numberOfLines;

        return numberOfLines;
    }

    /**
     * @brief Checks to see if a given buffer contains contains UTF-8 or a \0.
     * 
     * @param buffer Data buffer to search through
     * @param size Size of the data buffer
     */
    bool IsBinarish(
                    const char* const buffer,
                    const size_t size)
{
    constexpr size_t minimumSize{1u};

    if (size < minimumSize)
        throw RuntimeException{"The buffer's minimum size must be greater than or equal to 1", RuntimeException::USAGE_ERROR};

    if (buffer == nullptr)
        throw RuntimeException{"Input buffer is null", RuntimeException::UNEXPECTED_ERROR};

    constexpr size_t binarishPeekSize{8u};

    for(size_t it{0}; it < binarishPeekSize; ++it)
    {

        if(it == size)
            break;

        if(buffer[it] == '\0')
            return true;

        if((buffer[it] & (std::numeric_limits<char>::max() + 0x01)) != 0u)
            return true;
    }

    return false;
}
};
}
