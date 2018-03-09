#include "options.h"
#include "fort_impl.h"
#include "assert.h"
#include "vector.h"

/*****************************************************************************
 *               COLUMN OPTIONS
 * ***************************************************************************/

struct fort_cell_options g_default_cell_option =
{
    FT_ANY_ROW,    /* cell_row */
    FT_ANY_COLUMN, /* cell_col */

    /* options */
    FT_OPT_MIN_WIDTH  | FT_OPT_TEXT_ALIGN | FT_OPT_TOP_PADDING | FT_OPT_BOTTOM_PADDING
    | FT_OPT_LEFT_PADDING | FT_OPT_RIGHT_PADDING | FT_OPT_EMPTY_STR_HEIGHT ,

    0,             /* col_min_width */
    RightAligned,  /* align */
    0,      /* cell_padding_top         */
    0,      /* cell_padding_bottom      */
    1,      /* cell_padding_left        */
    1,      /* cell_padding_right       */
    1,      /* cell_empty_string_height */

    Common, /* row_type */
};

static int get_option_value_if_exists_otherwise_default(const struct fort_cell_options *cell_opts, uint32_t option)
{
    if (cell_opts == NULL || !OPTION_IS_SET(cell_opts->options, option)) {
        cell_opts = &g_default_cell_option;
    }

    switch (option) {
        case FT_OPT_MIN_WIDTH:
            return cell_opts->col_min_width;
        case FT_OPT_TEXT_ALIGN:
            return cell_opts->align;
        case FT_OPT_TOP_PADDING:
            return cell_opts->cell_padding_top;
        case FT_OPT_BOTTOM_PADDING:
            return cell_opts->cell_padding_bottom;
        case FT_OPT_LEFT_PADDING:
            return cell_opts->cell_padding_left;
        case FT_OPT_RIGHT_PADDING:
            return cell_opts->cell_padding_right;
        case FT_OPT_EMPTY_STR_HEIGHT:
            return cell_opts->cell_empty_string_height;
        case FT_OPT_ROW_TYPE:
            return cell_opts->row_type;
        default:
            /* todo: implement later */
            exit(333);
    }
}


fort_column_options_t g_column_options =
{
    0,           /* col_min_width*/
    RightAligned, /* align */
};

fort_column_options_t create_column_options()
{
    fort_column_options_t result;
    memset(&result, '\0', sizeof(result));
    memcpy(&result, &g_column_options, sizeof(result));
    return result;
}


#define DEFAULT_CELL_OPTION  {FT_ROW_UNSPEC, FT_COLUMN_UNSPEC, 0, 0, 0}

fort_cell_opt_container_t *create_cell_opt_container()
{
    fort_cell_opt_container_t *ret = create_vector(sizeof(fort_cell_options_t), DEFAULT_VECTOR_CAPACITY);
    return ret;
}


void destroy_cell_opt_container(fort_cell_opt_container_t *cont)
{
    if (cont)
        destroy_vector(cont);
}

const fort_cell_options_t* cget_cell_opt(const fort_cell_opt_container_t *cont, unsigned row, unsigned col)
{
    assert(cont);
    size_t sz = vector_size(cont);
    size_t i = 0;
    for (i = 0; i < sz; ++i) {
        const fort_cell_options_t* opt = (const fort_cell_options_t*)vector_at_c(cont, i);
        if (opt->cell_row == row && opt->cell_col == col)
            return opt;
    }
    return NULL;
}

fort_cell_options_t* get_cell_opt_and_create_if_not_exists(fort_cell_opt_container_t *cont, unsigned row, unsigned col)
{
    assert(cont);
    size_t sz = vector_size(cont);
    size_t i = 0;
    for (i = 0; i < sz; ++i) {
        fort_cell_options_t* opt = (fort_cell_options_t*)vector_at(cont, i);
        if (opt->cell_row == row && opt->cell_col == col)
            return opt;
    }
    fort_cell_options_t opt = DEFAULT_CELL_OPTION;
    opt.cell_row = row;
    opt.cell_col = col;
    if (IS_SUCCESS(vector_push(cont, &opt))) {
        return (fort_cell_options_t*)vector_at(cont, sz);
    }

    return NULL;
}




int get_cell_opt_value_hierarcial(const fort_table_options_t *options, size_t row, size_t column, uint32_t option)
{
    assert(options);

    const fort_cell_options_t* opt = NULL;
    if (options->cell_options != NULL) {
        while (1) {
            opt = cget_cell_opt(options->cell_options, row, column);
            if (opt != NULL)
                break;
            if (row != FT_ANY_ROW) {
                row = FT_ANY_ROW;
                continue;
            }
            if (column != FT_ANY_COLUMN) {
                column = FT_ANY_COLUMN;
                continue;
            }

            opt = NULL;
            break;
        }
    }

    return get_option_value_if_exists_otherwise_default(opt, option);
}


static fort_status_t set_cell_option_impl(fort_cell_options_t *opt, uint32_t option, int value)
{
    assert(opt);

    OPTION_SET(opt->options, option);
    if (OPTION_IS_SET(option, FT_OPT_MIN_WIDTH)) {
        opt->col_min_width = value;
    } else if (OPTION_IS_SET(option, FT_OPT_TEXT_ALIGN)) {
        opt->align = value;
    } else if (OPTION_IS_SET(option, FT_OPT_TOP_PADDING)) {
        opt->cell_padding_top = value;
    } else if (OPTION_IS_SET(option, FT_OPT_BOTTOM_PADDING)) {
        opt->cell_padding_bottom = value;
    } else if (OPTION_IS_SET(option, FT_OPT_LEFT_PADDING)) {
        opt->cell_padding_left = value;
    } else if (OPTION_IS_SET(option, FT_OPT_RIGHT_PADDING)) {
        opt->cell_padding_right = value;
    } else if (OPTION_IS_SET(option, FT_OPT_EMPTY_STR_HEIGHT)) {
        opt->cell_empty_string_height = value;
    } else if (OPTION_IS_SET(option, FT_OPT_ROW_TYPE)) {
        opt->row_type = value;
    }

    return F_SUCCESS;
}


fort_status_t set_cell_option(fort_cell_opt_container_t *cont, unsigned row, unsigned col, uint32_t option, int value)
{
    fort_cell_options_t* opt = get_cell_opt_and_create_if_not_exists(cont, row, col);
    if (opt == NULL)
        return F_ERROR;

    return set_cell_option_impl(opt, option, value);
    /*
    OPTION_SET(opt->options, option);
    if (OPTION_IS_SET(option, FT_OPT_MIN_WIDTH)) {
        opt->col_min_width = value;
    } else if (OPTION_IS_SET(option, FT_OPT_TEXT_ALIGN)) {
        opt->align = value;
    }

    return F_SUCCESS;
    */
}

fort_status_t set_default_cell_option(uint32_t option, int value)
{
    return set_cell_option_impl(&g_default_cell_option, option, value);
}

/*****************************************************************************
 *               OPTIONS
 * ***************************************************************************/





fort_table_options_t g_table_options = {
    /* border_chars */
    {
     '+', '-', '+', '+',
     '|', '|', '|',
     '\0', '\0', '\0', '\0',
     '+', '-', '+', '+'
    },

    /* header_border_chars */
    {
    '+', '-', '+', '+',
    '|', '|', '|',
    '+', '-', '+', '+',
    '+', '-', '+', '+'
    },

    /* separator_chars */
    {
    '+', '=', '+', '+',
    },

    NULL,     /* cell_options */
};


fort_table_options_t* create_table_options()
{
    fort_table_options_t* options = F_CALLOC(sizeof(fort_table_options_t), 1);
    if (options == NULL) {
        return NULL;
    }
    memcpy(options, &g_table_options, sizeof(fort_table_options_t));
    options->cell_options = create_cell_opt_container();
    if (options->cell_options == NULL) {
        destroy_table_options(options);
        options = NULL;
    }

    return options;
}

fort_table_options_t* copy_table_options(const fort_table_options_t *option)
{
    /* todo: normal implementation, do deep copy of col options */

    fort_table_options_t* new_opt = create_table_options();
    if (new_opt == NULL)
        return NULL;

    memcpy(new_opt, option, sizeof(fort_table_options_t));

    if (option->cell_options) {
        destroy_cell_opt_container(new_opt->cell_options);
        new_opt->cell_options = copy_vector(option->cell_options);
        if (new_opt->cell_options == NULL) {
            destroy_table_options(new_opt);
            new_opt = NULL;
        }
    }
    return new_opt;
}


void destroy_table_options(fort_table_options_t* options)
{
    if (options == NULL)
        return;

    if (options->cell_options != NULL) {
        destroy_cell_opt_container(options->cell_options);
    }
    F_FREE(options);
}



