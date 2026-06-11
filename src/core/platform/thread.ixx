module;
#if defined(_WIN32)
#include <windows.h>
#elif defined(__APPLE__) || defined(__linux__)
#include <pthread.h>
#endif
export module nekomata2.core.platform.thread;
import std;

export namespace nekomata2 {

#if defined(_WIN32)
inline void setThreadName(const std::string& name) {
    std::wstring name_wstring(name.begin(), name.end());
    SetThreadDescription(GetCurrentThread(), name_wstring.c_str());
}
#elif defined(__APPLE__)
inline void setThreadName(const std::string& name) {
    pthread_setname_np(name.c_str());
}
#elif defined(__linux__)
inline void setThreadName(const std::string& name) {
    pthread_setname_np(pthread_self(), name.c_str());
}
#else
inline void setThreadName(const std::string&) {
    // Not supported
}
#endif

}