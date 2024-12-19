#ifndef __XUBINH_SERVER_UTIL_ANY
#define __XUBINH_SERVER_UTIL_ANY

#include <type_traits>
#include <typeinfo>
#include <utility>

#include "util/address_of.h"
#include "util/type_traits.h"

namespace xubinh_server {

namespace util {

//////////////////////////////
// [TODO]: clean these up
template <typename T>
using enable_if_is_pointer_type_t =
    type_traits::enable_if_t<std::is_pointer<T>::value>;

template <typename T>
using disable_if_is_pointer_type_t =
    type_traits::enable_if_t<!std::is_pointer<T>::value>;

template <typename T>
using disable_if_is_rvalue_reference_type_t =
    type_traits::enable_if_t<!std::is_rvalue_reference<T>::value>;

template <typename T>
using remove_pointer_t = type_traits::remove_pointer_t<T>;

template <typename T>
using remove_cv_t = type_traits::remove_cv_t<T>;

template <typename T>
using remove_reference_t = type_traits::remove_reference_t<T>;

template <typename T>
using add_lvalue_reference_t = typename std::add_lvalue_reference<T>::type;

template <typename T>
using add_const_t = typename std::add_const<T>::type;

template <
    typename T,
    bool = type_traits::is_non_const_lvalue_reference<T>::value>
struct __is_not_non_const_lvalue_reference_impl : std::true_type {};

template <typename T>
struct __is_not_non_const_lvalue_reference_impl<T, true> : std::false_type {};

template <typename T>
struct is_not_non_const_lvalue_reference
    : __is_not_non_const_lvalue_reference_impl<T> {};
//
//////////////////////////////

// mimicking boost::Any
//
// syntax:
//
// - `int value = 0; Any any = value; // asign`
// - `int *value_ptr = any_cast<int *>(&any); // cast as pointer`
// - `int &value_ref = any_cast<int &>(any); // cast as reference`
// - ``AnotherClass another_value = any_cast<AnotherClass>(value); // fail,
//   throw `bad_any_cast` ``
class Any {
private:
    class HolderBase {
    public:
        virtual ~HolderBase() = default;

        virtual const std::type_info &type() const noexcept = 0;

        virtual HolderBase *clone() const = 0;
    };

    template <typename DecayedValueType>
    class Holder final : public HolderBase {
    public:
        Holder(const DecayedValueType &value) : _value(value) {
        }

        Holder(DecayedValueType &&value) : _value(std::move(value)) {
        }

        // no copy (use clone instead)
        Holder(const Holder &) = delete;
        Holder &operator=(const Holder &) = delete;

        // no move (life time is controlled by wrapper class)
        Holder(Holder &&) = delete;
        Holder &operator=(Holder &&) = delete;

        ~Holder() override = default;

        const std::type_info &type() const noexcept override {
            return typeid(DecayedValueType);
        }

        HolderBase *clone() const override {
            return new Holder<DecayedValueType>(_value);
        }

        // - cannot be private since friend of `Any` doesn't mean friend of
        // `Holder`
        // - on the other hand, it is safe to be public since the `Holder`
        // pointer itself is private inside `Any`
    public:
        DecayedValueType _value;
    };

public:
    // default
    Any() noexcept : _holder_base(nullptr) {
    }

    // copy
    Any(const Any &other) : _holder_base(other._holder_base->clone()) {
    }

    // copy
    Any &operator=(const Any &other) {
        Any(other).swap(*this);

        return *this;
    }

    // move
    Any(Any &&other) noexcept : _holder_base(other._holder_base) {
        other._holder_base = nullptr;
    }

    // move
    Any &operator=(Any &&other) noexcept {
        other.swap(*this);
        Any().swap(other);

        return *this;
    }

    // perfect forwarding for value
    template <
        typename ValueType,
        typename =
            type_traits::disable_if_t<std::is_same<ValueType, Any &>::value>>
    Any(ValueType &&value)
        : _holder_base(new Holder<type_traits::decay_t<ValueType>>(
            std::forward<ValueType>(value)
        )) {
    }

    // perfect forwarding for value
    template <typename ValueType>
    Any &operator=(ValueType &&value) {
        Any(std::forward<ValueType>(value)).swap(*this);

        return *this;
    }

    ~Any() {
        delete _holder_base;
    }

    Any &swap(Any &other) noexcept {
        auto temp = _holder_base;
        _holder_base = other._holder_base;
        other._holder_base = temp;

        return *this;
    }

    bool empty() const noexcept {
        return _holder_base == nullptr;
    }

    void clear() noexcept {
        Any().swap(*this);
    }

    const std::type_info &type() const noexcept {
        return _holder_base ? _holder_base->type() : typeid(void);
    }

private:
    // safe: returns a nullptr if the casting fails
    template <
        typename PointerType,
        typename = enable_if_is_pointer_type_t<PointerType>>
    friend PointerType any_cast(Any *source) noexcept {
        using DecayedValueType = remove_cv_t<remove_pointer_t<PointerType>>;

        return source && source->type() == typeid(DecayedValueType)
                   ? address_of(static_cast<Any::Holder<DecayedValueType> *>(
                                    source->_holder_base
                   )
                                    ->_value)
                   : nullptr;
    }

    // [NOTE]: might lead to memory leaks if exceptions are thrown
    HolderBase *_holder_base;
};

// safe: compile-time error if the user tries to convert to non-const pointer
template <typename PointerType>
inline PointerType any_cast(const Any *source) noexcept {
    using DecayedValueType = remove_cv_t<remove_pointer_t<PointerType>>;

    // generates compile-time error if `PointerType` is non-const
    using ConstPointerType = const DecayedValueType *;

    return any_cast<ConstPointerType>(const_cast<Any *>(source));
}

class bad_any_cast : public std::bad_cast {
public:
    const char *what() const noexcept override {
        return "bad any_cast";
    }
};

template <
    typename ValueOrReferenceType,
    typename = disable_if_is_pointer_type_t<ValueOrReferenceType>>
ValueOrReferenceType any_cast(Any &source) {
    using NonReferenceValueType = remove_reference_t<ValueOrReferenceType>;

    using PointerType = NonReferenceValueType *;

    PointerType value_ptr = any_cast<PointerType>(address_of(source));

    if (!value_ptr) {
        throw bad_any_cast();
    }

    // for preventing the unnecessary copy when static_cast to a
    // non-reference type, e.g. the copying `std::string(*value_ptr)` when doing
    // `static_cast<std::string>(*value_ptr)`
    using ReferenceType = add_lvalue_reference_t<ValueOrReferenceType>;

    return static_cast<ReferenceType>(*value_ptr);
}

// safe: compile-time error if the user tries to convert to non-const reference
template <
    typename ValueOrReferenceType,
    typename = disable_if_is_pointer_type_t<ValueOrReferenceType>>
inline ValueOrReferenceType any_cast(const Any &source) {
    using NonReferenceValueType = remove_reference_t<ValueOrReferenceType>;

    // generates compile-time error if `ValueOrReferenceType` is non-const
    // reference type
    using ConstValueOrReferenceType = add_const_t<ValueOrReferenceType>;

    // for preventing the unnecessary copy when static_cast to a
    // non-reference type, e.g. the copying `std::string(*value_ptr)` when doing
    // `static_cast<std::string>(*value_ptr)`
    using ConstReferenceType =
        add_lvalue_reference_t<ConstValueOrReferenceType>;

    return any_cast<ConstReferenceType>(const_cast<Any &>(source));
}

template <
    typename ValueOrReferenceType,
    typename = disable_if_is_pointer_type_t<ValueOrReferenceType>>
ValueOrReferenceType any_cast(Any &&source) {
    // generates compile-time error only if `ValueOrReferenceType` is non-const
    // lvalue reference type
    static_assert(
        is_not_non_const_lvalue_reference<ValueOrReferenceType>::value,
        "tried to bind a temporary value to a non-const lvalue reference"
    );

    // for preventing the unnecessary copy when static_cast to a
    // non-reference type, e.g. the copying `std::string(*value_ptr)` when doing
    // `static_cast<std::string>(*value_ptr)`
    using ReferenceType = add_lvalue_reference_t<ValueOrReferenceType>;

    return any_cast<ReferenceType>(source);
}

} // namespace util

} // namespace xubinh_server

#endif