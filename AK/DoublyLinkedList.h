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
    DoublyLinkedListIterator() = default;

    bool operator!=(DoublyLinkedListIterator const& other) const { return m_node != other.m_node; }
    bool operator==(DoublyLinkedListIterator const& other) const { return m_node == other.m_node; }

    DoublyLinkedListIterator& operator++()
    {
        m_node = m_node->next;
        return *this;
    }

    ElementType& operator*() { return m_node->value; }
    ElementType* operator->() { return &m_node->value; }

    operator bool() const { return !is_end(); }

    DoublyLinkedListIterator prev() const { return DoublyLinkedListIterator(m_node ? m_node->prev : nullptr); }
    DoublyLinkedListIterator next() const { return DoublyLinkedListIterator(m_node ? m_node->next : nullptr); }

    [[nodiscard]] bool is_end() const { return !m_node; }
    [[nodiscard]] bool is_begin() const { return m_node && !m_node->prev; }

private:
    friend ListType;
    explicit DoublyLinkedListIterator(typename ListType::Node* node)
        : m_node(node)
    {
    }
    typename ListType::Node* m_node { nullptr };
};

template<typename T>
class DoublyLinkedList {
private:
    struct Node {
        explicit Node(T&& v)
            : value(move<T>(v))
        {
        }

        explicit Node(const T& v)
            : value(v)
        {
        }

        T value;
        Node* next { nullptr };
        Node* prev { nullptr };
    };

public:
    DoublyLinkedList() = default;
    ~DoublyLinkedList() { clear(); }

    [[nodiscard]] ALWAYS_INLINE bool is_empty() const { return !m_head; }

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

    inline size_t size_slow() const
    {
        size_t size = 0;
        for (auto* node = m_head; node; node = node->next)
            ++size;
        return size;
    }

    [[nodiscard]] T& first()
    {
        VERIFY(m_head);
        return m_head->value;
    }
    [[nodiscard]] const T& first() const
    {
        VERIFY(m_head);
        return m_head->value;
    }
    [[nodiscard]] T& last()
    {
        VERIFY(m_head);
        return m_tail->value;
    }
    [[nodiscard]] const T& last() const
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

    T take_last()
    {
        VERIFY(m_tail);
        auto* prev_tail = m_tail;
        T value = move(last());
        if (m_head == m_tail)
            m_head = nullptr;
        m_tail = m_tail->prev;
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

    template<typename U>
    void prepend(U&& value)
    {
        static_assert(IsSame<T, U>);
        auto* node = new Node(forward<U>(value));
        if (!m_head) {
            VERIFY(!m_tail);
            m_head = node;
            m_tail = node;
            return;
        }
        VERIFY(m_tail);
        VERIFY(!node->prev);
        m_head->prev = node;
        node->next = m_head;
        m_head = node;
    }

    [[nodiscard]] bool contains_slow(const T& value) const
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

    ConstIterator find(const T& value) const
    {
        return AK::find(begin(), end(), value);
    }

    Iterator find(const T& value)
    {
        return AK::find(begin(), end(), value);
    }

    void remove(const T& value)
    {
        auto it = find(value);
        if (!it.is_end())
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

    template<typename U = T>
    void insert_before(Iterator iterator, U&& value)
    {
        if (iterator.is_end()) {
            append(value);
            return;
        }

        auto* node = new Node(forward<U>(value));
        auto* old_prev = iterator.m_node->prev;
        if (old_prev)
            old_prev->next = node;
        node->prev = old_prev;
        node->next = iterator.m_node;
        iterator.m_node->prev = node;

        if (m_head == iterator.m_node)
            m_head = node;
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
        if (old_next)
            old_next->prev = node;
        node->next = old_next;
        node->prev = iterator.m_node;
        iterator.m_node->next = node;

        if (m_tail == iterator.m_node)
            m_tail = node;
    }

private:
    Node* m_head { nullptr };
    Node* m_tail { nullptr };
};

}

using AK::DoublyLinkedList;
