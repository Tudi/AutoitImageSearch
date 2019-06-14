#include "stdafx.h"

enum FunctionParamTypes
{
    FP_Int = 0,
    FP_Float,
    FP_String
};

int GetNextCharPos(const char *arg, char c, char c2 = 0)
{
    int index = 0;
    while (arg[index] != 0 && arg[index] != c && arg[index] != c2)
        index++;
    if (c2 != 0 && arg[index] == c2)
        return index;
    if (arg[index] == c)
        return index;
    return 0;
}

int GetFunctionNameLength(const char *arg)
{
    return GetNextCharPos(arg, '(');
}

int GetNextParamPos(const char *arg)
{
    return GetNextCharPos(arg, ',') + 1;
}

void GetScreenshotParams(const char *arg, int &StartX, int &StartY, int &EndX, int &EndY)
{
    StartX = 0;
    StartY = 0;
    EndX = 1;
    EndY = 1;
    int Index = 0;
    StartX = atoi(&arg[Index]);
    Index += GetNextParamPos(&arg[Index]);
    StartY = atoi(&arg[Index]);
    Index += GetNextParamPos(&arg[Index]);
    EndX = atoi(&arg[Index]);
    Index += GetNextParamPos(&arg[Index]);
    EndY = atoi(&arg[Index]);
    Index += GetNextParamPos(&arg[Index]);
}

void GetIntParam(const char *arg, int &Param)
{
    Param = 0;
    Param = atoi(arg);
}

int GetStringParam(char *arg, char *&Param)
{
    Param = arg;
    return GetNextCharPos(arg,',',')');
}

void ParseArgStrings(int argc, char **args)
{
    int Verbose = 0;
    if (argc == 0)
    {
        printf("usage : ImageSearchDLL.exe [function1] [functon2] ..... [functionX]\n");
        printf("example : ImageSearchDLL.exe verbose() screenshot(0,0,10,10) wait(1000) screenshot(0,0,10,10) printcalcsad() savescreenshot(1.bmp)\n");
        printf("Parameters are separated by space ' '. Float values use '.'\n");
        printf("Functions : \n");
        printf("\t- verbose() - print some info what the program is doing\n");
        printf("\t- screenshot(StartX,StartY,EndX,EndY) - take a screenshot from region\n");
        printf("\t- namess(name) - you can reference this screenshot based on this name\n");
        printf("\t- wait(milliseconds) - wait X milliseconds until executing next command\n");
        printf("\t- calcsad() - Calculate SAD between the last 2 taken screenshots.\n");
        printf("\t- printcalcsad() - Calculate SAD between the last 2 taken screenshots. Print the value\n");
        printf("\t- savesad() - Save last generated diff image as a PPM file\n");
        printf("\t- savescreenshot(FileName) - Save the last screenshot in an uncompressed 24 bpp BMP\n");
        printf("\t- printtimer() - How many milliseconds passed since the start of the program\n");
        return;
    }
    unsigned int Start = GetTimeTickI();
    for (int i = 1; i < argc; i++)
    {
        int FuncNameLen = GetFunctionNameLength(args[i]);
        if (FuncNameLen == 0)
        {
            if(Verbose==1)printf("No more recognizable functions found. Exiting\n");
            return;
        }
        args[i][FuncNameLen] = 0; //make it end here
        if (strcmp(args[i], "verbose") == 0)
        {
            Verbose = 1;
            if (Verbose == 1)printf("Will try to process function : %s\n", args[i]);
            continue;
        }
        if (strcmp(args[i], "screenshot") == 0)
        {
            int StartX, StartY, EndX, EndY;
            GetScreenshotParams(&args[i][FuncNameLen + 1], StartX, StartY, EndX, EndY);
            if (Verbose == 1)printf("Will try to process function : %s with params %d %d %d %d\n", args[i], StartX, StartY, EndX, EndY);
            TakeScreenshot(StartX, StartY, EndX, EndY);
            continue;
        }
        if (strcmp(args[i], "namess") == 0)
        {
            char *NameStart = NULL;
            int NameLen = GetStringParam(&args[i][FuncNameLen + 1], NameStart);
            NameStart[NameLen] = 0;
            if (Verbose == 1)printf("Will try to process function : %s with params %s\n", args[i], NameStart);
            MoveScreenshotToCache(NameStart);
            continue;
        }
        if (strcmp(args[i], "wait") == 0)
        {
            int SleepMS;
            GetIntParam(&args[i][FuncNameLen + 1], SleepMS);
            if (Verbose == 1)printf("Will try to process function : %s with params %d\n", args[i], SleepMS);
            Sleep(SleepMS);
            continue;
        }
        if (strcmp(args[i], "calcsad") == 0)
        {
            if (Verbose == 1)printf("Will try to process function : %s\n", args[i]);
            GenerateDiffMap();
            continue;
        }
        if (strcmp(args[i], "printcalcsad") == 0)
        {
            if (Verbose == 1)printf("Will try to process function : %s\n", args[i]);
            int SAD = GenerateDiffMap();
            printf("%d\n", SAD);
            continue;
        }
        if (strcmp(args[i], "savesad") == 0)
        {
            char *NameStart = NULL;
            int NameLen = GetStringParam(&args[i][FuncNameLen + 1], NameStart);
            NameStart[NameLen] = 0;
            if (Verbose == 1)printf("Will try to process function : %s with params %s\n", args[i], NameStart);
            //SaveDiffMap();
            SaveImage(MotionDiff.Pixels, MotionDiff.GetWidth(), MotionDiff.GetHeight(), NameStart, 1);
            continue;
        }
        if (strcmp(args[i], "savescreenshot") == 0)
        {
            char *NameStart = NULL;
            int NameLen = GetStringParam(&args[i][FuncNameLen + 1], NameStart);
            NameStart[NameLen] = 0;
            if (Verbose == 1)printf("Will try to process function : %s with params %s\n", args[i], NameStart);
            SaveImage(CurScreenshot->Pixels, CurScreenshot->GetWidth(), CurScreenshot->GetHeight(), NameStart);
            continue;
        }
        if (strcmp(args[i], "printtimer") == 0)
        {
            if (Verbose == 1)printf("Will try to process function : %s\n", args[i]);
            unsigned int Now = GetTimeTickI();
            printf("%u\n", Now - Start);
            continue;
        }
        if (Verbose == 1)printf("Could not process function : %s\n", args[i]);
    }
}