#pragma once

#include <algorithm>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "RuntimeException.hpp"

namespace vmf
{
/** Delimiter tree kept separate from VMF's legacy Tree. */
class RadamsaDelimiterTree
{
public:
    struct Node
    {
        std::pair<uint8_t, uint8_t> delimiter{0u, 0u};
        size_t startIndex{0u};
        size_t endIndex{0u};
        bool needsSeparator{false};
        Node* parent{nullptr};
        std::vector<std::unique_ptr<Node>> children;

        Node(size_t start, std::pair<uint8_t, uint8_t> delim, Node* p = nullptr)
            : delimiter(delim), startIndex(start), endIndex(start), parent(p) {}

        Node(const Node&) = delete;
        Node& operator=(const Node&) = delete;

        std::unique_ptr<Node> deepCopy(Node* newParent = nullptr) const
        {
            auto copy = std::make_unique<Node>(startIndex, delimiter, newParent);
            copy->endIndex = endIndex;
            copy->needsSeparator = needsSeparator;
            std::vector<std::pair<const Node*, Node*>> worklist{{this, copy.get()}};
            while (!worklist.empty())
            {
                const auto [source, destination] = worklist.back();
                worklist.pop_back();
                for (const auto& child : source->children)
                {
                    auto childCopy =
                        std::make_unique<Node>(child->startIndex, child->delimiter, destination);
                    childCopy->endIndex = child->endIndex;
                    childCopy->needsSeparator = child->needsSeparator;
                    Node* childCopyRaw = childCopy.get();
                    destination->children.push_back(std::move(childCopy));
                    worklist.push_back({child.get(), childCopyRaw});
                }
            }
            return copy;
        }
    };

private:
    static constexpr size_t maxInputBytes{1u << 20};
    std::string originalData;
    std::unique_ptr<Node> root;

    static constexpr std::pair<uint8_t, uint8_t> delimiters[] = {
        {40u, 41u}, {91u, 93u}, {60u, 62u},
        {123u, 125u}, {34u, 34u}, {39u, 39u}};

    static std::optional<std::pair<uint8_t, uint8_t>> openingDelimiter(uint8_t byte)
    {
        for (const auto& delimiter : delimiters)
            if (delimiter.first == byte) return delimiter;
        return std::nullopt;
    }

    static std::optional<std::pair<uint8_t, uint8_t>> closingDelimiter(uint8_t byte)
    {
        for (const auto& delimiter : delimiters)
            if (delimiter.second == byte) return delimiter;
        return std::nullopt;
    }

    static bool isBinarish(const std::string& data)
    {
        size_t printableBytes{0u};
        for (const unsigned char byte : data)
        {
            if (printableBytes == 8u) return false;
            if (byte == 0u || (byte & 0x80u) != 0u) return true;
            ++printableBytes;
        }
        return false;
    }

    void build(const std::string& data)
    {
        if (data.size() > maxInputBytes)
            throw RuntimeException{"Delimiter tree input is too large", RuntimeException::UNEXPECTED_ERROR};
        if (isBinarish(data))
            throw RuntimeException{"Delimiter tree input is binary-like", RuntimeException::UNEXPECTED_ERROR};

        originalData = data;
        root = std::make_unique<Node>(0u, std::pair<uint8_t, uint8_t>{0u, 0u});
        root->endIndex = data.size();
        std::vector<std::unique_ptr<Node>> stack;

        for (size_t index = 0u; index < data.size(); ++index)
        {
            const uint8_t byte = static_cast<uint8_t>(data[index]);
            const auto close = closingDelimiter(byte);
            if (close && !stack.empty() &&
                stack.back()->delimiter.first == close->first &&
                index != stack.back()->startIndex)
            {
                auto node = std::move(stack.back());
                stack.pop_back();
                node->endIndex = index + 1u;
                node->delimiter = *close;
                Node* parent = stack.empty() ? root.get() : stack.back().get();
                node->parent = parent;
                parent->children.push_back(std::move(node));
            }
            else if (const auto open = openingDelimiter(byte))
            {
                stack.push_back(std::make_unique<Node>(
                    index, std::pair<uint8_t, uint8_t>{open->first, 0u}));
            }
            else
            {
                auto leaf =
                    std::make_unique<Node>(index, std::pair<uint8_t, uint8_t>{0u, 0u});
                leaf->endIndex = index + 1u;
                Node* parent = stack.empty() ? root.get() : stack.back().get();
                leaf->parent = parent;
                parent->children.push_back(std::move(leaf));
            }
        }

        // Append unmatched opening nodes to the synthetic root.
        for (auto& node : stack)
        {
            node->parent = root.get();
            root->children.push_back(std::move(node));
        }
    }

    static size_t childPosition(const Node* node)
    {
        if (node == nullptr || node->parent == nullptr)
            throw RuntimeException{"Cannot swap the synthetic root", RuntimeException::USAGE_ERROR};
        const auto& siblings = node->parent->children;
        const auto it = std::find_if(
            siblings.begin(), siblings.end(),
            [node](const auto& child) { return child.get() == node; });
        if (it == siblings.end())
            throw RuntimeException{"Node is not present in its parent", RuntimeException::USAGE_ERROR};
        return static_cast<size_t>(it - siblings.begin());
    }

    static bool isAncestor(const Node* ancestor, const Node* node)
    {
        for (const Node* current = node->parent; current; current = current->parent)
            if (current == ancestor) return true;
        return false;
    }

public:
    RadamsaDelimiterTree() = default;
    RadamsaDelimiterTree(const RadamsaDelimiterTree&) = delete;
    RadamsaDelimiterTree& operator=(const RadamsaDelimiterTree&) = delete;
    RadamsaDelimiterTree(RadamsaDelimiterTree&&) noexcept = default;
    RadamsaDelimiterTree& operator=(RadamsaDelimiterTree&&) noexcept = default;

    static std::optional<RadamsaDelimiterTree> tryBuild(const std::string& data) noexcept
    {
        std::optional<RadamsaDelimiterTree> result;
        try
        {
            result.emplace();
            result->build(data);
        }
        catch (...)
        {
            result.reset();
        }
        return result;
    }

    void collectCandidates(std::vector<Node*>& candidates) const
    {
        if (!root) return;
        std::vector<Node*> worklist;
        for (auto it = root->children.rbegin(); it != root->children.rend(); ++it)
            worklist.push_back(it->get());

        while (!worklist.empty())
        {
            Node* node = worklist.back();
            worklist.pop_back();
            if (node->startIndex != node->endIndex &&
                node->delimiter != std::pair<uint8_t, uint8_t>{0u, 0u})
                candidates.push_back(node);
            for (auto it = node->children.rbegin(); it != node->children.rend(); ++it)
                worklist.push_back(it->get());
        }
    }
    void collectSwapCandidates(std::vector<Node*>& candidates) const
    {
        collectCandidates(candidates);
    }

    /** Repeat the path selected by a production-selectable delimiter node. */
    void repeatSelectedPath(
        Node* selectedNode,
        size_t repeatCount,
        size_t maxTotalNodes,
        size_t maxOutputBytes)
    {
        if (!selectedNode || !selectedNode->parent) return;
        Node* selectedParent = selectedNode->parent;
        if (selectedParent == root.get()) return;

        const size_t selectedChildIndex = childPosition(selectedNode);
        const auto measure = [this](const Node* subtree) {
            std::pair<size_t, size_t> totals{0u, 0u};
            std::vector<const Node*> worklist{subtree};
            while (!worklist.empty())
            {
                const Node* node = worklist.back();
                worklist.pop_back();
                if (totals.first != std::numeric_limits<size_t>::max()) ++totals.first;
                size_t nodeBytes{0u};
                if (node->children.empty())
                    nodeBytes = node->endIndex - node->startIndex;
                else
                {
                    nodeBytes = static_cast<size_t>(node->needsSeparator);
                    nodeBytes += static_cast<size_t>(node->delimiter.first != 0u);
                    nodeBytes += static_cast<size_t>(node->delimiter.second != 0u);
                }
                if (std::numeric_limits<size_t>::max() - totals.second < nodeBytes)
                    totals.second = std::numeric_limits<size_t>::max();
                else
                    totals.second += nodeBytes;
                for (const auto& child : node->children) worklist.push_back(child.get());
            }
            return totals;
        };

        const auto whole = measure(root.get());
        const auto parent = measure(selectedParent);
        const auto child = measure(selectedNode);
        const size_t nodeDelta = parent.first - child.first;
        const size_t byteDelta = parent.second - child.second;

        if (maxTotalNodes != std::numeric_limits<size_t>::max())
        {
            if (whole.first > maxTotalNodes) return;
            if (nodeDelta != 0u)
                repeatCount = std::min(repeatCount, (maxTotalNodes - whole.first) / nodeDelta);
        }
        if (maxOutputBytes != std::numeric_limits<size_t>::max())
        {
            if (whole.second > maxOutputBytes) return;
            if (byteDelta != 0u)
                repeatCount = std::min(repeatCount, (maxOutputBytes - whole.second) / byteDelta);
        }

        Node* currentParent = selectedParent;
        for (size_t repetition = 0u; repetition < repeatCount; ++repetition)
        {
            auto parentCopy = currentParent->deepCopy(currentParent);
            currentParent->children[selectedChildIndex] = std::move(parentCopy);
            currentParent = currentParent->children[selectedChildIndex].get();
        }
    }

    /** Delete a production-selectable delimiter subtree. */
    bool deleteSelectedNode(Node* selectedNode)
    {
        if (!selectedNode || !selectedNode->parent) return false;
        Node* parent = selectedNode->parent;

        // The synthetic root cannot be resolved through Node::get_mut().
        if (parent == root.get()) return false;

        const size_t position = childPosition(selectedNode);
        parent->children.erase(
            parent->children.begin() +
                static_cast<std::vector<std::unique_ptr<Node>>::difference_type>(position));
        return true;
    }

    /** Duplicate a production-selectable delimiter subtree beside itself. */
    bool duplicateSelectedNode(
        Node* selectedNode,
        size_t maxTotalNodes,
        size_t maxOutputBytes)
    {
        if (!selectedNode || !selectedNode->parent) return false;
        Node* parent = selectedNode->parent;

        // The synthetic root cannot be resolved through Node::get_mut().
        if (parent == root.get()) return false;

        const auto measure = [](const Node* subtree) {
            std::pair<size_t, size_t> totals{0u, 0u};
            std::vector<const Node*> worklist{subtree};
            while (!worklist.empty())
            {
                const Node* node = worklist.back();
                worklist.pop_back();
                if (totals.first == std::numeric_limits<size_t>::max())
                    return totals;
                ++totals.first;

                const size_t nodeBytes = node->children.empty()
                    ? node->endIndex - node->startIndex
                    : static_cast<size_t>(node->needsSeparator)
                        + static_cast<size_t>(node->delimiter.first != 0u)
                        + static_cast<size_t>(node->delimiter.second != 0u);
                if (std::numeric_limits<size_t>::max() - totals.second < nodeBytes)
                    totals.second = std::numeric_limits<size_t>::max();
                else
                    totals.second += nodeBytes;
                for (const auto& child : node->children) worklist.push_back(child.get());
            }
            return totals;
        };

        auto duplicate = selectedNode->deepCopy(parent);
        duplicate->needsSeparator = selectedNode->startIndex > 0u &&
            originalData[selectedNode->startIndex - 1u] == ',';

        const auto whole = measure(root.get());
        const auto added = measure(duplicate.get());
        if (maxTotalNodes != std::numeric_limits<size_t>::max() &&
            (whole.first > maxTotalNodes || added.first > maxTotalNodes - whole.first))
            return false;
        if (maxOutputBytes != std::numeric_limits<size_t>::max() &&
            (whole.second > maxOutputBytes || added.second > maxOutputBytes - whole.second))
            return false;

        const size_t position = childPosition(selectedNode);
        parent->children.insert(
            parent->children.begin() +
                static_cast<std::vector<std::unique_ptr<Node>>::difference_type>(position + 1u),
            std::move(duplicate));
        return true;
    }

    size_t serializedSize() const
    {
        if (!root) return 0u;
        size_t total{0u};
        std::vector<const Node*> worklist{root.get()};
        while (!worklist.empty())
        {
            const Node* node = worklist.back();
            worklist.pop_back();
            size_t nodeBytes = node->children.empty()
                ? node->endIndex - node->startIndex
                : static_cast<size_t>(node->needsSeparator)
                    + static_cast<size_t>(node->delimiter.first != 0u)
                    + static_cast<size_t>(node->delimiter.second != 0u);
            if (std::numeric_limits<size_t>::max() - total < nodeBytes)
                return std::numeric_limits<size_t>::max();
            total += nodeBytes;
            for (const auto& child : node->children) worklist.push_back(child.get());
        }
        return total;
    }


    /** Replace a destination node with a deep copy of the complete source subtree. */
    void replaceNode(Node* destination, const Node* source)
    {
        if (!destination || !source)
            throw RuntimeException{"Both nodes must not be nullptr", RuntimeException::USAGE_ERROR};
        if (destination == source) return;

        Node* parent = destination->parent;
        const size_t position = childPosition(destination);
        auto replacement = source->deepCopy(parent);
        parent->children[position] = std::move(replacement);
    }

    void swapNodes(Node* node1, Node* node2)
    {
        if (!node1 || !node2)
            throw RuntimeException{"Both nodes must not be nullptr", RuntimeException::USAGE_ERROR};
        if (node1 == node2) return;

        if (isAncestor(node1, node2))
        {
            Node* parent = node1->parent;
            const size_t position = childPosition(node1);
            parent->children[position] = node2->deepCopy(parent);
            return;
        }
        if (isAncestor(node2, node1)) return;

        Node* parent1 = node1->parent;
        Node* parent2 = node2->parent;
        const size_t position1 = childPosition(node1);
        const size_t position2 = childPosition(node2);
        std::swap(parent1->children[position1], parent2->children[position2]);
        parent1->children[position1]->parent = parent1;
        parent2->children[position2]->parent = parent2;
    }

    std::string toString() const
    {
        if (!root) return "";
        std::string output;
        struct Frame { const Node* node; size_t nextChild; bool entered; };
        std::vector<Frame> stack{{root.get(), 0u, false}};

        while (!stack.empty())
        {
            Frame& frame = stack.back();
            const Node* node = frame.node;
            if (!frame.entered)
            {
                frame.entered = true;
                if (node->children.empty())
                {
                    output.append(originalData, node->startIndex, node->endIndex - node->startIndex);
                    stack.pop_back();
                    continue;
                }
                if (node->needsSeparator) output.push_back(',');
                if (node->delimiter.first)
                    output.push_back(static_cast<char>(node->delimiter.first));
            }

            if (frame.nextChild < node->children.size())
            {
                const Node* child = node->children[frame.nextChild++].get();
                stack.push_back({child, 0u, false});
                continue;
            }
            if (node->delimiter.second)
                output.push_back(static_cast<char>(node->delimiter.second));
            stack.pop_back();
        }
        return output;
    }
};
}
