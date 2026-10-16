#pragma once

#include <vector>

template<typename Item>
class preallocated_list
{
    struct node
    {
        const Item* value;
        size_t next = SIZE_MAX; // SIZE_MAX (-1) means "no next"
    };

    std::vector<node> pool;
    size_t head = SIZE_MAX;
    size_t tail = SIZE_MAX;

public:
    class iterator {
        friend class preallocated_list;

        const preallocated_list* list;
        size_t current;

    public:
        // all the usings are here to make std::find_if happy
        using iterator_category = std::forward_iterator_tag;
        using value_type = Item;
        using difference_type = std::ptrdiff_t;
        using pointer = const Item*;
        using reference = const Item&;

        iterator(const preallocated_list* list, size_t start)
            : list(list), current(start)
        {
        }

        const Item* operator*() const
        {
            return list->pool[current].value;
        }

        iterator& operator++()
        {
            current = list->pool[current].next;
            return *this;
        }

        const iterator next() const
        {
            return {list, list->pool[current].next};
        }

        bool operator == (const iterator& that) const
        {
            return this->list == that.list && this->current == that.current;
        }

        bool operator != (const iterator& that) const
        {
            return !(*this == that);
        }

        operator bool () const
        {
            return current != SIZE_MAX;
        }
    };

    iterator begin() const { return iterator(this, head); }
    iterator end() const { return iterator(this, SIZE_MAX); }

    bool insert(iterator it, const Item* value)
    {
        size_t index = allocate_node(value);

        if (!it) // insert at head if iterator is invalid
        {
            pool[index].next = head;
            head = index;
            if (tail == SIZE_MAX) tail = head;
            return true;
        }

        // insert after the iterator's current position
        pool[index].next = pool[it.current].next;
        pool[it.current].next = index;

        if (pool[index].next == SIZE_MAX)
        {
            tail = index; // Update tail if inserted at end
        }

        return true;
    }

    bool empty() const
    {
        return head == SIZE_MAX;
    }

    void clear()
    {
        pool.clear();
        head = tail = SIZE_MAX;
    }

    size_t size() const
    {
        return pool.size();
    }

private:
    size_t allocate_node(const Item* item)
    {
        pool.push_back({item, SIZE_MAX});
        return pool.size() - 1;
    }
};