#ifndef MY_ASSERT_H
#define MY_ASSERT_H


#define ASCII_RESET   "\033[0m"

#define ASCII_BLACK   "\033[30m"
#define ASCII_RED     "\033[31m"
#define ASCII_GREEN   "\033[32m"
#define ASCII_YELLOW  "\033[33m"
#define ASCII_BLUE    "\033[34m"
#define ASCII_MAGENTA "\033[35m"
#define ASCII_CYAN    "\033[36m"
#define ASCII_WHITE   "\033[37m"

#ifndef DEBUG_MODE
#define ASSERT(...) ((void)0)
#else
#define ASSERT(test, ...) \
if(!(test)) \
    {\
        fprintf(stderr, ASCII_RED "\nERROR: " ASCII_RESET "ASSERT failed in file: %s, line: %s%d%s\n", ASCII_YELLOW __FILE__ ASCII_RESET, ASCII_YELLOW ,__LINE__, ASCII_RESET);\
        fprintf(stderr, "\t%s", ASCII_RED "ASSERT" ASCII_RESET "(" #test ")\n");\
        fprintf(stderr, "\t" __VA_ARGS__);\
        fprintf(stderr, "\n");\
        *(volatile int *)0 = 0;\
    }
#endif


#endif
