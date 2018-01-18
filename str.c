
#include "str.h"
#include "garbage_collector.h"

void free_str(string *str) {
    free(str->str);
    str->str = NULL;
    str->length = 0;
    str->alloc_size = 0;
}

int duplicate_str(string *duplicate, string *source) {
    if ((duplicate->str = (char *) gc_alloc_pointer(strlen(source->str) +1)) == NULL) {
        return STR_ALLOC_ERROR;
    }
    strcpy(duplicate->str, source->str);
    duplicate->length = source->length;
    duplicate->alloc_size = source->alloc_size;
    return STR_OK;
}

int init_value_str(string *str, const char *value_str) {
    int size;
    if (value_str == NULL)
        return STR_ERROR;

    size = strlen(value_str);

    if ((str->str = (char *) gc_alloc_pointer((size + 1) * sizeof(char))) == NULL)
        return STR_ALLOC_ERROR;

    strncpy(str->str, value_str, size);
    str->str[size] = '\0';
    str->length = size;
    str->alloc_size = size + 1;

    return STR_OK;
}
