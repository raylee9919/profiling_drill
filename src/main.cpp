/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Sung Woo Lee $
   $Notice: (C) Copyright 2024 by Sung Woo Lee. All Rights Reserved. $
   ======================================================================== */

#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <string.h>

#define array_count(ARR) (sizeof(ARR) / sizeof(ARR[0]))

#include "types.h"
#include "platform.h"
#include "debug.cpp"
#include "haversine_generator.cpp"


struct Read_Entire_File_Result
{
    char    *contents;
    size_t  size;
};
static Read_Entire_File_Result
read_entire_file_and_null_terminate(char *filename)
{
    TIMED_FUNCTION;

    Read_Entire_File_Result result = {};
    char *contents = 0;
    size_t file_size = 0;

    FILE *file;
    file = fopen(filename, "rb");
    if (file)
    {
        fseek(file, 0, SEEK_END);
        file_size = ftell(file);
        fseek(file, 0, SEEK_SET);

        contents = (char *)malloc(file_size + 1);
        fread(contents, file_size, 1, file);
        contents[file_size] = 0;

        fclose(file);
    }
    else
    {
        fprintf(stderr, "ERROR: Couldn't open file %s\n", filename);
    }

    result.contents = contents;
    result.size = file_size;

    return result;
}

struct Sample
{
    f64 x0;
    f64 y0;
    f64 x1;
    f64 y1;
};

struct Parse_Result
{
    u64     size;
    Sample  *samples;
};

static f64
string_to_f64(char *str, size_t size)
{
    TIMED_FUNCTION;

    f64 result = 0;

    f64 left = 0;
    f64 right = 0;
    s32 exp = 1;

    if (str)
    {
        s32 is_left = 1;
        for (char *at = str;
             at < str + size;
             ++at)
        {
            if (is_left)
            {
                if (*at == '.')
                {
                    is_left = 0;
                }
                else
                {
                    f64 val = (f64)(*at - '0');
                    left = left*10 + val;
                }
            }
            else
            {
                f64 val = (f64)(*at - '0');
                right = right + (val / pow(10, exp++));
            }
        }
    }

    result = (left + right);
    return result;
}

static f64
march_and_get_value(char **at)
{
    TIMED_FUNCTION;

    *at += 4;
    s32 is_positive;
    if (**at == '-')
    {
        is_positive = 0;
        *at += 1;
    }
    else
    {
        is_positive = 1;
    }
    char *end = *at;
    while (*end != ',' &&
           *end != '}')
    {
        ++end;
    }
    size_t size = (end - *at);

    f64 val = (is_positive ? 1 : -1) * string_to_f64(*at, size);
    return val;
}

// @NOTE: This ain't general-purpose parser.
static Parse_Result
parse_haversine_json(char *contents, size_t file_size, u64 DEBUG_sample_size)
{
    TIMED_FUNCTION;

    Parse_Result result = {};
    s32 size = 0;
    result.samples = (Sample *)malloc(DEBUG_sample_size * sizeof(Sample));

    if (contents &&
        file_size)
    {
        s32 is_in_array = 0;

        for (char *at = contents;
             at < (contents + file_size);
             ++at)
        {
            if (is_in_array)
            {
                Sample *sample = (result.samples + size);
                switch (*at)
                {
                    case 'x':
                    {
                        char next_char = *(at + 1);
                        if (next_char == '0')
                        {
                            sample->x0 = march_and_get_value(&at);
                        }
                        else if (next_char == '1')
                        {
                            sample->x1 = march_and_get_value(&at);
                        }
                    } break;
                    
                    case 'y':
                    {
                        char next_char = *(at + 1);
                        if (next_char == '0')
                        {
                            sample->y0 = march_and_get_value(&at);
                        }
                        else if (next_char == '1')
                        {
                            sample->y1 = march_and_get_value(&at);
                            ++size;
                        }
                    } break;

                    case ']':
                    {
                        is_in_array = 0;
                    } break;

                    default:
                    {
                    } break;
                }
            }
            else
            {
                if (*at == '[')
                    is_in_array = 1;
            }
        }
    }
    else
    {
        fprintf(stderr, "Invalid arguments.\n");
    }

    result.size = size;
    return result;
}

static void
accumulate_sum(f64 *sum, Sample *sample, f64 coefficient)
{
    TIMED_FUNCTION;

    f64 EarthRadius = 6372.8;
    f64 HaversineDistance = reference_haversine(sample->x0, sample->y0, sample->x1, sample->y1, EarthRadius);
    *sum += (coefficient * HaversineDistance);
}

int main(int argc, char **args)
{
    u64 os_freq = get_os_counter_frequency();
    printf("OS Frequency: %llu\n", os_freq);

    u64 cpu_start = read_cpu_timer();
    u64 os_start = read_os_timer();
    u64 os_end = 0;
    u64 os_elapsed = 0;

    while (os_elapsed < os_freq)
    {
        os_end = read_os_timer();
        os_elapsed = os_end - os_start;
    }
    
    u64 cpu_end = read_cpu_timer();
    u64 cpu_elapsed = (cpu_end - cpu_start);
    u64 cpu_freq = 0;
    if (os_elapsed)
    {
        cpu_freq = cpu_elapsed * os_freq / os_elapsed;
    }

    printf("OS Timer: %llu -> %llu = %llu elapsed\n", os_start, os_end, os_elapsed);
    printf("OS Seconds: %.4f\n", (f64)os_elapsed/(f64)os_freq);
    printf("CPU Timer: %llu -> %llu = %llu elapsed\n", cpu_start, cpu_end, cpu_end - cpu_start);
    printf("CPU Frequency: %llu (guessed)\n", cpu_freq);

    generate_haversine_json(argc, &args);

    char json_filename[256];
    u64 pair_count = atoll(args[3]);
    generate_json_filename(json_filename, pair_count, "flex", "json");

    Read_Entire_File_Result read_result = read_entire_file_and_null_terminate(json_filename);
    char *contents = read_result.contents;
    size_t size = read_result.size;

    if (contents && size)
    {
        Parse_Result parse_result = parse_haversine_json(contents, size, pair_count);
        u64 sample_size = parse_result.size;
        f64 coefficient = (1.0 / (f64)sample_size);
        f64 sum = 0;
        for (u32 idx = 0;
             idx < sample_size;
             ++idx)
        {
            Sample *sample = parse_result.samples + idx;
            accumulate_sum(&sum, sample, coefficient);
        }

        fprintf(stdout, "\nPair count: %llu\n", sample_size);
        fprintf(stdout, "Expected sum: %.16f\n", sum);

        printf("======================================\n");
        for (s32 idx = 0;
             idx < array_count(table);
             ++idx)
        {
            const char *function_name = table[idx].function_name;
            if (function_name)
            {
                Timed_Function_Entry *entry = table + idx;
                u64 cpu_timer_elapsed = entry->cpu_timer_elapsed;
                u32 count = entry->count;

                printf("%s[%u]: Total %llu cycles, Average %.2f cycles\n", function_name, count, cpu_timer_elapsed, (f64)cpu_timer_elapsed/(f64)count);
            }
        }
    }
    else
    {
        fprintf(stderr, "ERROR: Couldn't read contents from %s\n", json_filename);
    }

    return 0;
}
    
