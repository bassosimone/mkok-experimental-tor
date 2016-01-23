// Part of measurement-kit <https://measurement-kit.github.io/>.
// Measurement-kit is free software. See AUTHORS and LICENSE for more
// information on the copying conditions.

#ifndef MEASUREMENT_KIT_COMMON_OBJECT_HPP
#define MEASUREMENT_KIT_COMMON_OBJECT_HPP

#include <list>
#include <map>
#include <measurement_kit/common/error.hpp>
#include <measurement_kit/common/maybe.hpp>
#include <measurement_kit/common/var.hpp>
#include <string>

namespace mk {

class Object; // Forward decl.

typedef std::map<Object, Object> Dict; ///< Dictionary type
typedef double Double;                 ///< Double type
typedef int Int;                       ///< Int type
typedef std::list<Object> List;        ///< List type
typedef std::string Str;               ///< String type

/// Type of object
enum class ObjectType {
    None = 0, ///< The underlying type is none
    Dict,     ///< The underlying type is a dictionary
    Double,   ///< The underlying type is a double
    Int,      ///< The underlying type is a int
    List,     ///< The underlying type is a list
    Str       ///< The underlying type is a string
};

// Forward declarations
class ObjectImpl;
class ObjectTypeSwitch;

/// A generic object
class Object {
  public:
    Object();       ///< Initialize empty object
    Object(Dict);   ///< Initialize from dict
    Object(Double); ///< Initialize from double
    Object(Int);    ///< Initialize from int
    Object(List);   ///< Initialize from list
    Object(Str);    ///< Initialize from str

    /// Initialize object from C string.
    Object(const char *value) : Object(std::string(value)) {}

    bool is_dict() const;   ///< Returns true if object is dict
    bool is_double() const; ///< Returns true if object is double
    bool is_int() const;    ///< Returns true if object is int
    bool is_list() const;   ///< Returns true if object is list
    bool is_str() const;    ///< Returns true if object is str

    Maybe<Dict> as_dict() const;     ///< Converts object to dict
    Maybe<Double> as_double() const; ///< Converts object to double
    Maybe<Int> as_int() const;       ///< Converts object to int
    Maybe<List> as_list() const;     ///< Converts object to list
    Maybe<Str> as_str() const;       ///< Converts object to str

    /// Return type of object.
    ObjectType get_type() const;

    /// Lazy switch through all the possible types.
    ObjectTypeSwitch switch_type() const;

    /// Lower-than comparator (required to implement Dict).
    bool operator<(Object other) const;

    /// Syntactic sugar to walk through dict.
    static void for_each(Dict dict, std::function<void(Object, Object)> func) {
        for (auto pair : dict) {
            func(pair.first, pair.second);
        }
    }

    /// Syntactic sugar to walk through list.
    static void for_each(List list, std::function<void(Object)> func) {
        for (auto elem : list) {
            func(elem);
        }
    }

  private:
    Var<ObjectImpl> ctx;
};

/// Allow to lazy switch through all the possible types.
class ObjectTypeSwitch {
  public:
    /// Constructor that captures references to object context.
    ObjectTypeSwitch(Var<ObjectImpl>);

    /// Function to call if object is None.
    ObjectTypeSwitch &case_none(std::function<void()> fn) {
        case_none_fn = fn;
        return *this;
    }

    /// Function to call if object is dict.
    ObjectTypeSwitch &case_dict(std::function<void(Dict)> fn) {
        case_dict_fn = fn;
        return *this;
    }

    /// Function to call if object is double.
    ObjectTypeSwitch &case_double(std::function<void(double)> fn) {
        case_double_fn = fn;
        return *this;
    }

    /// Function to call if object is int.
    ObjectTypeSwitch &case_int(std::function<void(int)> fn) {
        case_int_fn = fn;
        return *this;
    }

    /// Function to call if object is List.
    ObjectTypeSwitch &case_list(std::function<void(List)> fn) {
        case_list_fn = fn;
        return *this;
    }

    /// Function to call if object is Str.
    ObjectTypeSwitch &case_str(std::function<void(Str)> fn) {
        case_str_fn = fn;
        return *this;
    }

    /// Function that emultes the `default` clause.
    void otherwise(std::function<void()> fn) { otherwise_fn = fn; }

    /// Executes the lazy switch.
    ~ObjectTypeSwitch();

  private:
    Var<ObjectImpl> ctx;
    std::function<void()> case_none_fn;
    std::function<void(Dict)> case_dict_fn;
    std::function<void(double)> case_double_fn;
    std::function<void(int)> case_int_fn;
    std::function<void(List)> case_list_fn;
    std::function<void(Str)> case_str_fn;
    std::function<void()> otherwise_fn;
};

} // namespace mk
#endif
