#include "row.h"
#include "cell.h"
#include "string_buffer.h"
#include "assert.h"
#include "vector.h"
#include "ctype.h"

struct fort_row
{
    vector_t *cells;
    /*enum RowType type;*/
};



fort_row_t * create_row()
{
    fort_row_t * row = F_CALLOC(sizeof(fort_row_t), 1);
    if (row == NULL)
        return NULL;
    row->cells = create_vector(sizeof(fort_cell_t*), DEFAULT_VECTOR_CAPACITY);
    if (row->cells == NULL) {
        F_FREE(row);
        return NULL;
    }

    /*
    row->is_header = F_FALSE;
    row->type = Common;
    */
    return row;
}

void destroy_row(fort_row_t *row)
{
    size_t i = 0;
    if (row == NULL)
        return;

    if (row->cells) {
        size_t cells_n = vector_size(row->cells);
        for (i = 0; i < cells_n; ++i) {
            fort_cell_t *cell = *(fort_cell_t **)vector_at(row->cells, i);
            destroy_cell(cell);
        }
        destroy_vector(row->cells);
    }

    F_FREE(row);
}





int columns_in_row(const fort_row_t *row)
{
    if (row == NULL || row->cells == NULL)
        return 0;

    return vector_size(row->cells);
}


fort_cell_t *get_cell_implementation(fort_row_t *row, size_t col, enum PolicyOnNull policy)
{
    if (row == NULL || row->cells == NULL) {
        return NULL;
    }

    switch (policy) {
        case DoNotCreate:
            if (col < columns_in_row(row)) {
                return *(fort_cell_t**)vector_at(row->cells, col);
            }
            return NULL;
            break;
        case Create:
            while(col >= columns_in_row(row)) {
                fort_cell_t *new_cell = create_cell();
                if (new_cell == NULL)
                    return NULL;
                if (IS_ERROR(vector_push(row->cells, &new_cell))) {
                    destroy_cell(new_cell);
                    return NULL;
                }
            }
            return *(fort_cell_t**)vector_at(row->cells, col);
            break;
    }
    return NULL;
}


fort_cell_t *get_cell(fort_row_t *row, size_t col)
{
    return get_cell_implementation(row, col, DoNotCreate);
}

const fort_cell_t *get_cell_c(const fort_row_t *row, size_t col)
{
    return get_cell((fort_row_t *)row, col);
}

fort_cell_t *get_cell_and_create_if_not_exists(fort_row_t *row, size_t col)
{
    return get_cell_implementation(row, col, Create);
}





int print_row_separator(char *buffer, size_t buffer_sz,
                               const size_t *col_width_arr, size_t cols,
                               const fort_row_t *upper_row, const fort_row_t *lower_row,
                               enum HorSeparatorPos separatorPos,
                               const separator_t *sep, const context_t *context)
{
#define CHECK_RESULT_AND_MOVE_DEV(statement) \
    k = statement; \
    if (k < 0) {\
        goto clear; \
    } \
    dev += k;

    typedef char char_type;
    char new_line_char = '\n';
    int (*snprint_n_chars_)(char *, size_t , size_t , char) = snprint_n_chars;


    assert(buffer);
    assert(context);

    int dev = 0;
    int k = 0;

    enum RowType lower_row_type = Common;
    if (lower_row != NULL) {
        lower_row_type = get_cell_opt_value_hierarcial(context->table_options, context->row, FT_ANY_COLUMN, FT_OPT_ROW_TYPE);
    }
    enum RowType upper_row_type = Common;
    if (upper_row != NULL) {
        upper_row_type = get_cell_opt_value_hierarcial(context->table_options, context->row-1, FT_ANY_COLUMN, FT_OPT_ROW_TYPE);
    }


    /*  Row separator anatomy
     *
     *  L  I  I  I  IV  I   I   I  R
     */
    const char *L = NULL;
    const char *I = NULL;
    const char *IV = NULL;
    const char *R = NULL;

    const char (*border_chars)[BorderItemPosSize] = NULL;
    border_chars = &context->table_options->border_chars;
    if (upper_row_type == Header || lower_row_type == Header) {
        border_chars = &context->table_options->header_border_chars;
    }

    if (sep && sep->enabled) {
        L = &(context->table_options->separator_chars[LH_sip]);
        I = &(context->table_options->separator_chars[IH_sip]);
        IV = &(context->table_options->separator_chars[II_sip]);
        R = &(context->table_options->separator_chars[RH_sip]);
    } else {
        switch (separatorPos) {
            case TopSeparator:
                L = &(*border_chars)[TL_bip];
                I = &(*border_chars)[TT_bip];
                IV = &(*border_chars)[TV_bip];
                R = &(*border_chars)[TR_bip];
                break;
            case InsideSeparator:
                L = &(*border_chars)[LH_bip];
                I = &(*border_chars)[IH_bip];
                IV = &(*border_chars)[II_bip];
                R = &(*border_chars)[RH_bip];
                break;
            case BottomSeparator:
                L = &(*border_chars)[BL_bip];
                I = &(*border_chars)[BB_bip];
                IV = &(*border_chars)[BV_bip];
                R = &(*border_chars)[BR_bip];
                break;
            default:
                break;
        }
    }

    /* If all chars are not printable, skip line separator */  /* todo: add processing for wchar_t */
    if (!isprint(*L) && !isprint(*I) && !isprint(*IV) && !isprint(*R))
        return 0;

    size_t i = 0;
    for (i = 0; i < cols; ++i) {
        if (i == 0) {
            CHECK_RESULT_AND_MOVE_DEV(snprint_n_chars_(buffer + dev, buffer_sz - dev, 1, (char_type)*L));
        } else {
            CHECK_RESULT_AND_MOVE_DEV(snprint_n_chars_(buffer + dev, buffer_sz - dev, 1, (char_type)*IV));
        }
        CHECK_RESULT_AND_MOVE_DEV(snprint_n_chars_(buffer + dev, buffer_sz - dev, col_width_arr[i], (char_type)*I));
    }
    CHECK_RESULT_AND_MOVE_DEV(snprint_n_chars_(buffer + dev, buffer_sz - dev, 1, (char_type)*R));

    CHECK_RESULT_AND_MOVE_DEV(snprint_n_chars_(buffer + dev, buffer_sz - dev, 1, new_line_char));

    return dev;

clear:
    return -1;

#undef CHECK_RESULT_AND_MOVE_DEV
}



int wprint_row_separator(wchar_t *buffer, size_t buffer_sz,
                               const size_t *col_width_arr, size_t cols,
                               const fort_row_t *upper_row, const fort_row_t *lower_row,
                               enum HorSeparatorPos separatorPos, const separator_t *sep,
                               const context_t *context)
{
#define CHECK_RESULT_AND_MOVE_DEV(statement) \
    k = statement; \
    if (k < 0) {\
        goto clear; \
    } \
    dev += k;

    typedef wchar_t char_type;
    char new_line_char = L'\n';
    int (*snprint_n_chars_)(wchar_t*, size_t , size_t , wchar_t) = wsnprint_n_chars;


    assert(buffer);
    assert(context);

    int dev = 0;
    int k = 0;

    enum RowType lower_row_type = Common;
    if (lower_row != NULL) {
        lower_row_type = get_cell_opt_value_hierarcial(context->table_options, context->row, FT_ANY_COLUMN, FT_OPT_ROW_TYPE);
    }
    enum RowType upper_row_type = Common;
    if (upper_row != NULL) {
        upper_row_type = get_cell_opt_value_hierarcial(context->table_options, context->row-1, FT_ANY_COLUMN, FT_OPT_ROW_TYPE);
    }

    /*  Row separator anatomy
     *
     *  L  I  I  I  IV  I   I   I  R
     */
    const char *L = NULL;
    const char *I = NULL;
    const char *IV = NULL;
    const char *R = NULL;

    const char (*border_chars)[BorderItemPosSize] = NULL;
    border_chars = &context->table_options->border_chars;
    if (upper_row_type == Header || lower_row_type == Header) {
        border_chars = &context->table_options->header_border_chars;
    }


    if (sep && sep->enabled) {
        L = &(context->table_options->separator_chars[LH_sip]);
        I = &(context->table_options->separator_chars[IH_sip]);
        IV = &(context->table_options->separator_chars[II_sip]);
        R = &(context->table_options->separator_chars[RH_sip]);
    } else {
        switch (separatorPos) {
            case TopSeparator:
                L = &(*border_chars)[TL_bip];
                I = &(*border_chars)[TT_bip];
                IV = &(*border_chars)[TV_bip];
                R = &(*border_chars)[TR_bip];
                break;
            case InsideSeparator:
                L = &(*border_chars)[LH_bip];
                I = &(*border_chars)[IH_bip];
                IV = &(*border_chars)[II_bip];
                R = &(*border_chars)[RH_bip];
                break;
            case BottomSeparator:
                L = &(*border_chars)[BL_bip];
                I = &(*border_chars)[BB_bip];
                IV = &(*border_chars)[BV_bip];
                R = &(*border_chars)[BR_bip];
                break;
            default:
                break;
        }
    }

    /* If all chars are not printable, skip line separator */  /* todo: add processing for wchar_t */
    if (!isprint(*L) && !isprint(*I) && !isprint(*IV) && !isprint(*R))
        return 0;

    size_t i = 0;
    for (i = 0; i < cols; ++i) {
        if (i == 0) {
            CHECK_RESULT_AND_MOVE_DEV(snprint_n_chars_(buffer + dev, buffer_sz - dev, 1, (char_type)*L));
        } else {
            CHECK_RESULT_AND_MOVE_DEV(snprint_n_chars_(buffer + dev, buffer_sz - dev, 1, (char_type)*IV));
        }
        CHECK_RESULT_AND_MOVE_DEV(snprint_n_chars_(buffer + dev, buffer_sz - dev, col_width_arr[i], (char_type)*I));
    }
    CHECK_RESULT_AND_MOVE_DEV(snprint_n_chars_(buffer + dev, buffer_sz - dev, 1, (char_type)*R));

    CHECK_RESULT_AND_MOVE_DEV(snprint_n_chars_(buffer + dev, buffer_sz - dev, 1, new_line_char));

    return dev;

clear:
    return -1;

#undef CHECK_RESULT_AND_MOVE_DEV
}



fort_row_t* create_row_from_string(const char *str)
{
    fort_row_t * row = create_row();
    if (row == NULL)
        return NULL;

    if (str == NULL)
        return row;

    char *str_copy = F_STRDUP(str);
    if (str_copy == NULL)
        goto clear;

    char *pos = str_copy;
    char *base_pos = str_copy;
    int number_of_separators = 0;
    while (*pos) {
        pos = strchr(pos, FORT_COL_SEPARATOR);
        if (pos != NULL) {
            *(pos) = '\0';
            ++pos;
            number_of_separators++;
        }

        fort_cell_t *cell = create_cell();
        if (cell == NULL)
            goto clear;

/*        int status = fill_buffer_from_string(cell->str_buffer, base_pos);  */
        int status = fill_cell_from_string(cell, base_pos);
        if (IS_ERROR(status)) {
            destroy_cell(cell);
            goto clear;
        }

        status = vector_push(row->cells, &cell);
        if (IS_ERROR(status)) {
            destroy_cell(cell);
            goto clear;
        }

        if (pos == NULL)
            break;
        base_pos = pos;
    }

    /* special case if in format string last cell is empty */
    while (vector_size(row->cells) < (number_of_separators + 1)) {
        fort_cell_t *cell = create_cell();
        if (cell == NULL)
            goto clear;

/*        int status = fill_buffer_from_string(cell->str_buffer, "");  */
        int status = fill_cell_from_string(cell, "");
        if (IS_ERROR(status)) {
            destroy_cell(cell);
            goto clear;
        }

        status = vector_push(row->cells, &cell);
        if (IS_ERROR(status)) {
            destroy_cell(cell);
            goto clear;
        }
    }

    F_FREE(str_copy);
    return row;

clear:
    destroy_row(row);
    F_FREE(str_copy);
    return NULL;
}




fort_row_t* create_row_from_fmt_string(const char* FORT_RESTRICT fmt, va_list *va_args)
{
    string_buffer_t *buffer = create_string_buffer(DEFAULT_STR_BUF_SIZE, CharBuf);
    if (buffer == NULL)
        return NULL;

    int cols_origin = number_of_columns_in_format_string(fmt);

    while (1) {
        va_list va;
        va_copy(va, *va_args);
        int virtual_sz = vsnprintf(buffer->cstr, string_buffer_capacity(buffer)/*buffer->str_sz*/, fmt, va);
        va_end(va);
        /* If error encountered */
        if (virtual_sz == -1)
            goto clear;

        /* Successful write */
        if (virtual_sz < string_buffer_capacity(buffer))
            break;

        /* Otherwise buffer was too small, so incr. buffer size ant try again. */
        if (!IS_SUCCESS(realloc_string_buffer_without_copy(buffer)))
            goto clear;
    }

    int cols = number_of_columns_in_format_string(buffer->cstr);
    if (cols == cols_origin) {

        fort_row_t *row = create_row_from_string(buffer->cstr);
        if (row == NULL) {
            goto clear;
        }

        destroy_string_buffer(buffer);
        return row;
    }

    /* todo: add processing of cols != cols_origin */

clear:
    destroy_string_buffer(buffer);
    return NULL;
}


int snprintf_row(const fort_row_t *row, char *buffer, size_t buf_sz, size_t *col_width_arr, size_t col_width_arr_sz,
                        size_t row_height, const context_t *context)
{
    typedef char char_type;
    char space_char = ' ';
    char new_line_char = '\n';
    int (*snprint_n_chars_)(char *, size_t , size_t , char) = snprint_n_chars;
    int (*cell_printf_)(fort_cell_t *, size_t, size_t, char *, size_t, const context_t *) = cell_printf;


    assert(context);
    if (row == NULL)
        return -1;

    int cols_in_row = columns_in_row(row);
    if (cols_in_row > col_width_arr_sz)
        return -1;

    /*  Row separator anatomy
     *
     *  L    data    IV    data   IV   data    R
     */

    enum RowType row_type = get_cell_opt_value_hierarcial(context->table_options, context->row, FT_ANY_COLUMN, FT_OPT_ROW_TYPE);
    const char (*bord_chars)[BorderItemPosSize] = (row_type == Header)
            ? (&context->table_options->header_border_chars)
            : (&context->table_options->border_chars);
    const char *L = &(*bord_chars)[LL_bip];
    const char *IV = &(*bord_chars)[IV_bip];
    const char *R = &(*bord_chars)[RR_bip];


    int dev = 0;
    size_t i = 0;
    for (i = 0; i < row_height; ++i) {
        dev += snprint_n_chars_(buffer + dev, buf_sz - dev, 1, (char_type)*L);
        size_t j = 0;
        for (j = 0; j < col_width_arr_sz; ++j) {
            ((context_t *)context)->column = j;
            if (j < cols_in_row) {
                fort_cell_t *cell = *(fort_cell_t**)vector_at(row->cells, j);
                dev += cell_printf_(cell, i, j, buffer + dev, col_width_arr[j] + 1, context);
            } else {
                dev += snprint_n_chars_(buffer + dev, buf_sz - dev, col_width_arr[j], space_char);
            }
            if (j == col_width_arr_sz - 1) {
                dev += snprint_n_chars_(buffer + dev, buf_sz - dev, 1, (char_type)*R);
            } else {
                dev += snprint_n_chars_(buffer + dev, buf_sz - dev, 1, (char_type)*IV);
            }
        }
        dev += snprint_n_chars_(buffer + dev, buf_sz - dev, 1, new_line_char);
    }
    return dev;
}



int wsnprintf_row(const fort_row_t *row, wchar_t *buffer, size_t buf_sz, size_t *col_width_arr, size_t col_width_arr_sz,
                        size_t row_height, const context_t *context)
{
    typedef wchar_t char_type;
    char space_char = L' ';
    char new_line_char = L'\n';
    int (*snprint_n_chars_)(wchar_t *, size_t , size_t , wchar_t) = wsnprint_n_chars;
    int (*cell_printf_)(fort_cell_t *, size_t, size_t, wchar_t *, size_t, const context_t *) = cell_wprintf;


    assert(context);
    if (row == NULL)
        return -1;

    int cols_in_row = columns_in_row(row);
    if (cols_in_row > col_width_arr_sz)
        return -1;

    /*  Row separator anatomy
     *
     *  L    data    IV    data   IV   data    R
     */

    enum RowType row_type = get_cell_opt_value_hierarcial(context->table_options, context->row, FT_ANY_COLUMN, FT_OPT_ROW_TYPE);
    const char (*bord_chars)[BorderItemPosSize] = (row_type)
            ? (&context->table_options->header_border_chars)
            : (&context->table_options->border_chars);
    const char *L = &(*bord_chars)[LL_bip];
    const char *IV = &(*bord_chars)[IV_bip];
    const char *R = &(*bord_chars)[RR_bip];


    int dev = 0;
    size_t i = 0;
    for (i = 0; i < row_height; ++i) {
        dev += snprint_n_chars_(buffer + dev, buf_sz - dev, 1, (char_type)*L);
        size_t j = 0;
        for (j = 0; j < col_width_arr_sz; ++j) {
            ((context_t *)context)->column = j;
            if (j < cols_in_row) {
                fort_cell_t *cell = *(fort_cell_t**)vector_at(row->cells, j);
                dev += cell_printf_(cell, i, j, buffer + dev, col_width_arr[j] + 1, context);
            } else {
                dev += snprint_n_chars_(buffer + dev, buf_sz - dev, col_width_arr[j], space_char);
            }
            if (j == col_width_arr_sz - 1) {
                dev += snprint_n_chars_(buffer + dev, buf_sz - dev, 1, (char_type)*R);
            } else {
                dev += snprint_n_chars_(buffer + dev, buf_sz - dev, 1, (char_type)*IV);
            }
        }
        dev += snprint_n_chars_(buffer + dev, buf_sz - dev, 1, new_line_char);
    }
    return dev;
}

