static f64
square(f64 A)
{
    f64 Result = (A*A);
    return Result;
}

static f64
radians_from_degrees(f64 Degrees)
{
    f64 Result = 0.01745329251994329577 * Degrees;
    return Result;
}

static f64
reference_haversine(f64 X0, f64 Y0, f64 X1, f64 Y1, f64 EarthRadius)
{
    TIMED_FUNCTION;

    f64 lat1 = Y0;
    f64 lat2 = Y1;
    f64 lon1 = X0;
    f64 lon2 = X1;
    
    f64 dLat = radians_from_degrees(lat2 - lat1);
    f64 dLon = radians_from_degrees(lon2 - lon1);
    lat1 = radians_from_degrees(lat1);
    lat2 = radians_from_degrees(lat2);
    
    f64 a = square(sin(dLat/2.0)) + cos(lat1)*cos(lat2)*square(sin(dLon/2));
    f64 c = 2.0*asin(sqrt(a));
    
    f64 Result = EarthRadius * c;
    
    return Result;
}

struct Random_Series
{
    u64 A, B, C, D;
};

static u64
rotate_left(u64 V, int Shift)
{
    u64 Result = ((V << Shift) | (V >> (64-Shift)));
    return Result;
}

static u64
random_u64(Random_Series *Series)
{
    u64 A = Series->A;
    u64 B = Series->B;
    u64 C = Series->C;
    u64 D = Series->D;
    
    u64 E = A - rotate_left(B, 27);
    
    A = (B ^ rotate_left(C, 17));
    B = (C + D);
    C = (D + E);
    D = (E + A);
    
    Series->A = A;
    Series->B = B;
    Series->C = C;
    Series->D = D;
    
    return D;
}

static Random_Series
seed(u64 Value)
{
    Random_Series Series = {};
    
    // NOTE(casey): This is the seed pattern for JSF generators, as per the original post
    Series.A = 0xf1ea5eed;
    Series.B = Value;
    Series.C = Value;
    Series.D = Value;
    
    u32 Count = 20;
    while (Count--)
    {
        random_u64(&Series);
    }
    
    return Series;
}

static f64
random_in_range(Random_Series *Series, f64 Min, f64 Max)
{
    f64 t = (f64)random_u64(Series) / (f64)U64_MAX;
    f64 Result = (1.0 - t)*Min + t*Max;
    
    return Result;
}

inline void
generate_json_filename(char *buf, long long unsigned pair_count, const char *label, const char *extension)
{
    sprintf(buf, "data_%llu_%s.%s", pair_count, label, extension);
}

static FILE *
open(long long unsigned PairCount, char const *Label, char const *Extension)
{
    char Temp[256];
    generate_json_filename(Temp, PairCount, Label, Extension);
    FILE *Result = fopen(Temp, "wb");
    if(!Result)
    {
        fprintf(stderr, "Unable to open \"%s\" for writing.\n", Temp);
    }
    
    return Result;
}

static f64
random_degree(Random_Series *Series, f64 Center, f64 Radius, f64 MaxAllowed)
{
    f64 MinVal = Center - Radius;
    if(MinVal < -MaxAllowed)
    {
        MinVal = -MaxAllowed;
    }
    
    f64 MaxVal = Center + Radius;
    if(MaxVal > MaxAllowed)
    {
        MaxVal = MaxAllowed;
    }
    
    f64 Result = random_in_range(Series, MinVal, MaxVal);
    return Result;
}

static void
generate_haversine_json(int arg_count, char ***args)
{
    TIMED_FUNCTION;

    if (arg_count == 4)
    {
        u64 ClusterCountLeft = U64_MAX;
        f64 MaxAllowedX = 180;
        f64 MaxAllowedY = 90;
        
        f64 XCenter = 0;
        f64 YCenter = 0;
        f64 XRadius = MaxAllowedX;
        f64 YRadius = MaxAllowedY;
        
        char const *MethodName = (*args)[1];
        if(strcmp(MethodName, "cluster") == 0)
        {
            ClusterCountLeft = 0;
        }
        else if(strcmp(MethodName, "uniform") != 0)
        {
            MethodName = "uniform";
            fprintf(stderr, "WARNING: Unrecognized method name. Using 'uniform'.\n");
        }
        
        u64 seedValue = atoll((*args)[2]);
        Random_Series Series = seed(seedValue);
        
        u64 MaxPairCount = (1ULL << 34);
        u64 PairCount = atoll((*args)[3]);
        if(PairCount < MaxPairCount)
        {
            u64 ClusterCountMax = 1 + (PairCount / 64);
            
            FILE *FlexJSON = open(PairCount, "flex", "json");
            FILE *HaverAnswers = open(PairCount, "haveranswer", "f64");
            if(FlexJSON && HaverAnswers)
            {
                fprintf(FlexJSON, "{\"pairs\":[\n");
                f64 Sum = 0;
                f64 SumCoef = 1.0 / (f64)PairCount;
                for(u64 PairIndex = 0; PairIndex < PairCount; ++PairIndex)
                {
                    if(ClusterCountLeft-- == 0)
                    {
                        ClusterCountLeft = ClusterCountMax;
                        XCenter = random_in_range(&Series, -MaxAllowedX, MaxAllowedX);
                        YCenter = random_in_range(&Series, -MaxAllowedY, MaxAllowedY);
                        XRadius = random_in_range(&Series, 0, MaxAllowedX);
                        YRadius = random_in_range(&Series, 0, MaxAllowedY);
                    }
                    
                    f64 X0 = random_degree(&Series, XCenter, XRadius, MaxAllowedX);
                    f64 Y0 = random_degree(&Series, YCenter, YRadius, MaxAllowedY);
                    f64 X1 = random_degree(&Series, XCenter, XRadius, MaxAllowedX);
                    f64 Y1 = random_degree(&Series, YCenter, YRadius, MaxAllowedY);
                    
                    f64 EarthRadius = 6372.8;
                    f64 HaversineDistance = reference_haversine(X0, Y0, X1, Y1, EarthRadius);
                    
                    Sum += SumCoef*HaversineDistance;
                    
                    char const *JSONSep = (PairIndex == (PairCount - 1)) ? "\n" : ",\n";
                    fprintf(FlexJSON, "    {\"x0\":%.16f, \"y0\":%.16f, \"x1\":%.16f, \"y1\":%.16f}%s", X0, Y0, X1, Y1, JSONSep);
                    
                    fwrite(&HaversineDistance, sizeof(HaversineDistance), 1, HaverAnswers);
                }
                fprintf(FlexJSON, "]}\n");
                fwrite(&Sum, sizeof(Sum), 1, HaverAnswers);
        
                fprintf(stdout, "Method: %s\n", MethodName);
                fprintf(stdout, "Random seed: %llu\n", seedValue);
                fprintf(stdout, "Pair count: %llu\n", PairCount);
                fprintf(stdout, "Expected sum: %.16f\n", Sum);
            }
            
            if(FlexJSON) fclose(FlexJSON);
            if(HaverAnswers) fclose(HaverAnswers);
        }
        else
        {
            fprintf(stderr, "To avoid accidentally generating massive files, number of pairs must be less than %llu.\n", MaxPairCount);
        }
    }
    else
    {
        fprintf(stderr, "Usage: %s [uniform/cluster] [random seed] [number of coordinate pairs to generate]\n", (*args)[0]);
    }
}
