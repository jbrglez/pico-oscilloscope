#ifndef ERROR_PRINT_C
#define ERROR_PRINT_C

#include "my_strings.c"


#define RESET   "\033[0m"

#define BLACK   "\033[30m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"
#define WHITE   "\033[37m"


void print_std_error(char *str_1, char *str_2) {
    fprintf(stderr, RED "ERROR:" RESET " [%s] : %s", str_1, str_2);
}

void print_std_warn(char *str_1, char *str_2) {
    fprintf(stderr, YELLOW "WARNING:" RESET " [%s] : %s", str_1, str_2);
}

void print_number_out_of_range(i32 min, i32 max, i32 val) {
    fprintf(stderr, RED "ERROR:" RESET " [Value out of range] : Expected value in range %d-%d, found %d.\n", min, max, val);
}

void print_stderror_slice(char *str_1, slice_t slice, char *str_2) {
    fprintf(stderr, "%s%.*s%s", str_1, slice.len, slice.str, str_2);
}

void print_stderror_slice_array(char *str_1, slice_array_t array, char *str_2) {
    fprintf(stderr, "%s ", str_1);
    for (i32 i = 0; i < array.length; i++) {
        fprintf(stderr, "%.*s ", array.items[i].len, array.items[i].str);
    }
    fprintf(stderr, "%s", str_2);
}


void print_std_error_format(char *format, slice_array_t line_words) {
    fprintf(stderr, RED "ERROR:" RESET " [Parsing failed] : Expected format: %s\n", format);
    print_stderror_slice_array("Failed to parse line >", line_words, "\n");
}


void print_stderror_color_word_in_line(char *str_1, slice_array_t line, slice_t word, char *str_2) {
    fprintf(stderr, "%s ", str_1);
    for (i32 i = 0; i < line.length; i++) {
        if (are_slices_equal(line.items[i], word)) {
            // fprintf(stderr, "%s%.*s%s ", line.items[i].len, MAGENTA, line.items[i].str, RESET);
            fprintf(stderr, "\033[35m%.*s\033[00m ", line.items[i].len, line.items[i].str);
        }
        else {
            fprintf(stderr, "%.*s ", line.items[i].len, line.items[i].str);
        }
    }
    fprintf(stderr, "%s", str_2);
}

void print_stderror_line_parse_failed_at(slice_array_t line, slice_t word) {
    print_stderror_color_word_in_line( YELLOW "INFO:" RESET "  [Error on line] >", line, word, "\n");
}

void print_line_parse_failed(slice_array_t words) {
    print_std_warn("Failed to parse line", " ");
    print_stderror_slice_array("", words, "\n");
}


b32 print_line_failed_many_few(i32 min, i32 max, slice_array_t line_words) {
    if (line_words.length < min) {
        print_std_error("Failed to parse", "Too few arguments.");
        print_stderror_slice_array(" Line :", line_words, "\n");
        return TRUE;
    }
    else if (line_words.length > max) {
        print_std_error("Failed to parse", "Too many arguments.");
        print_stderror_slice_array(" Line :", line_words, "\n");
        return TRUE;
    }
    else {
        return FALSE;
    }
}



#endif
