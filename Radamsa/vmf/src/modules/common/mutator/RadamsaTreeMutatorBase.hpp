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

#include <stack>
#include <optional>
#include <limits>
#include "RadamsaMutatorBase.hpp"
#include "RuntimeException.hpp"
#include "VmfRand.hpp"

using std::string;

namespace vmf
{
/**
 * @brief Base class for all Node/Path type mutators. Contains helper functions for handling and creatings trees/nodes.
 */
class RadamsaTreeMutatorBase: public RadamsaMutatorBase
{
public:
    RadamsaTreeMutatorBase() = default;
    virtual ~RadamsaTreeMutatorBase() = default;

    /**
     * @struct Node
     * 
     * @brief A node of a N-ary tree. Contains a data value, a parent pointer, and 0 or more children.
     */
    struct Node
    {
        string value;/**<String value of this node*/
        Node* parent;/**<Pointer to the parent of this node. NULL if this node is the root.*/
        std::vector<Node*> children;/**<Vector containing 0 or more nodes(children)*/

        /**
         * @brief Node constructor
         * 
         * @param v Value to be stored in this Node
         * @param p A pointer to the parent Node. Set to nullptr if this Node is the root
         */
        Node(string v, Node* p = nullptr) : value(v), parent(p) {}; /*< Node constructor*/

        // Disable copy constructor and copy assignment
        Node(const Node&) = delete;
        Node& operator=(const Node&) = delete;

        ~Node() {
            // DO NOT delete parent; parent owns this node, not the other way around

            // copying to avoid iterator invalidation via deleting an element while still looping over the vector
            std::vector<Node*> childrenCopy = this->children;
            for(Node* child : childrenCopy) {
                delete child;
            }

            return;
        }

        /**
         * @brief Creates a independent copy of a Node and all of its children.
         * 
         * @param newParent A pointer to the newly coppied parent Node
         */
        Node* deepCopy(Node* newParent = nullptr) const
        {
            auto selfCopy = std::make_unique<Node>(this->value, newParent);
            for (Node* child : this->children)
            {
                std::unique_ptr<Node> childCopy{child->deepCopy(selfCopy.get())};
                selfCopy->children.push_back(childCopy.get());
                childCopy.release();
            }
            return selfCopy.release();
        }
    };
    /**
     * @struct Tree 
     * 
     * @brief A collection of nodes. A N-ary tree that is able to have 0 or more children. 
     */
    struct Tree
    {
    private:
        static constexpr size_t maxInputBytes{1u << 20};
        static constexpr size_t maxParsedNodes{1u << 19};
        static constexpr size_t maxTreeDepth{4096u};

        /**
         * @brief Parses a Tree from a string in the form ""A(B(C)(D))(E)"". Builds a tree of Nodes using this string.
         * 
         * @param treeStr The string to parsed
         */
        void buildTree(string treeStr) {
            if(treeStr.empty()) {
                throw RuntimeException{"Tree string is empty", RuntimeException::UNEXPECTED_ERROR};
            }

            // Cap the raw parse input before any tree construction work begins.
            if (treeStr.length() > maxInputBytes) {
                throw RuntimeException{"Tree string exceeds the maximum allowed input size", RuntimeException::UNEXPECTED_ERROR};
            }

            std::stack<Node*> stk;
            string value;
            size_t parsedNodes{0u};

            auto pushWithDepthCheck = [&](Node* node) {
                if (stk.size() + 1u > maxTreeDepth) {
                    throw RuntimeException{"Tree depth exceeds the maximum allowed depth", RuntimeException::UNEXPECTED_ERROR};
                }
                stk.push(node);
            };

            auto createNode = [&](const string& nodeValue, Node* parent = nullptr) -> Node* {
                if (parsedNodes >= maxParsedNodes) {
                    throw RuntimeException{"Tree node count exceeds the maximum allowed nodes", RuntimeException::UNEXPECTED_ERROR};
                }

                Node* n = insertNode(nodeValue, parent);
                ++parsedNodes;
                return n;
            };

            for(size_t i = 0; i < treeStr.length(); ++i) {
                unsigned char ch = treeStr[i];

                if(std::isspace(ch)) {
                    continue;
                }
                else if(ch == '(') {
                    if(!value.empty()) {
                        Node* n;
                        if(stk.empty()) {
                            n = createNode(value);
                            this->root = n;
                        }
                        else {
                            n = createNode(value, stk.top());
                        }

                        pushWithDepthCheck(n);
                        value.clear();
                    }
                    else {
                        Node* toPush = nullptr;
                        // upcomming additional child, re-push root
                        if(stk.empty() && this->root != nullptr) {
                            toPush = this->root;
                        }
                        // upcomming additional child, re-push the most recently popped node
                        else if(!stk.empty() && !stk.top()->children.empty()) {
                            toPush = stk.top()->children.back();
                        }

                        if(toPush != nullptr) {
                            pushWithDepthCheck(toPush);
                        }
                        else {
                            throw RuntimeException{"Unexpected open bracket without a parent node", RuntimeException::UNEXPECTED_ERROR};
                        }
                    }
                }
                else if(ch == ')') {
                    if(stk.empty()) {
                        throw RuntimeException{"Unmatched open bracket in tree string", RuntimeException::UNEXPECTED_ERROR};
                    }

                    if(!value.empty()) {
                        createNode(value, stk.top());

                        value.clear();
                    }

                    stk.pop();
                }
                else {
                    value += ch;
                }
            }

            if(!stk.empty()) {
                throw RuntimeException{"Unmatched open bracket in tree string", RuntimeException::UNEXPECTED_ERROR};
            }


            // case for TreeStr consisting of a single root node
            if(!value.empty() && this->root == nullptr) {
                this->root = createNode(value);
            }

            return;
        }
    
    public:
        Node* root = nullptr; /**<Root of the tree */

        Tree() = default;

        Tree(string const&) = delete; /**<The throwing constructor form is removed. Use Tree::tryBuild instead. */

        /**
         * @brief Builds a Tree from `treeStr`. Returns std::nullopt on any parse failure. 
         * Never throws. On parse failure the partially-built Tree's destructor runs before the optional resets, so no Node is leaked.
         * 
         * @param treeStr A string representing a tree in the form Root(Child(Grand-Child))(Child)/ A(B(C)(D))
         */
        [[nodiscard]] static std::optional<Tree> tryBuild(string const& treeStr) noexcept
        {
            std::optional<Tree> result;
            try
            {
                result.emplace();
                result->buildTree(treeStr);
            }
            catch (...)
            {
                result.reset();
            }
            return result;
        }
        
        Tree(const Tree&) = delete; /**<Deleting copy constructor to avoid shallow copies*/
        Tree& operator=(const Tree&) = delete;/**<Deleting copy assignment operator to avoid shallow copies*/

        /**
        * @brief Move constructor for Tree
        */
        Tree(Tree&& other) noexcept {
            root = other.root;
            other.root = nullptr;
        }
        /** 
        * @brief Move assignment operator for Tree
        */
        Tree& operator=(Tree&& other) noexcept {
            if(this != &other) {    // prevents self-assignment
                deleteNode(root);

                root = other.root;
                other.root = nullptr;
            }
            return *this;
        }

        ~Tree() { deleteNode(this->root); }

        /** 
         * @brief Creates a parenthesis-delimited string from the provided subtree.
         * 
         * @param n The root of the subtree of Nodes to be stringified
        */
        
        string toString(Node* n) {
            // 
            if(!n) return "";

            string treeStr;

            // Explicit stack allocated on the heap: holds tuples of {node, child_index, visted}
            std::vector<std::tuple<Node*, size_t, bool>> stack;

            stack.push_back({n, 0, false});

            while (!stack.empty()) {

                // Get a reference to the top item so we can modify its child index
                auto& [curr_node, idx, visted] = stack.back();
                
                //Case 1: Vist parent and place it outside.
                if (curr_node->parent == NULL && !visted) {
                    treeStr += n->value;
                    visted = true;
                    continue;
                }

                //Case 2: A signular node just return the treeStr.
                if (curr_node->parent == NULL && curr_node->children.empty()){
                    return treeStr;
                }

                //Case 3: Leaf Node (No Children)
                if(curr_node->children.empty() && curr_node->parent != NULL && !visted) {
                    treeStr += "(";
                    treeStr += curr_node->value;
                    treeStr += ")";
                    stack.pop_back();
                    continue;
                }

                //Case 4: Need to add closing parath. 
                if(idx > curr_node->children.size() - 1 && curr_node->parent != NULL) {
                    treeStr += ")";
                    stack.pop_back();
                    continue;
                }

                //Case 5: Node with children need to add opening parath.
                if(!curr_node->children.empty() && !visted) {
                    treeStr += "(";
                    treeStr += curr_node->value;
                    visted = true;
                }

                // Case 6: Process children.
                if (idx <= curr_node->children.size() - 1) {
                    Node* child = curr_node->children[idx];
                    idx++; // Move parent index forward for when we return
                    stack.push_back({child, 0, false});
                }

                // Case 7: Default
                else {
                    stack.pop_back();
                }
            }

            return treeStr;
        }

        /**
         * @brief Estimates the serialized byte size of a subtree without building the output string.
         *
         * Root nodes contribute only their raw value bytes; non-root nodes contribute their value bytes plus
         * one opening and one closing parenthesis, matching `toString()`.
         *
         * @param n The root of the subtree
         */
        size_t estimateSerializedSize(Node* n) {
            if(!n) return 0u;

            size_t total = n->value.size();
            if (n->parent != nullptr)
            {
                total += 2u;
            }

            for (Node* child : n->children)
            {
                size_t childSize = estimateSerializedSize(child);
                if (std::numeric_limits<size_t>::max() - total < childSize)
                {
                    throw RuntimeException{"Estimated serialized tree size overflowed size_t", RuntimeException::UNEXPECTED_ERROR};
                }
                total += childSize;
            }

            return total;
        }

        /**
         * @brief Counts the number of Nodes in a subtree including children
         * 
         * @param n The root of the subtree
         */
        size_t countNodes(Node* n) {
            if(!n) return 0u;

            size_t count = 1; // count n itself
            for(Node* child : n->children) count += countNodes(child);

            return count;
        }
        
        /**
         * @brief Traverse tree in-order, returning index-th node.
         * 
         * @param n The root Node of the tree
         * @param index the index of the Node to be found
         */
        Node* findNodeByIndex(Node* n, size_t& index) {

            if(n == nullptr) return nullptr;
            if(index == 0) return n;

            --index;
            for(Node* child : n->children) {
                Node* result = findNodeByIndex(child, index);
                if(result != nullptr) return result;
            }

            return nullptr;
        }

        /**
         * @brief Collects all non-leaf nodes in a subtree so callers can pick valid parents directly.
         *
         * @param n The root of the subtree
         * @param internalNodes Output vector that receives nodes with at least one child
         */
        void collectInternalNodes(Node* n, std::vector<Node*>& internalNodes)
        {
            if (n == nullptr)
            {
                return;
            }

            if (!n->children.empty())
            {
                internalNodes.push_back(n);
            }

            for (Node* child : n->children)
            {
                collectInternalNodes(child, internalNodes);
            }
        }
        /**
         * @brief Inserts a Node as a child of the given parent. If parent is nullptr returns the newly created Node.
         * 
         * @param value The value to be stored with this node
         * @param parent The parent that this node will be a child of
         */
        Node* insertNode(string value, Node* parent = nullptr)
        {
            auto newNode = std::make_unique<Node>(value, parent);
            if (parent)
            {
                parent->children.push_back(newNode.get());
            }
            return newNode.release();
        }

        /**
         * @brief Duplicates a node and all of its children.
         * 
         * @param original Pointer to the orginal root node that is to be duplicated
         * @param newParent Pointer to the new root node of the duplicate
         */
        Node* duplicateNode(Node* original, Node* newParent) {

            if(!original) {
                throw RuntimeException{"Node to be duplicated must not be nullptr", RuntimeException::USAGE_ERROR};
            }
            if(original == this->root) {
                throw RuntimeException{"Node to be duplicated must not be root", RuntimeException::USAGE_ERROR};
            }
            if(!newParent) {
                throw RuntimeException{"Parent of the Node to be duplicated must not be nullptr", RuntimeException::USAGE_ERROR};
            }

            Node* duplicate = insertNode(original->value, newParent);

            for(Node* child : original->children) {
                /*
                 *	duplicateNode attaches the new child via insertNode internally, so the recursive call alone is sufficient. Discard the return value; an outer push_back here would be a second attach of the same pointer and double-delete on Tree teardown.
                 */
                duplicateNode(child, duplicate);
            }

            return duplicate;
        }
        
        /**
         * @brief Replace one node's value with another's.
         * 
         * @param toReplace The node that is to be overwritten
         * @param toCopy The node that is to be copied
        */
        void replaceNode(Node* toReplace, Node* toCopy) {

            if(!toReplace || !toCopy) {
                throw RuntimeException{"Both nodes must not be nullptr", RuntimeException::USAGE_ERROR};
            }

            toReplace->value = toCopy->value;

            return;
        }
        /**
         * @brief Swap the values of two nodes.
         * 
         * @param node1 The first node to be swaped
         * @param node2 The second node to be swaped
        */
        void swapNodes(Node* node1, Node* node2) {

            if(!node1 || !node2) {
                throw RuntimeException{"Both nodes to be swapped must not be nullptr", RuntimeException::USAGE_ERROR};
            }

            const string temp = node1->value;
            node1->value = node2->value;
            node2->value = temp;

            return;
        }
        /**
         * @brief Deallocate a node and its children, and remove this node from its parent child list.
         * 
         * @param n The node that is to be deallocated.
        */
        void deleteNode(Node* n) {

            if(n == nullptr) return;

            // erase self from parent's children before destroying subtree
            if(n->parent) {
                auto& parentChildren = n->parent->children;
                auto it = std::find(parentChildren.begin(), parentChildren.end(), n);
                if(it != parentChildren.end()) {
                    parentChildren.erase(it);
                }
            }

            if(n == this->root) this->root = nullptr;

            std::vector<Node*> pending{n};
            while(!pending.empty()) {
                Node* current = pending.back();
                pending.pop_back();

                for(Node* child : current->children) {
                    child->parent = nullptr;
                    pending.push_back(child);
                }
                
                current->children.clear();
                delete current;
            }
        }
    
        /** 
         * @brief Iteratively deep-copies a path at the provided subtree to a specific child index.
         * 
         * Iterative form. Each iteration deep-copies the subtree rooted at `parent` and attaches the copy as a new child, 
         * so live-tree size grows by exactly `(countNodes(parent) - countNodes(parent->children[childIndex]))` nodes per iteration. 
         * The structural invariant of the iterative form (parentCopy at iteration i is a deep copy of the parentCopy planted at 
         * iteration i-1, which is structurally identical to the original `parent`) keeps both K = countNodes(parent) 
         * and S = countNodes(parent->children[childIndex]) constant across all iterations of a single call, so the total node 
         * growth is `effectiveNumReps * (K - S)`.
         * The cap is therefore computed once at entry by dividing the remaining node budget by the per-iteration delta, rather 
         * than measured incrementally after each iteration.
         *
         * @param parent Root of the subtree to be repeated.
         * @param childIndex Index to replace with the deep copy
         * @param numReps Number of copies to be performed.
         * @param maxTotalNodes Max number of Nodes that are to be created by repeating a path.
         */
        void repeatPath(Node* parent, size_t childIndex, size_t numReps,
                        size_t maxTotalNodes = std::numeric_limits<size_t>::max())
        {
            if (parent == nullptr) throw RuntimeException{"Node to be repeated must not be nullptr", RuntimeException::USAGE_ERROR};
            if (childIndex >= parent->children.size()) throw RuntimeException{"childIndex is out of bounds", RuntimeException::INDEX_OUT_OF_RANGE};

            /*
             *	Compute the per-iteration node delta and the current tree size once. 
             *  These are loop invariants for the iterative form, so a single division 
             *  yields the maximum number of iterations that fit under the node budget.
             */
            const size_t kSize = countNodes(parent);
            const size_t sSize = countNodes(parent->children[childIndex]);
            const size_t deltaPerIter = (kSize > sSize) ? (kSize - sSize) : 0u;
            const size_t currentSize = countNodes(this->root);

            size_t effectiveNumReps = numReps;
            if (deltaPerIter > 0u && maxTotalNodes != std::numeric_limits<size_t>::max())
            {
                const size_t budget = (maxTotalNodes > currentSize) ? (maxTotalNodes - currentSize) : 0u;
                const size_t fittingIters = budget / deltaPerIter;
                effectiveNumReps = std::min(numReps, fittingIters);
            }

            Node* current = parent;
            for (size_t i = 0; i < effectiveNumReps; ++i)
            {
                if (childIndex >= current->children.size()) break;

                Node* parentCopy = current->deepCopy(current);
                Node* toReplace = current->children[childIndex];
                std::vector<Node*> childrenCopy = toReplace->children;
                for (Node* child : childrenCopy)
                {
                    this->deleteNode(child);
                }
                delete toReplace;

                current->children[childIndex] = parentCopy;
                current = parentCopy;
            }
        }
    };
};
}
