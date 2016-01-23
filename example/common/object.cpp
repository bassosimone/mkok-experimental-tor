// Part of measurement-kit <https://measurement-kit.github.io/>.
// Measurement-kit is free software. See AUTHORS and LICENSE for more
// information on the copying conditions.

#include <measurement_kit/common/object.hpp>
#include <iostream>

using namespace mk;

static Object create_object() {
    Dict person{
        {"name", "Simone Basso"},
        {"age", 33},
        {"weight", 64.2}
    };
    person["height"] = 1.65;
    person["address"] = Dict{
        {"city", "Turin"},
        {"region", "Piedmont"},
        {"country", "IT"}
    };
    person["favorite_colors"] = List{"red", "green", "black"};
    return person;
}

static void print_scalar(Object obj) {
    obj.switch_type()
        .case_int([](Int x) { std::cout << x; })
        .case_double([](Double x) { std::cout << x; })
        .case_str([](Str x) { std::cout << x; })
        .otherwise([]() { std::cout << " -not a scalar- "; });
}

int main() {
    Object object = create_object();

    object.switch_type().case_dict([](Dict dict) {
        Object::for_each(dict, [](Object key, Object value) {

            print_scalar(key);

            std::cout << ": ";

            value.switch_type()
                .case_int([](Int x) { std::cout << x; })
                .case_double([](Double x) { std::cout << x; })
                .case_str([](Str x) { std::cout << x; })

                .case_dict([](Dict dict) {
                    std::cout << "{";
                    Object::for_each(dict, [](Object key, Object value) {
                        print_scalar(key);
                        std::cout << ": ";
                        print_scalar(value);
                        std::cout << ", ";
                    });
                    std::cout << "}";
                })

                .case_list([](List list) {
                    std::cout << "[";
                    Object::for_each(list, [](Object elem) {
                        print_scalar(elem);
                        std::cout << ", ";
                    });
                    std::cout << "]";
                });

            std::cout << "\n";
        });
    });

    object.switch_type()
        .case_int([](Int) { /* nothing */ })
        .otherwise([]() { std::cout << "OTHERWISE\n"; });
}
