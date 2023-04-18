#pragma once

#include <cassert>
#include <initializer_list>
#include <string>
#include <stdexcept>
#include <utility>
#include <algorithm>
#include <iterator>

#include "array_ptr.h"

class ReserveProxyObj {
public:
    explicit ReserveProxyObj(size_t size) : size_(size) {
    }

    size_t GetSize() {
        return size_;
    }

private:
    size_t size_ = 0;
};

template <typename Type>
class SimpleVector {
public:
    using Iterator = Type*;
    using ConstIterator = const Type*;

    SimpleVector() noexcept = default;

    explicit SimpleVector(size_t size) : SimpleVector(size, Type{}) {
    }

    SimpleVector(ReserveProxyObj obj) {
        size_t size_ = obj.GetSize();
        Reserve(size_);
    }

    SimpleVector(size_t size, const Type& value) : size_(size), capacity_(size), items_(size) {
        std::fill(items_.Get(), items_.Get() + size, value);
    }

    SimpleVector(std::initializer_list<Type> init) : size_(init.size()), capacity_(init.size()), items_(init.size()) {
        std::copy(init.begin(), init.end(), items_.Get());
    }

    SimpleVector(const SimpleVector& other) {
        assert(size_ == 0);
        ArrayPtr<Type> items(other.capacity_);
        if (!other.IsEmpty()) {
            std::copy(other.begin(), other.end(), items.Get());
        }
        items_.swap(items);
        size_ = other.size_;
        capacity_ = other.capacity_;
    }

    SimpleVector(SimpleVector&& other) {
        assert(size_ == 0);
        size_ = std::exchange(other.size_, 0);
        capacity_ = std::exchange(other.capacity_, 0);
        items_ = std::move(other.items_);
    }

    SimpleVector& operator=(const SimpleVector& rhs) {
        if (this != &rhs) {
            SimpleVector<Type> copy_vector(rhs);
            swap(copy_vector);
        }
        return *this;
    }

    SimpleVector& operator=(SimpleVector&& rhs) {
        if (this != &rhs) {
            swap(rhs);
        }
        return *this;
    }

    size_t GetSize() const noexcept {
        return size_;
    }

    size_t GetCapacity() const noexcept {
        return capacity_;
    }

    bool IsEmpty() const noexcept {
        return size_ == 0;
    }

    Type& operator[](size_t index) noexcept {
        assert(index < size_);
        return items_[index];
    }

    const Type& operator[](size_t index) const noexcept {
        assert(index < size_);
        return items_[index];
    }

    Type& At(size_t index) {
        if (index >= size_) {
            using namespace std::string_literals;
            throw std::out_of_range("Out of range"s);
        }
        return items_[index];
    }

    const Type& At(size_t index) const {
        if (index >= size_) {
            using namespace std::string_literals;
            throw std::out_of_range("Out of range");
        }
        return items_[index];
    }

    void Clear() noexcept {
        size_ = 0;
    }

    void Reserve(size_t new_capacity) {
        if (new_capacity > capacity_) {
            ArrayPtr<Type> new_items(new_capacity);
            std::move(items_.Get(), items_.Get() + size_, new_items.Get());
            std::generate(new_items.Get() + size_, new_items.Get() + new_capacity, []() {return std::move(Type{}); });
            items_.swap(new_items);
            capacity_ = new_capacity;
        }
    }

    void Resize(size_t new_size) {
        if (size_ == new_size) {
            return;
        }
        else if (new_size <= capacity_ && new_size > size_) {
            std::generate(items_.Get() + size_, items_.Get() + new_size, []() {return std::move(Type{}); });
        }
        else if (new_size > capacity_) {
            size_t new_capacity = std::max(new_size, capacity_ * 2);
            Reserve(new_capacity);
        }
        size_ = new_size;
    }

    void PushBack(const Type& item) {
        if (size_ == capacity_) {
            size_t new_capacity = (size_ == 0 ? 1 : capacity_ * 2);
            Reserve(new_capacity);
            items_[size_] = item;
            ++size_;
            return;
        }
        items_[size_] = item;
        ++size_;
    }

    void PushBack(Type&& item) {
        if (size_ == capacity_) {
            size_t new_capacity = (size_ == 0 ? 1 : capacity_ * 2);
            Reserve(new_capacity);
            items_[size_] = std::move(item);
            ++size_;
            return;
        }
        items_[size_] = std::move(item);
        ++size_;
    }

    Iterator Insert(ConstIterator pos, const Type& value) {
        assert(pos >= items_.Get() && pos <= (items_.Get() + size_));
        size_t new_capacity;
        Iterator new_pos = const_cast<Iterator>(pos);
        if (size_ == capacity_) {
            new_capacity = (size_ == 0 ? 1 : capacity_ * 2);
        }
        else {
            new_capacity = capacity_;
        }
        ArrayPtr<Type> new_items(new_capacity);
        size_t index = std::distance(items_.Get(), new_pos);
        std::copy(items_.Get(), new_pos, new_items.Get());

        new_items[index] = value;
        std::copy_backward(new_pos, items_.Get() + size_, new_items.Get() + (size_ + 1));
        std::fill(new_items.Get() + (size_ + 1), new_items.Get() + new_capacity, Type{});

        items_.swap(new_items);
        ++size_;
        capacity_ = new_capacity;
        return &items_[index];
    }

    Iterator Insert(ConstIterator pos, Type&& value) {
        assert(pos >= items_.Get() && pos <= (items_.Get() + size_));
        size_t new_capacity;
        Iterator new_pos = const_cast<Iterator>(pos);
        if (size_ == capacity_) {
            new_capacity = (size_ == 0 ? 1 : capacity_ * 2);
        }
        else {
            new_capacity = capacity_;
        }
        ArrayPtr<Type> new_items(new_capacity);
        size_t index = std::distance(items_.Get(), new_pos);
        std::move(items_.Get(), new_pos, new_items.Get());

        new_items[index] = std::move(value);
        std::move_backward(new_pos, items_.Get() + size_, new_items.Get() + (size_ + 1));
        std::generate(new_items.Get() + (size_ + 1), new_items.Get() + new_capacity, []() {return std::move(Type{}); });

        items_ = std::move(new_items);
        ++size_;
        capacity_ = new_capacity;
        return &items_[index];
    }

    void PopBack() noexcept {
        assert(!IsEmpty());
        --size_;
    }

    Iterator Erase(ConstIterator pos) {
        assert(!IsEmpty());
        assert(pos >= items_.Get() && pos <= (items_.Get() + size_));
        Iterator new_pos = const_cast<Iterator>(pos);
        size_t index = std::distance(items_.Get(), new_pos);
        std::move(new_pos + 1, items_.Get() + size_, new_pos);
        --size_;
        return &items_[index];
    }

    void swap(SimpleVector& other) noexcept {
        items_.swap(other.items_);
        std::swap(size_, other.size_);
        std::swap(capacity_, other.capacity_);
    }

    void swap(SimpleVector&& other) noexcept {
        items_ = other.items_;
        std::swap(size_, other.size_);
        std::swap(capacity_, other.capacity_);
    }

    Iterator begin() noexcept {
        return items_.Get();
    }

    Iterator end() noexcept {
        return items_.Get() + size_;
    }

    ConstIterator begin() const noexcept {
        return items_.Get();
    }

    ConstIterator end() const noexcept {
        return items_.Get() + size_;
    }

    ConstIterator cbegin() const noexcept {
        return items_.Get();
    }

    ConstIterator cend() const noexcept {
        return items_.Get() + size_;
    }

private:
    size_t size_ = 0;
    size_t capacity_ = 0;
    ArrayPtr<Type> items_;
};

template <typename Type>
inline bool operator==(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename Type>
inline bool operator!=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs == rhs);
}

template <typename Type>
inline bool operator<(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename Type>
inline bool operator<=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(rhs < lhs);
}

template <typename Type>
inline bool operator>(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return rhs < lhs;
}

template <typename Type>
inline bool operator>=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs < rhs);
}

ReserveProxyObj Reserve(size_t capacity_to_reserve) {
    return ReserveProxyObj(capacity_to_reserve);
}