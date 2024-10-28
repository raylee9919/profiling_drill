#define TIMED_FUNCTION Timed_Function timed_function(__FUNCTION__);

struct Timed_Function_Entry
{
    const char *function_name;

    u64 cpu_timer_start;
    u64 cpu_timer_end;
    u64 cpu_timer_elapsed;

    u32 count;
};

#define DEBUG_TIMED_FUNCTION_ENTRY_COUNT 256
static Timed_Function_Entry table[DEBUG_TIMED_FUNCTION_ENTRY_COUNT];

static s32
function_name_to_idx(const char *name)
{
    s64 x = (s64)name;
    u32 table_size = array_count(table);
    u32 idx = ((u32)(x >> 32) + (u32)x) % table_size;
    for (;;)
    {
        if (table[idx].function_name == 0)
        {
            table[idx].function_name = name;
            break;
        }
        else if (table[idx].function_name == name)
        {
            break;
        }
        else
        {
            ++idx;
            idx %= table_size;
        }
    }
    return idx;
}

struct Timed_Function
{
    Timed_Function(const char *function)
    {
        _function_name = function;
        _idx = function_name_to_idx(function);

        Timed_Function_Entry *entry = table + _idx;
        entry->function_name = function;
        entry->cpu_timer_start = read_cpu_timer();
    }

    ~Timed_Function()
    {
        Timed_Function_Entry *entry = table + _idx;
        entry->cpu_timer_end = read_cpu_timer();
        entry->cpu_timer_elapsed += (table[_idx].cpu_timer_end - table[_idx].cpu_timer_start);
        ++entry->count;
    }

    const char *_function_name;
    u32 _idx;
};
