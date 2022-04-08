/*
 * Copyright (c) 2018-2020, Andreas Kling <kling@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Assertions.h>
#include <AK/Find.h>
#include <AK/StdLibExtras.h>
#include <AK/Traits.h>
#include <AK/Types.h>

namespace AK {

template<typename ListType, typename ElementType>
class SinglyLinkedListIterator {
public:
    using NodeType = typename ListType::Node;

    SinglyLinkedListIterator() = default;

    [[nodiscard]] bool is_end() const { return !m_node; }
    [[nodiscard]] auto next() const { return SinglyLinkedListIterator(m_node ? m_node->next : nullptr, m_node); }

    void remove(ListType& list)
    {
        m_removed = true;
        list.remove(*this);
    };

    operator bool() const { return !is_end(); }
    bool operator==(SinglyLinkedListIterator const& other) const { return m_node == other.m_node; }
    bool operator!=(SinglyLinkedListIterator const& other) const { return m_node != other.m_node; }

    SinglyLinkedListIterator& operator++()
    {
        if (m_removed)
            m_removed = false;
        else
            m_prev = m_node;
        if (m_node)
            m_node = m_node->next;
        return *this;
    }

    ElementType& operator*()
    {
        VERIFY(!m_removed);
        return m_node->value;
    }

    ElementType* operator->()
    {
        VERIFY(!m_removed);
        return &m_node->value;
    }

private:
    friend ListType;

    explicit SinglyLinkedListIterator(NodeType* node, NodeType* prev = nullptr)
        : m_node(node)
        , m_prev(prev)
    {
    }

    NodeType* m_node { nullptr };
    NodeType* m_prev { nullptr };
    bool m_removed { false };
};

template<typename T>
class SinglyLinkedList {
private:
    struct Node {
        explicit Node(T&& v)
            : value(move(v))
        {
        }
        explicit Node(const T& v)
            : value(v)
        {
        }
        T value;
        Node* next { nullptr };
    };

public:
    SinglyLinkedList() = default;
    SinglyLinkedList(SinglyLinkedList const& other) = delete;
    SinglyLinkedList(SinglyLinkedList&& other)
        : m_head(other.m_head)
        , m_tail(other.m_tail)
    {
        other.m_head = nullptr;
        other.m_tail = nullptr;
    }

    ~SinglyLinkedList() { clear(); }

    SinglyLinkedList& operator=(SinglyLinkedList const& other) = delete;
    SinglyLinkedList& operator=(SinglyLinkedList&&) = delete;

    [[nodiscard]] bool is_empty() const { return !m_head; }

    inline size_t size_slow() const
    {
        size_t size = 0;
        for (auto* node = m_head; node; node = node->next)
            ++size;
        return size;
    }

    void clear()
    {
        for (auto* node = m_head; node;) {
            auto* next = node->next;
            delete node;
            node = next;
        }
        m_head = nullptr;
        m_tail = nullptr;
    }

    [[nodiscard]] T& first()
    {
        VERIFY(m_head);
        return m_head->value;
    }
    [[nodiscard]] T const& first() const
    {
        VERIFY(m_head);
        return m_head->value;
    }
    [[nodiscard]] T& last()
    {
        VERIFY(m_head);
        return m_tail->value;
    }
    [[nodiscard]] T const& last() const
    {
        VERIFY(m_head);
        return m_tail->value;
    }

    T take_first()
    {
        VERIFY(m_head);
        auto* prev_head = m_head;
        T value = move(first());
        if (m_tail == m_head)
            m_tail = nullptr;
        m_head = m_head->next;
        delete prev_head;
        return value;
    }

    template<typename U = T>
    void append(U&& value)
    {
        auto* node = new Node(forward<U>(value));
        if (!m_head) {
            VERIFY(!m_tail);
            m_head = node;
            m_tail = node;
            return;
        }
        VERIFY(m_tail);
        m_tail->next = node;
        m_tail = node;
    }

    [[nodiscard]] bool contains_slow(T const& value) const
    {
        return find(value) != end();
    }

    using Iterator = SinglyLinkedListIterator<SinglyLinkedList, T>;
    friend Iterator;
    Iterator begin() { return Iterator(m_head); }
    Iterator end() { return {}; }

    using ConstIterator = SinglyLinkedListIterator<const SinglyLinkedList, const T>;
    friend ConstIterator;
    ConstIterator begin() const { return ConstIterator(m_head); }
    ConstIterator end() const { return {}; }

    ConstIterator find(T const& value) const
    {
        return AK::find(begin(), end(), value);
    }

    Iterator find(T const& value)
    {
        return AK::find(begin(), end(), value);
    }

    template<typename TUnaryPredicate>
    ConstIterator find_if(TUnaryPredicate&& pred) const
    {
        return AK::find_if(begin(), end(), forward<TUnaryPredicate>(pred));
    }

    template<typename TUnaryPredicate>
    Iterator find_if(TUnaryPredicate&& pred)
    {
        return AK::find_if(begin(), end(), forward<TUnaryPredicate>(pred));
    }

    template<typename U = T>
    void insert_before(Iterator iterator, U&& value)
    {
        static_assert(
            requires { T(value); }, "Conversion operator is missing.");

        if (iterator.is_end()) {
            append(value);
            return;
        }

        auto* node = new Node(forward<U>(value));
        node->next = iterator.m_node;
        if (m_head == iterator.m_node)
            m_head = node;
        if (iterator.m_prev)
            iterator.m_prev->next = node;
    }

    template<typename U = T>
    void insert_after(Iterator iterator, U&& value)
    {
        static_assert(
            requires { T(value); }, "Conversion operator is missing.");

        if (iterator.is_end()) {
            append(value);
            return;
        }

        auto* node = new Node(forward<U>(value));
        node->next = iterator.m_node->next;

        iterator.m_node->next = node;

        if (m_tail == iterator.m_node)
            m_tail = node;
    }

    void remove(T const& value)
    {
        auto it = find(value);
        if (!it.is_end())
            remove(it);
    }

    void remove(Iterator& iterator)
    {
        VERIFY(!iterator.is_end());
        if (m_head == iterator.m_node)
            m_head = iterator.m_node->next;
        if (m_tail == iterator.m_node)
            m_tail = iterator.m_prev;
        if (iterator.m_prev)
            iterator.m_prev->next = iterator.m_node->next;
        delete iterator.m_node;
    }

private:
    Node* m_head { nullptr };
    Node* m_tail { nullptr };
};

}

using AK::SinglyLinkedList;
