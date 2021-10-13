#include <stdexcept>
#include <utility>

// Исключение этого типа должно генерироватся при обращении к пустому optional
class BadOptionalAccess : public std::exception {
public:
    using exception::exception;

    virtual const char* what() const noexcept override {
        return "Bad optional access";
    }
};

template <typename T>
class Optional {
public:
    Optional() = default;
    Optional(const T& value){
        new (data_) T(value);
        is_initialized_ = true;
    }
    Optional(T&& value){
        new (data_) T(std::move(value));
        is_initialized_ = true;
    }
    Optional(const Optional& other){
        if (other.HasValue()){
            new (data_) T(other.Value());
            is_initialized_ = true;
        }
    }
    Optional(Optional&& other){
        if (other.HasValue()){
            new (data_) T(std::move(other.Value()));
            is_initialized_ = true;
        }    
    }

    Optional& operator=(const T& value){
        if (HasValue()){
            Value() = value;
        } else {
           new (data_) T(value);
           is_initialized_ = true;
        }
        return *this;   
    }
    Optional& operator=(T&& value){
        if (HasValue()){
            Value() = std::move(value);
        } else {
           new (data_) T(std::move(value));
           is_initialized_ = true;
        }
        return *this;          
    }
    Optional& operator=(const Optional& rhs){
        if (HasValue()){
            if (rhs.HasValue()){
                Value()  = rhs.Value();
            } else {
                Reset();
            }
        } else if (rhs.HasValue()){
            new (data_) T(rhs.Value());
            is_initialized_ = true;
        }
        return *this;                
    }
    Optional& operator=(Optional&& rhs){
        if (HasValue()){
            if (rhs.HasValue()){
                Value() = std::move(rhs.Value());

            } else {
                Reset();
            }
        } else if (rhs.HasValue()){
            new (data_) T(std::move(rhs.Value()));
            is_initialized_ = true;
        }
        return *this;                     
    }

    ~Optional(){
        if (HasValue()){
            reinterpret_cast<T*>(data_)->~T();
        }
    }

    template<typename ...Ts>
    void Emplace(Ts&&... vs){
        if (HasValue()){
            Reset();
        }
        new (data_) T(Construct<decltype(vs)>(vs)...);
        is_initialized_ = true;
    } 

    bool HasValue() const{
        return is_initialized_;
    }

    // Операторы * и -> не должны делать никаких проверок на пустоту Optional.
    // Эти проверки остаются на совести программиста
    T& operator*(){
        return *reinterpret_cast<T*>(data_);
    }
    const T& operator*() const{
        return *reinterpret_cast<const T*>(data_);
    }
    T* operator->(){
        return reinterpret_cast<T*>(data_);
    }
    const T* operator->() const{
        return reinterpret_cast<const T*>(data_);
    }

    // Метод Value() генерирует исключение BadOptionalAccess, если Optional пуст
    T& Value(){
        if (!is_initialized_){
            throw BadOptionalAccess();
        } else {
            return *reinterpret_cast<T*>(data_);
        }
    }
    const T& Value() const{
        if (!is_initialized_){
            throw BadOptionalAccess();
        } else {
            return *reinterpret_cast<const T*>(data_);
        } 
    }

    void Reset(){
        reinterpret_cast<T*>(data_)->~T();
        is_initialized_ = false;
    }

private:
    // alignas нужен для правильного выравнивания блока памяти
    alignas(T) char data_[sizeof(T)];
    bool is_initialized_ = false;

    template <typename Type, typename S>
    Type Construct(S&& arg) {
        return Type(std::forward<S>(arg));
    }
};