/* =============================================================================
 * Copyright (c) 2026 Vigilant Cyber Systems
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
 * 
 */
class RadamsaTreeMutatorBase: public RadamsaMutatorBase
{
public:
    RadamsaTreeMutatorBase() = default;
    virtual ~RadamsaTreeMutatorBase() = default;

    struct Node
    {
        string value;
        Node* parent;
        std::vector<Node*> children;

        Node(string v, Node* p = nullptr) : value(v), parent(p) {};

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

    struct Tree
    {
    private:
        // parses a Tree from a string in the form ""A(B(C)(D))(E)""
        void buildTree(string treeStr) {
            if(treeStr.empty()) {
                throw RuntimeException{"Tree string is empty", RuntimeException::UNEXPECTED_ERROR};
            }

            std::stack<Node*> stk;
            string value;
            for(size_t i = 0; i < treeStr.length(); ++i) {
                unsigned char ch = treeStr[i];

                if(std::isspace(ch)) {
                    continue;
                }
                else if(ch == '(') {
                    if(!value.empty()) {
                        Node* n;
                        if(stk.empty()) {
                            n = insertNode(value);
                            this->root = n;
                        }
                        else {
                            n = insertNode(value, stk.top());
                        }

                        stk.push(n);
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
                            stk.push(toPush);
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
                        insertNode(value, stk.top());

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
                this->root = insertNode(value);
            }

            return;
        }
    
    public:
        Node* root = nullptr;

        Tree() = default;

        /*
         *	Tree construction goes through the noexcept tryBuild factory. The throwing constructor form is deleted: a constructor that throws after attaching Nodes under root would leak those Nodes, since C++ does not run the class destructor for an object whose constructor threw.
         */

        // The throwing constructor form is removed. Use Tree::tryBuild instead.
        Tree(string const&) = delete;

        // Builds a Tree from `treeStr`. Returns std::nullopt on any parse failure. Never throws. On parse failure the partially-built Tree's destructor runs before the optional resets, so no Node is leaked.
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
        
        // deleting copy constructor and copy assignment to avoid shallow copies
        Tree(const Tree&) = delete;
        Tree& operator=(const Tree&) = delete;

        // Move constructor
        Tree(Tree&& other) noexcept {
            root = other.root;
            other.root = nullptr;
        }

        // Move assignment operator
        Tree& operator=(Tree&& other) noexcept {
            if(this != &other) {    // prevents self-assignment
                deleteNode(root);

                root = other.root;
                other.root = nullptr;
            }
            return *this;
        }

        ~Tree() { deleteNode(this->root); }

        string toString(Node* n) {
            // create a parenthesis-delimited string from subtree n

            if(!n) return "";

            string treeStr(n->value);

            for(Node* child : n->children) {
                treeStr += "(";
                treeStr += toString(child);
                treeStr += ")";
            }

            return treeStr;
        }

        size_t countNodes(Node* n) {
            if(!n) return 0u;

            size_t count = 1; // count n itself
            for(Node* child : n->children) count += countNodes(child);

            return count;
        }
    
        Node* findNodeByIndex(Node* n, size_t& index) {
            // traverse tree in-order, returning index-th node

            if(n == nullptr) return nullptr;
            if(index == 0) return n;

            --index;
            for(Node* child : n->children) {
                Node* result = findNodeByIndex(child, index);
                if(result != nullptr) return result;
            }

            return nullptr;
        }

        Node* insertNode(string value, Node* parent = nullptr)
        {
            auto newNode = std::make_unique<Node>(value, parent);
            if (parent)
            {
                parent->children.push_back(newNode.get());
            }
            return newNode.release();
        }

        Node* duplicateNode(Node* original, Node* newParent) {
            // Duplicates a node and all of its children

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

        void replaceNode(Node* toReplace, Node* toCopy) {
            // Replace one node's value with another's

            if(!toReplace || !toCopy) {
                throw RuntimeException{"Both nodes must not be nullptr", RuntimeException::USAGE_ERROR};
            }

            toReplace->value = toCopy->value;

            return;
        }

        void swapNodes(Node* node1, Node* node2) {
            // Swap the values of two nodes

            if(!node1 || !node2) {
                throw RuntimeException{"Both nodes to be swapped must not be nullptr", RuntimeException::USAGE_ERROR};
            }

            const string temp = node1->value;
            node1->value = node2->value;
            node2->value = temp;

            return;
        }

        void deleteNode(Node* n) {
            // deallocate n and its children, and remove n from n->parent->children

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
    
        /*
         *	Iterative form. Each iteration deep-copies the subtree rooted at `parent` and attaches the copy as a new child, so live-tree size grows by exactly `(countNodes(parent) - countNodes(parent->children[childIndex]))` nodes per iteration. The structural invariant of the iterative form (parentCopy at iteration i is a deep copy of the parentCopy planted at iteration i-1, which is structurally identical to the original `parent`) keeps both K = countNodes(parent) and S = countNodes(parent->children[childIndex]) constant across all iterations of a single call, so the total node growth is `effectiveNumReps * (K - S)`. The cap is therefore computed once at entry by dividing the remaining node budget by the per-iteration delta, rather than measured incrementally after each iteration.
         */

        // Replaces parent->children[childIndex] with successive deep copies of `parent`. Loops up to `numReps` times, further bounded by `maxTotalNodes` so the live tree never exceeds that node count. The default `maxTotalNodes` of `std::numeric_limits<size_t>::max()` disables the adaptive cap and runs the full `numReps` iterations. When the current tree already exceeds the budget no iterations are performed.
        void repeatPath(Node* parent, size_t childIndex, size_t numReps,
                        size_t maxTotalNodes = std::numeric_limits<size_t>::max())
        {
            if (parent == nullptr) throw RuntimeException{"Node to be repeated must not be nullptr", RuntimeException::USAGE_ERROR};
            if (childIndex >= parent->children.size()) throw RuntimeException{"childIndex is out of bounds", RuntimeException::INDEX_OUT_OF_RANGE};

            /*
             *	Compute the per-iteration node delta and the current tree size once. These are loop invariants for the iterative form, so a single division yields the maximum number of iterations that fit under the node budget.
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
