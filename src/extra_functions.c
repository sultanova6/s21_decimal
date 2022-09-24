#include "s21_decimal.h"

int get_bit(const s21_decimal decVar, int bit) {
    int res = 0;
    if (bit / 32 < 4) {
        unsigned int mask = 1u << (bit % 32);
        res = decVar.bits[bit / 32] & mask;
    }
    return !!res;
}

int offset_left(s21_decimal *varPtr, int value_offset) {
    int return_value = OK;
    int lastbit = get_highest_bit(*varPtr);
    if (lastbit + value_offset > 95) {
        return_value = INF;
    } else {
        for (int i = 0; i < value_offset; i++) {
            int value_31bit = get_bit(*varPtr, 31);
            int value_63bit = get_bit(*varPtr, 63);
            varPtr->bits[0] <<= 1;
            varPtr->bits[1] <<= 1;
            varPtr->bits[2] <<= 1;
            if (value_31bit) set_bit(varPtr, 32, 1);
            if (value_63bit) set_bit(varPtr, 64, 1);
        }
    }
    return return_value;
}

int get_sign(const s21_decimal *varPtr) {
    unsigned int mask = 1u << 31;
    return !!(varPtr->bits[3] & mask);
}

int get_scale(const s21_decimal *varPtr) {
    return (char)(varPtr->bits[3] >> 16);
}

void set_scale(s21_decimal *varPtr, int scale) {
    int clearMask = ~(0xFF << 16);
    varPtr->bits[3] &= clearMask;
    int mask = scale << 16;
    varPtr->bits[3] |= mask;
}

void mul_only_bits(s21_decimal value_1, s21_decimal value_2,
                   s21_decimal *result) {
    clean(result);
    s21_decimal tmp_res;
    int last_bit_1 = get_highest_bit(value_1);
    for (int i = 0; i <= last_bit_1; i++) {
        clean(&tmp_res);
        int value_bit_1 = get_bit(value_1, i);

        if (value_bit_1) {
            tmp_res = value_2;
            offset_left(&tmp_res, i);
            bit_addition(*result, tmp_res, result);
        }
    }
}

int bit_addition(s21_decimal var1, s21_decimal var2, s21_decimal *result) {
    clean(result);
    int return_value = OK;
    int buffer = 0;

    for (int i = 0; i < 96; i++) {
        int cur_bit_of_var1 = get_bit(var1, i);
        int cur_bit_of_var2 = get_bit(var2, i);

        if (!cur_bit_of_var1 && !cur_bit_of_var2) {
            if (buffer) {
                set_bit(result, i, 1);
                buffer = 0;
            } else {
                set_bit(result, i, 0);
            }
        } else if (cur_bit_of_var1 != cur_bit_of_var2) {
            if (buffer) {
                set_bit(result, i, 0);
                buffer = 1;
            } else {
                set_bit(result, i, 1);
            }
        } else {
            if (buffer) {
                set_bit(result, i, 1);
                buffer = 1;
            } else {
                set_bit(result, i, 0);
                buffer = 1;
            }
        }
        if (i == 95 && buffer == 1) return_value = INF;
    }

    return return_value;
}

void set_sign(s21_decimal *varPtr, int sign) {
    unsigned int mask = 1u << 31;
    if (sign != 0) {
        varPtr->bits[3] |= mask;
    } else {
        varPtr->bits[3] &= ~mask;
    }
}

void sub_only_bits(s21_decimal value_1, s21_decimal value_2,
                   s21_decimal *result) {
    clean(result);

    if (s21_is_equal(value_1, value_2) == TRUE) {
    } else {
        int value_1_last_bit = get_highest_bit(value_1);
        int buffer = 0;
        for (int i = 0; i <= value_1_last_bit; i++) {
            int current_bit_of_value_1 = get_bit(value_1, i);
            int current_bit_of_value_2 = get_bit(value_2, i);

            if (!current_bit_of_value_1 && !current_bit_of_value_2) {
                if (buffer) {
                    buffer = 1;
                    set_bit(result, i, 1);
                } else {
                    set_bit(result, i, 0);
                }
            } else if (current_bit_of_value_1 && !current_bit_of_value_2) {
                if (buffer) {
                    buffer = 0;
                    set_bit(result, i, 0);
                } else {
                    set_bit(result, i, 1);
                }
            } else if (!current_bit_of_value_1 && current_bit_of_value_2) {
                if (buffer) {
                    buffer = 1;
                    set_bit(result, i, 0);
                } else {
                    buffer = 1;
                    set_bit(result, i, 1);
                }
            } else if (current_bit_of_value_1 && current_bit_of_value_2) {
                if (buffer) {
                    buffer = 1;
                    set_bit(result, i, 1);
                } else {
                    set_bit(result, i, 0);
                }
            }
        }
    }
}

void div_only_bits(s21_decimal number_1, s21_decimal number_2, s21_decimal *buf,
                   s21_decimal *result) {
    clean(buf);
    clean(result);
    for (int i = get_highest_bit(number_1); i >= 0; i--) {
        if (get_bit(number_1, i)) set_bit(buf, 0, 1);
        if (s21_is_greater_or_equal(*buf, number_2) == TRUE) {
            sub_only_bits(*buf, number_2, buf);
            if (i != 0) offset_left(buf, 1);
            if (get_bit(number_1, i - 1)) set_bit(buf, 0, 1);
            offset_left(result, 1);
            set_bit(result, 0, 1);
        } else {
            offset_left(result, 1);
            if (i != 0) offset_left(buf, 1);
            if ((i - 1) >= 0 && get_bit(number_1, i - 1)) set_bit(buf, 0, 1);
        }
    }
}

void minus_scale(s21_decimal *a) {
    s21_decimal ten = {{10, 0, 0, 0}};
    if (get_highest_bit(*a) < 32 && a->bits[0] < 10) a->bits[0] = 0;
    s21_decimal musor;
    div_only_bits(*a, ten, &musor, a);
}

int sub_without_scale(s21_decimal value1, s21_decimal value2, s21_decimal *result) {
    clean(result);
    int ret = 0;
    int reminder = 0;
    if (!get_bit(value1, 127) && get_bit(value2, 127)) {
        s21_negate(value2, &value2);
        ret = add_without_scale(value1, value2, result);
    } else if (get_bit(value1, 127) && !get_bit(value2, 127)) {
        set_bit(&value2, 127, 1);
        ret = add_without_scale(value1, value2, result);
    } else {
        if (get_bit(value1, 127) && get_bit(value2, 127)) {
            s21_negate(value2, &value2);
            s21_negate(value1, &value1);
            swap_values(&value1, &value2);
        }
        if (!s21_is_greater_or_equal(value1, value2)) {
            set_bit(result, 127, 1);
            swap_values(&value1, &value2);
        }
        for (int i = 0; i < 96; i++) {
            if (get_bit(value1, i) - get_bit(value2, i) == 0) {
                if (reminder == 1) {
                    set_bit(result, i, 1);
                }
            } else if (get_bit(value1, i) - get_bit(value2, i) == 1) {
                if (reminder == 0)
                    set_bit(result, i, 1);
                else
                    reminder = 0;
            } else if (get_bit(value1, i) - get_bit(value2, i) == -1) {
                if (reminder != 1) {
                    set_bit(result, i, 1);
                    reminder = 1;
                } else {
                    reminder = 1;
                }
            }
            if (i == 95 && reminder) {
                if (get_bit(*result, 127))
                    ret = 2;
                else
                    ret = 1;
            }
        }
    }
    return ret;
}

void scale_equalize(s21_decimal *value_1, s21_decimal *value_2) {
    s21_decimal ten = {{10u, 0, 0, 0}};
    if (get_scale(value_1) < get_scale(value_2)) {
        int difference = get_scale(value_2) - get_scale(value_1);
        if (get_bit(*value_2, 93) == 0 && get_bit(*value_2, 94) == 0 &&
            get_bit(*value_2, 95) == 0) {
            for (int i = 0; i < difference; i++) {
                mul_only_bits(*value_1, ten, value_1);
            }
            set_scale(value_1, get_scale(value_2));
        } else {
            for (int i = 0; i < difference; i++) {
                minus_scale(value_2);
            }
            set_scale(value_2, get_scale(value_1));
        }
    } else {
        int difference = get_scale(value_1) - get_scale(value_2);
        if (get_bit(*value_1, 93) == 0 && get_bit(*value_1, 94) == 0 &&
            get_bit(*value_1, 95) == 0) {
            for (int i = 0; i < difference; i++) {
                mul_only_bits(*value_2, ten, value_2);
            }
            set_scale(value_2, get_scale(value_1));
        } else {
            for (int i = 0; i < difference; i++) {
                minus_scale(value_1);
            }
            set_scale(value_1, get_scale(value_2));
        }
    }
}

void bits_copy(s21_decimal src, s21_decimal *dest) {
    dest->bits[0] = src.bits[0];
    dest->bits[1] = src.bits[1];
    dest->bits[2] = src.bits[2];
}

int add_without_scale(s21_decimal value_1, s21_decimal value_2, s21_decimal *result) {
    int result_Function = 0;
    if (get_bit(value_1, 127) != get_bit(value_2, 127)) {
        s21_decimal value_1tmp = value_1;
        s21_decimal value_2tmp = value_2;
        if (get_bit(value_1, 127))
            s21_negate(value_1, &value_1tmp);
        else
            s21_negate(value_2, &value_2tmp);
        result_Function = sub_without_scale(value_1tmp, value_2tmp, result);
        if (get_bit(value_1, 127) && s21_is_greater(value_1, value_2))
            s21_negate(*result, result);
        else if (s21_is_greater(value_2, value_1))
            s21_negate(*result, result);
    } else {
        int check_minus;
        int in_mind = 0;
        s21_decimal temp = {{0, 0, 0, 0}};
        if (get_bit(value_1, 127) == 1) set_bit(&temp, 127, 1);
        for (int i = 0; i < 96; i++) {
            if (get_bit(value_1, i) && get_bit(value_2, i)) {
                if (in_mind) set_bit(&temp, i, 1);
                in_mind = 1;
            } else if (get_bit(value_1, i) || get_bit(value_2, i)) {
                if (!in_mind) set_bit(&temp, i, 1);
            } else if (in_mind) {
                set_bit(&temp, i, 1);
                in_mind = 0;
            }
            if (i == 95 && in_mind) {
                check_minus = sign_minus(temp);
                if (check_minus == 1)
                    result_Function = 2;
                else
                    result_Function = 1;
            }
        }
        *result = temp;
    }
    return result_Function;
}

void swap_values(s21_decimal *value1, s21_decimal *value2) {
    s21_decimal value2_tmp = *value2;
    *value2 = *value1;
    *value1 = value2_tmp;
}

void set_bit(s21_decimal *varPtr, int bit, int value) {
    unsigned int mask = 1u << (bit % 32);
    if (bit / 32 < 4 && value) {
        varPtr->bits[bit / 32] |= mask;
    } else if (bit / 32 < 4 && !value) {
        varPtr->bits[bit / 32] &= ~mask;
    }
}

void clean(s21_decimal *d) {
    if (d) {
        d->bits[0] = 0;
        d->bits[1] = 0;
        d->bits[2] = 0;
        d->bits[3] = 0;
    }
}

int get_highest_bit(s21_decimal dec) {
    int i = 95;
    while (i) {
        if (get_bit(dec, i))
            break;
        i--;
    }
    return i;
}

int shift_left(s21_decimal *dec, int shift) {
    int ret_val = 0;
    int highest_bit = get_highest_bit(*dec);
    if (highest_bit + shift > 95) {
        ret_val = 1;
    } else {
        for (int i = 0; i < shift; i++) {
            int last_low_bit_set = get_bit(*dec, 31);
            int last_mid_bit_set = get_bit(*dec, 63);
            dec->bits[0] = dec->bits[0] << 1;
            dec->bits[1] = dec->bits[1] << 1;
            dec->bits[2] = dec->bits[2] << 1;
            if (last_low_bit_set)
                set_bit(dec, 32, 1);
            if (last_mid_bit_set)
                set_bit(dec, 64, 1);
        }
    }
    if (get_bit(*dec, 127) && ret_val)
        ret_val = 2;
    return ret_val;
}

int sign_minus(s21_decimal a) { return a.bits[3] >> 31; }

int equalize_to_lower(s21_decimal *dec, int scale) {
    int ret = 0;
    while (scale--) {
        s21_decimal reminder;
        s21_decimal ten = {{10, 0, 0, 0}};
        *dec = binary_division(*dec, ten, &reminder, &ret);
        if (ret != 0)
            break;
    }
    return ret;
}

int get_float_sign(float *src) {
    return *(int *)src >> 31;
}

int equalize_to_bigger(s21_decimal *dec, int scale) {
    int ret = 0;
    while (scale--) {
        s21_decimal buf1 = *dec, buf2 = *dec;
        ret = shift_left(&buf2, 3);
        if (ret != 0)
            break;
        ret = shift_left(&buf1, 1);
        if (ret != 0)
            break;
        ret = s21_add(buf2, buf1, dec);
        if (ret != 0)
            break;
    }
    return ret;
}

s21_decimal binary_division(s21_decimal dec1, s21_decimal dec2, s21_decimal *remainder, int *fail) {
    clean(remainder);
    s21_decimal result = {{0, 0, 0, 0}};
    int result_scale = 0;
    *fail = processing_eq_scales(&dec1, &dec2, &result_scale);
    for (int i = get_highest_bit(dec1); i >= 0; i--) {
        if (get_bit(dec1, i))
            set_bit(remainder, 0, 1);
        if (s21_is_greater_or_equal(*remainder, dec2)) {
            *fail = sub_without_scale(*remainder, dec2, remainder);
            if (i != 0)
                *fail = shift_left(remainder, 1);
            if (get_bit(dec1, i - 1))
                set_bit(remainder, 0, 1);
            *fail = shift_left(&result, 1);
            set_bit(&result, 0, 1);
        } else {
            *fail = shift_left(&result, 1);
            if (i != 0)
                *fail = shift_left(remainder, 1);
            if (i - 1 >= 0 && get_bit(dec1, i - 1))
                set_bit(remainder, 0, 1);
        }
    }
    set_scale(remainder, result_scale);
    return result;
}

int equalize_scales(s21_decimal *dec1, s21_decimal *dec2, int scale) {
    int scale1 = get_scale(dec1);
    int scale2 = get_scale(dec2);
    int ret = 0;
    int sign1 = get_bit(*dec1, 127);
    dec1->bits[3] = 0;
    int sign2 = get_bit(*dec2, 127);
    dec2->bits[3] = 0;
    if (scale1 > scale)
        ret = equalize_to_lower(dec1, scale1 - scale);
    else if (scale1 < scale)
        ret = equalize_to_bigger(dec1, scale - scale1);
    if (scale2 > scale)
        ret = equalize_to_lower(dec2, scale2 - scale);
    else if (scale2 < scale)
        ret = equalize_to_bigger(dec2, scale - scale2);
    set_scale(dec1, scale);
    set_scale(dec2, scale);
    if (sign1)
        set_bit(dec1, 127, 1);
    if (sign2)
        set_bit(dec2, 127, 1);
    return ret;
}

int processing_eq_scales(s21_decimal *value_1, s21_decimal *value_2, int *final_scale) {
    s21_decimal value1_tmp = *value_1;
    s21_decimal value2_tmp = *value_2;
    int scale1 = get_scale(value_1);
    int scale2 = get_scale(value_2);
    int ret = 0, scale_to_write = scale1;
    if (scale1 > scale2) {
        ret = equalize_scales(&value1_tmp, &value2_tmp, scale1);
        scale_to_write = scale1;
        if (ret != 0) {
            scale_to_write = scale2;
            value1_tmp = *value_1;
            value2_tmp = *value_2;
            ret = equalize_scales(&value1_tmp, &value2_tmp, scale2);
        }
    } else if (scale1 < scale2) {
        ret = equalize_scales(&value1_tmp, &value2_tmp, scale2);
        scale_to_write = scale2;
        if (ret != 0) {
            scale_to_write = scale1;
            value1_tmp = *value_1;
            value2_tmp = *value_2;
            ret = equalize_scales(&value1_tmp, &value2_tmp, scale1);
        }
    }
    *final_scale = scale_to_write;
    *value_1 = value1_tmp;
    *value_2 = value2_tmp;
    return ret;
}

int float_binary_exp(float *src) {
    return ((*(int *)src & ~SIGN) >> 23) - 127;
}

