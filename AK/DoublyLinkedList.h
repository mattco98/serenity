/*
 * Copyright (c) 2018-2020, Andreas Kling <kling@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Assertions.h>
#include <AK/Find.h>
#include <AK/StdLibExtras.h>

namespace AK {

template<typename ListType, typename ElementType>
class DoublyLinkedListIterator {
public:
    using NodeType = typename ListType::Node;

    DoublyLinkedListIterator() = default;

    [[nodiscard]] bool is_end() const { return !m_node; }
    [[nodiscard]] auto prev() const { return DoublyLinkedListIterator(m_node ? m_node->prev : nullptr); }
    [[nodiscard]] auto next() const { return DoublyLinkedListIterator(m_node ? m_node->next : nullptr); }

    void remove(ListType& list)
    {
        m_removed = true;
        list.remove(*this);
    }

    operator bool() const { return !is_end(); }
    bool operator!=(DoublyLinkedListIterator const& other) const { return m_node != other.m_node; }
    bool operator==(DoublyLinkedListIterator const& other) const { return m_node == other.m_node; }

    DoublyLinkedListIterator& operator++()
    {
        if (m_removed)
            m_removed = false;
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

    explicit DoublyLinkedListIterator(NodeType* node)
        : m_node(node)
    {
    }

    NodeType* m_node;
    bool m_removed { false };
};

template<typename T>
class DoublyLinkedList {
private:
    struct Node {
        template<typename U>
        explicit Node(U&& v)
            : value(forward<U>(v))
        {
            static_assert(
                requires { T(v); }, "Conversion operator is missing.");
        }
        T value;
        Node* next { nullptr };
        Node* prev { nullptr };
    };

public:
    DoublyLinkedList() = default;
    DoublyLinkedList(DoublyLinkedList const& other) = delete;
    DoublyLinkedList(DoublyLinkedList&& other)
        : m_head(other.m_head)
        , m_tail(other.m_tail)
    {
        other.m_head = nullptr;
        other.m_tail = nullptr;
    }

    ~DoublyLinkedList() { clear(); }

    DoublyLinkedList& operator=(DoublyLinkedList const& other) = delete;
    DoublyLinkedList& operator=(DoublyLinkedList&&) = delete;

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
        VERIFY(m_tail);
        return m_tail->value;
    }
    [[nodiscard]] T const& last() const
    {
        VERIFY(m_tail);
        return m_tail->value;
    }

    T take_first()
    {
        VERIFY(m_head);
        auto* prev_head = m_head;
        T value = move(first());
        m_head = m_head->next;
        if (!m_head) {
            m_tail = nullptr;
        } else {
            m_head->prev = nullptr;
        }
        delete prev_head;
        return value;
    }

    T take_last()
    {
        VERIFY(m_tail);
        auto* prev_tail = m_tail;
        T value = move(last());
        m_tail = m_tail->prev;
        if (!m_tail) {
            m_head = nullptr;
        } else {
            m_tail->next = nullptr;
        }
        delete prev_tail;
        return value;
    }

    template<typename U = T>
    void append(U&& value)
    {
        static_assert(
            requires { T(value); }, "Conversion operator is missing.");
        auto* node = new Node(forward<U>(value));
        if (!m_head) {
            VERIFY(!m_tail);
            m_head = node;
            m_tail = node;
            return;
        }
        VERIFY(m_tail);
        m_tail->next = node;
        node->prev = m_tail;
        m_tail = node;
    }

    template<typename U = T>
    void prepend(U&& value)
    {
        static_assert(
            requires { T(value); }, "Conversion operator is missing.");
        auto* node = new Node(forward<U>(value));
        if (!m_head) {
            VERIFY(!m_tail);
            m_head = node;
            m_tail = node;
            return;
        }
        VERIFY(m_tail);
        m_head->prev = node;
        node->next = m_head;
        m_head = node;
    }

    [[nodiscard]] bool contains_slow(T const& value) const
    {
        return find(value) != end();
    }

    using Iterator = DoublyLinkedListIterator<DoublyLinkedList, T>;
    friend Iterator;
    Iterator begin() { return Iterator(m_head); }
    Iterator end() { return {}; }

    using ConstIterator = DoublyLinkedListIterator<const DoublyLinkedList, const T>;
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
        if (iterator.is_end()) {
            append(value);
            return;
        }

        auto* node = new Node(forward<U>(value));
        auto* old_prev = iterator.m_node->prev;
        if (old_prev) {
            old_prev->next = node;
        } else {
            m_head = node;
        }
        node->prev = old_prev;
        node->next = iterator.m_node;
        iterator.m_node->prev = node;
    }

    template<typename U = T>
    void insert_after(Iterator iterator, U&& value)
    {
        if (iterator.is_end()) {
            append(value);
            return;
        }

        auto* node = new Node(forward<U>(value));
        auto* old_next = iterator.m_node->next;
        if (old_next) {
            old_next->prev = node;
        } else {
            m_tail = node;
        }
        node->next = old_next;
        node->prev = iterator.m_node;
        iterator.m_node->next = node;
    }

    void remove(T const& value)
    {
        if (auto it = find(value))
            remove(it);
    }

    void remove(Iterator it)
    {
        VERIFY(it.m_node);
        auto* node = it.m_node;
        if (node->prev) {
            VERIFY(node != m_head);
            node->prev->next = node->next;
        } else {
            VERIFY(node == m_head);
            m_head = node->next;
        }
        if (node->next) {
            VERIFY(node != m_tail);
            node->next->prev = node->prev;
        } else {
            VERIFY(node == m_tail);
            m_tail = node->prev;
        }
        delete node;
    }

private:
    Node* m_head { nullptr };
    Node* m_tail { nullptr };
};

}

using AK::DoublyLinkedList;
