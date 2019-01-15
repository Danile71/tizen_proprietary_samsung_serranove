#ifndef utility_h
#define utility_h

#define LIST(var) std::cout << var << std::endl

#ifdef DEBUG
    #define DRT_DEBUG(...)                    \
    do                                         \
    {                                          \
        fprintf(stderr, __VA_ARGS__);       \
    }                                          \
    while (0)
#else
    #define DRT_DEBUG(...)
#endif

class ENoncopyable {
    ENoncopyable(const ENoncopyable&);
    ENoncopyable& operator = (const ENoncopyable&);
public:
    ENoncopyable() {}
    virtual ~ENoncopyable() {}
};

#endif //
