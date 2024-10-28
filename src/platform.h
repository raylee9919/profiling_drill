#ifdef _MSC_VER
  #include <windows.h>
  #include <intrin.h>


static u64
get_os_counter_frequency(void)
{
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    u64 result = freq.QuadPart;
    return result;
}

static u64
read_os_timer(void)
{
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    u64 result = counter.QuadPart;
    return result;
}

static u64
read_cpu_timer(void)
{
    u64 result = __rdtsc();
    return result;
}



#endif
