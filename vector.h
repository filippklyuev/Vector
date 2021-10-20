#pragma once
#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <new>
#include <memory>
#include <utility>
#include <iostream>

#include <iostream>

template <typename T>
class RawMemory {
public:
    RawMemory() = default;
    RawMemory(const RawMemory&) = delete;
    RawMemory& operator=(const RawMemory& rhs) = delete;

    RawMemory(RawMemory&& other) noexcept { 
        buffer_ = other.buffer_;
        capacity_ = other.capacity_;
        other.buffer_ = nullptr;
        other.capacity_ = 0;
    }
    RawMemory& operator=(RawMemory&& rhs) noexcept {
        Deallocate(buffer_);
        buffer_ = rhs.buffer_;
        capacity_ = rhs.capacity_;
        rhs.buffer_ = nullptr;
        rhs.capacity_ = 0;
        return *this;
    }

    explicit RawMemory(size_t capacity)
        : buffer_(Allocate(capacity))
        , capacity_(capacity) {
    }

    ~RawMemory() {
        Deallocate(buffer_);
    }

    T* operator+(size_t offset) noexcept {
        assert(offset <= capacity_);
        return buffer_ + offset;
    }

    const T* operator+(size_t offset) const noexcept {
        return const_cast<RawMemory&>(*this) + offset;
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<RawMemory&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < capacity_);
        return buffer_[index];
    }

    void Swap(RawMemory& other) noexcept {
        std::swap(buffer_, other.buffer_);
        std::swap(capacity_, other.capacity_);
    }

    const T* GetAddress() const noexcept {
        return buffer_;
    }

    T* GetAddress() noexcept {
        return buffer_;
    }

    size_t Capacity() const {
        return capacity_;
    }

private:
    static T* Allocate(size_t n) {
        return n != 0 ? static_cast<T*>(operator new(n * sizeof(T))) : nullptr;
    }

    static void Deallocate(T* buf) noexcept {
        operator delete(buf);
    }

    T* buffer_ = nullptr;
    size_t capacity_ = 0;
};

template <typename T>
class Vector {
public:
     Vector() = default;

 explicit Vector(size_t size)
        : data_(size)
        , size_(size) 
    {
        std::uninitialized_value_construct_n(data_.GetAddress(), size);
    }

    Vector(const Vector& other)
        : data_(other.size_)
        , size_(other.size_) 
    {
        std::uninitialized_copy_n(other.data_.GetAddress(), size_, data_.GetAddress());
    }

    Vector(Vector&& other) noexcept
        : data_(std::move(other.data_))
        , size_ (other.size_)
    {
        other.size_ = 0;
    }


    Vector& operator=(const Vector& rhs) {
        if (this != &rhs) {
            if (rhs.size_ > data_.Capacity()) {
                Vector rhs_copy(rhs);
                Swap(rhs_copy);
            } else {
                
                for (size_t i = 0; i < rhs.size_ && i < size_; i++){
                    data_[i] = rhs.data_[i];
                }
                if (size_ < rhs.size_){
                    std::uninitialized_copy_n(rhs.data_ + size_, rhs.size_ - size_, data_ + size_);
                }
                if ((static_cast<int64_t>(size_) - static_cast<int64_t>(rhs.size_)) > 0){
                    std::destroy_n(data_ + rhs.size_, size_ - rhs.size_);
                }
                size_ = rhs.size_;
            }
        }
        return *this;
    }

    Vector& operator=(Vector&& rhs) noexcept {
        data_ = std::move(rhs.data_);
        size_ = rhs.size_;
        rhs.size_ = 0;
        return *this;

    }

    void Swap(Vector& other) noexcept {
        data_.Swap(other.data_);
        std::swap(size_, other.size_);
    }

    ~Vector() {
        std::destroy_n(data_.GetAddress(), size_);
    }

    size_t Size() const noexcept {
        return size_;
    }

    size_t Capacity() const noexcept {
        return data_.Capacity();
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<Vector&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < size_);
        return data_[index];
    }

    void Reserve(size_t new_capacity) {
        if (new_capacity <= data_.Capacity()) {
            return;
        }
        RawMemory<T> new_data(new_capacity);
        uninitCopyOrMoveN(data_.GetAddress(), new_data.GetAddress(), size_);
        std::destroy_n(data_.GetAddress(), size_);
        data_.Swap(new_data);
    }

    void Resize(size_t new_size){
        if (new_size < size_){
            std::destroy_n(data_ + new_size,  size_ - new_size);
        } else {
            Reserve(new_size);
            std::uninitialized_value_construct_n(data_ + size_, new_size - size_);
        }
        size_ = new_size;
    }

    void PushBack(const T& value){
        EmplaceBack(value);
    }

    void PushBack(T&& value){
        EmplaceBack(std::move(value));
    }

    template <typename... Args>
    T& EmplaceBack(Args&&... args){
        if (size_ == Capacity()){
            RawMemory<T> new_data(size_ == 0 ? 1 : size_ * 2);
            new (new_data + size_) T(std::forward<Args>(args)...);
            uninitCopyOrMoveN(data_.GetAddress(), new_data.GetAddress(), size_);
            std::destroy_n(data_.GetAddress(), size_);
            data_.Swap(new_data);            
        } else {
            new (data_ + size_) T(std::forward<Args>(args)...);
        }
        ++size_;
        return data_[size_ - 1];
    }

    void PopBack()  noexcept {
        std::destroy_at(data_ + (size_ - 1));
        size_--;
    }

    using iterator = T*;
    using const_iterator = const T*;
    
    iterator begin() noexcept {
        return data_.GetAddress();
    }
    iterator end() noexcept {
        return data_ + size_;
    }
    const_iterator begin() const noexcept {
        return cbegin();
    }
    const_iterator end() const noexcept{
        return cend();
    }
    const_iterator cbegin() const noexcept {
        return data_.GetAddress();
    }
    const_iterator cend() const noexcept{
        return data_ + size_;
    }

    template <typename... Args>
    iterator Emplace(const_iterator pos, Args&&... args){
        if (pos < cbegin() || pos > cend()){
            return end() ;
        }
        size_t distance = pos - begin();
        if (size_ == Capacity()){
            RawMemory<T> new_data(size_ == 0 ? 1 : size_ * 2);
            new (new_data + distance) T(std::forward<Args>(args)...);
            try { 
                uninitCopyOrMoveN(begin(), new_data.GetAddress(), distance);     
            } catch(...){
                std::destroy_at(new_data + distance);
                throw;
            }
            try {
                uninitCopyOrMoveN(begin() + distance, new_data + distance + 1, size_ - distance);   
            } catch (...){
                std::destroy_n(new_data.GetAddress(), distance + 1);
                throw;
            }          
            std::destroy_n(data_.GetAddress(), size_);
            data_.Swap(new_data);   
        } else {
            if (distance != size_) {
                T temp_value(std::forward<Args>(args)...);
                std::uninitialized_move_n(end() - 1, 1, end());
                std::move_backward(begin() + distance, end() - 1, end());
                data_[distance] = (std::move(temp_value));
            } else {
                new (data_ + distance) T(std::forward<Args>(args)...);  
            }
        }
        ++size_;
        return begin() + distance;
    }

    iterator Erase(const_iterator pos) /*noexcept(std::is_nothrow_move_assignable_v<T>)*/{
        size_t distance = pos - data_.GetAddress();
        if (distance < size_ - 1){
            std::move(begin() + (distance  + 1), begin() + size_, begin() + distance);
        }
        PopBack();
        return data_ + distance;
    }

    iterator Insert(const_iterator pos, const T& value){
        return Emplace(pos, value);
    }

    iterator Insert(const_iterator pos, T&& value){
        return Emplace(pos, std::move(value));
    }

private:
    RawMemory<T> data_;
    size_t size_ = 0;

    void uninitCopyOrMoveN(T* from, T* to, size_t nbr){
        if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>){
            std::uninitialized_move_n(from, nbr, to);
        } else {
            std::uninitialized_copy_n(from, nbr, to);
        }    
    }
};