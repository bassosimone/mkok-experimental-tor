// Part of measurement-kit <https://measurement-kit.github.io/>.
// Measurement-kit is free software. See AUTHORS and LICENSE for more
// information on the copying conditions.

#include <measurement_kit/common/object.hpp>

namespace mk {

class ObjectImpl {
  public:
    virtual ObjectType get_type() { return ObjectType::None; }
    virtual Dict *get_dict() { return nullptr; }
    virtual Double *get_double() { return nullptr; }
    virtual Int *get_int() { return nullptr; }
    virtual List *get_list() { return nullptr; }
    virtual Str *get_str() { return nullptr; }
};

#define XX(Type, type)                                                         \
                                                                               \
    class Object##Type : public ObjectImpl {                                   \
      public:                                                                  \
        ObjectType get_type() override { return ObjectType::Type; }            \
        Type *get_##type() override { return &val; }                           \
        Object##Type(Type v) : val(v) {}                                       \
                                                                               \
      private:                                                                 \
        Type val;                                                              \
    };                                                                         \
                                                                               \
    Object::Object(Type value) : ctx(new Object##Type(value)) {}               \
                                                                               \
    bool Object::is_##type() const {                                           \
        return ctx->get_type() == ObjectType::Type;                            \
    }                                                                          \
                                                                               \
    Maybe<Type> Object::as_##type() const {                                    \
        Type *ptr = ctx->get_##type();                                         \
        if (ptr == nullptr) {                                                  \
            return Maybe<Type>(TypeError(), Type());                           \
        }                                                                      \
        return Maybe<Type>(*ptr);                                              \
    }

XX(Dict, dict)
XX(Double, double)
XX(Int, int)
XX(List, list)
XX(Str, str)

#undef XX

Object::Object() : ctx(new ObjectImpl) {}

ObjectType Object::get_type() const { return ctx->get_type(); }

ObjectTypeSwitch Object::switch_type() const { return ObjectTypeSwitch(ctx); }

bool Object::operator<(Object other) const {
    if (ctx->get_type() != other.ctx->get_type()) {
        return ctx->get_type() < other.ctx->get_type();
    }
    if (ctx->get_type() == ObjectType::None) {
        return true;
    }
    if (ctx->get_type() == ObjectType::Dict) {
        return *ctx->get_dict() < *other.ctx->get_dict();
    }
    if (ctx->get_type() == ObjectType::Double) {
        return *ctx->get_double() < *other.ctx->get_double();
    }
    if (ctx->get_type() == ObjectType::Int) {
        return *ctx->get_int() < *other.ctx->get_int();
    }
    if (ctx->get_type() == ObjectType::List) {
        return *ctx->get_list() < *other.ctx->get_list();
    }
    if (ctx->get_type() == ObjectType::Str) {
        return *ctx->get_str() < *other.ctx->get_str();
    }
    throw GenericError(); // Should not happen
}

ObjectTypeSwitch::ObjectTypeSwitch(Var<ObjectImpl> k) : ctx(k) {}

ObjectTypeSwitch::~ObjectTypeSwitch() {

#define XX(Type, type)                                                         \
    do {                                                                       \
        if (ctx->get_type() == ObjectType::Type) {                             \
            if (case_##type##_fn) {                                            \
                case_##type##_fn(*ctx->get_##type());                          \
            } else if (otherwise_fn) {                                         \
                otherwise_fn();                                                \
            } else {                                                           \
                /* nothing */;                                                 \
            }                                                                  \
            return;                                                            \
        }                                                                      \
    } while (0)

    XX(Dict, dict);
    XX(Double, double);
    XX(Int, int);
    XX(List, list);
    XX(Str, str);

#undef XX

    if (ctx->get_type() == ObjectType::None) {
        if (case_none_fn) {
            case_none_fn();
        } else if (otherwise_fn) {
            otherwise_fn();
        } else {
            /* nothing */;
        }
        return;
    }

    throw GenericError(); // Should not happen
}

} // namespace mk
