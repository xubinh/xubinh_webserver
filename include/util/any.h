#ifndef __XUBINH_SERVER_UTIL_ANY
#define __XUBINH_SERVER_UTIL_ANY

#include <type_traits>
#include <typeinfo>

#include "util/address_of.h"
#include "util/type_traits.h"

namespace xubinh_server {

namespace util {

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
    class Holder : public HolderBase {
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

    private:
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
    template <typename AssumeNonReferenceValueType>
    friend AssumeNonReferenceValueType *any_cast(Any *source) noexcept;

    HolderBase *_holder_base;
};

// safe: returns a nullptr if the casting fails
template <
    typename PointerType,
    typename = type_traits::enable_if_t<std::is_pointer<PointerType>::value>>
PointerType any_cast(Any *source) noexcept {
    using DecayedValueType =
        type_traits::remove_cv_t<type_traits::remove_pointer_t<PointerType>>;

    return source && source->type() == typeid(DecayedValueType)
               ? address_of(static_cast<Any::Holder<DecayedValueType> *>(
                                source->_holder_base
               )
                                ->_value)
               : nullptr;
}

// safe: compile-time error if tries to convert const Any to mutable pointer
template <typename PointerType>
inline PointerType any_cast(const Any *source) noexcept {
    using DecayedValueType =
        type_traits::remove_cv_t<type_traits::remove_pointer_t<PointerType>>;

    // makes sure it's const
    return static_cast<const DecayedValueType *>(
        any_cast<PointerType>(const_cast<Any *>(source))
    );
}

class bad_any_cast : public std::bad_cast {
public:
    const char *what() const noexcept override {
        return "bad any_cast";
    }
};

template <typename ValueOrReferenceType>
ValueOrReferenceType any_cast(Any &source) {
    using NonReferenceValueType =
        type_traits::remove_reference_t<ValueOrReferenceType>;

    NonReferenceValueType *value_ptr =
        any_cast<NonReferenceValueType *>(address_of(source));

    if (value_ptr == nullptr) {
        throw bad_any_cast();
    }

    // for preventing the unnecessary copy when static_cast to a
    // non-reference type, e.g. the copying `std::string(*value_ptr)` when doing
    // `static_cast<std::string>(*value_ptr)`
    using ReferenceType =
        typename std::add_lvalue_reference<ValueOrReferenceType>::type;

    return static_cast<ReferenceType>(*value_ptr);
}

template <typename ValueOrReferenceType>
inline ValueOrReferenceType any_cast(const Any &source) {
    using NonReferenceValueType =
        type_traits::remove_reference_t<ValueOrReferenceType>;

    // makes sure it's const
    return static_cast<const NonReferenceValueType &>(
        any_cast<ValueOrReferenceType>(const_cast<Any &>(source))
    );
}

template <typename ValueOrReferenceType>
ValueOrReferenceType any_cast(Any &&source) {
    static_assert(
        !type_traits::is_non_const_lvalue_reference<
            ValueOrReferenceType>::value,
        "tried to bind a temporary value to a non-const lvalue reference"
    );

    return any_cast<ValueOrReferenceType>(source);
}

} // namespace util

} // namespace xubinh_server

#endif