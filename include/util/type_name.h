#ifndef __XUBINH_SERVER_UTIL_TYPE_NAME
#define __XUBINH_SERVER_UTIL_TYPE_NAME

#include <cxxabi.h>
#include <typeinfo>

namespace xubinh_server {

namespace util {

template <typename T>
class TypeName {
public:
    static std::string get_name() {
        return _get_demangled_name_if_possible();
    }

private:
    static std::string _get_demangled_name_if_possible() {
#ifdef __GNUC__
        int status;

        char *demangled_name_c_str =
            abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, &status);

        std::string demangled_name = (status == 0 && demangled_name_c_str)
                                         ? demangled_name_c_str
                                         : typeid(T).name();

        ::free(demangled_name_c_str);

        return demangled_name;
#else
        return typeid(T).name();
#endif
    }
};

} // namespace util

} // namespace xubinh_server

#endif