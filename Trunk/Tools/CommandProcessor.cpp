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

void Get4IntParams(const char *arg, int &StartX, int &StartY, int &EndX, int &EndY)
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

void Get2IntParams(const char *arg, int &p1, int &p2)
{
    p1 = 0;
    p2 = 0;
    int Index = 0;
    p1 = atoi(&arg[Index]);
    Index += GetNextParamPos(&arg[Index]);
    p2 = atoi(&arg[Index]);
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
        printf("example : ImageSearchDLL.exe Verbose() Screenshot(0,0,10,10) Wait(1000) Screenshot(0,0,10,10) PrintCalcSAD() SaveScreenshot(1.bmp)\n");
        printf("Parameters are separated by space ' '. Float values use '.'\n");
        printf("Functions : \n");
        printf("\t- Verbose() - print some info what the program is doing\n");
        printf("\t- Screenshot(StartX,StartY,EndX,EndY) - take a Screenshot from region\n");
        printf("\t- namess(name) - you can reference this Screenshot based on this name\n");
        printf("\t- Wait(milliseconds) - Wait X milliseconds until executing next command\n");
        printf("\t- CalcSAD() - Calculate SAD between the last 2 taken Screenshots.\n");
        printf("\t- PrintCalcSAD() - Calculate SAD between the last 2 taken Screenshots. Print the value\n");
        printf("\t- SaveSAD(FileName) - Save last generated diff image ( between 2 Screenshots)as a BMP file\n");
        printf("\t- SaveScreenshot(FileName) - Save the last Screenshot in an uncompressed 24 bpp BMP\n");
        printf("\t- PrintTimer() - How many milliseconds passed since the start of the program\n");
        printf("\t- ResizeScreenshot(width,height) - Direct line duplicate/skip resize. No billiniar filtering\n");
        printf("\t- BlurScreenshot(KernelSize,GaussianHeight) - Kernel size is the radius of the pixels to be merged. Center pixel is multiplied by Gaussian factor\n");
        printf("\t- BlurCache(FileName,KernelSize,GaussianHeight) - Load picture to cache.Kernel size is the radius of the pixels to be merged. Center pixel is multiplied by Gaussian factor\n");
        printf("\t- LuminosityRemove() - Move all shades of a color to the smallest version of that color. Different results based on lightning strenght\n");
        printf("\t- GradientReduce(shades) - snap colors to specific levels. Does not care about illumination strength\n");
        printf("\t- ColorReduceCache(CachedFileName,ChannelMaxColors) - Reduce each color channel to have max specified colors\n");
        printf("\t- SaveCache(CachedFileName,FileName) - Save one of the cached images as an uncompressed 24 bpp BMP\n");
        printf("\t- LF_AddImage(FileName,LineLength) - Break up the image into lines and add it to a global filter\n");
        printf("\t- LF_AddImageEliminateNonCommon(FileName,LineLength) - Break up the image into lines and substract it from the global filter\n");
        printf("\t- LF_MarkObjectProbability(FileName) - Based on the linefilter status, mark on this image the recognized lines.\n");
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
        if (strcmp(args[i], "Verbose") == 0)
        {
            Verbose = 1;
            if (Verbose == 1)printf("Will try to process function : %s\n", args[i]);
            continue;
        }
        if (strcmp(args[i], "Screenshot") == 0)
        {
            int StartX, StartY, EndX, EndY;
            Get4IntParams(&args[i][FuncNameLen + 1], StartX, StartY, EndX, EndY);
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
        if (strcmp(args[i], "Wait") == 0)
        {
            int SleepMS;
            GetIntParam(&args[i][FuncNameLen + 1], SleepMS);
            if (Verbose == 1)printf("Will try to process function : %s with params %d\n", args[i], SleepMS);
            Sleep(SleepMS);
            continue;
        }
        if (strcmp(args[i], "CalcSAD") == 0)
        {
            if (Verbose == 1)printf("Will try to process function : %s\n", args[i]);
            GenerateDiffMap();
            continue;
        }
        if (strcmp(args[i], "PrintCalcSAD") == 0)
        {
            if (Verbose == 1)printf("Will try to process function : %s\n", args[i]);
            int SAD = GenerateDiffMap();
            printf("%d\n", SAD);
            continue;
        }
        if (strcmp(args[i], "SaveSAD") == 0)
        {
            char *NameStart = NULL;
            int NameLen = GetStringParam(&args[i][FuncNameLen + 1], NameStart);
            NameStart[NameLen] = 0;
            if (Verbose == 1)printf("Will try to process function : %s with params %s\n", args[i], NameStart);
            //SaveDiffMap();
            SaveImage(MotionDiff.Pixels, MotionDiff.GetWidth(), MotionDiff.GetHeight(), NameStart, 1);
            continue;
        }
        if (strcmp(args[i], "SaveScreenshot") == 0)
        {
            char *NameStart = NULL;
            int NameLen = GetStringParam(&args[i][FuncNameLen + 1], NameStart);
            NameStart[NameLen] = 0;
            if (Verbose == 1)printf("Will try to process function : %s with params %s\n", args[i], NameStart);
            SaveImage(CurScreenshot->Pixels, CurScreenshot->GetWidth(), CurScreenshot->GetHeight(), NameStart);
            continue;
        }
        if (strcmp(args[i], "PrintTimer") == 0)
        {
            if (Verbose == 1)printf("Will try to process function : %s\n", args[i]);
            unsigned int Now = GetTimeTickI();
            printf("%u\n", Now - Start);
            continue;
        }
        if (strcmp(args[i], "ResizeScreenshot") == 0)
        {
            int NewWidth, NewHeight;
            Get2IntParams(&args[i][FuncNameLen + 1], NewWidth, NewHeight);
            if (Verbose == 1)printf("Will try to process function : %s with params %d %d\n", args[i], NewWidth, NewHeight);
            ResizeScreenshot(NewWidth, NewHeight);
            continue;
        }
        if (strcmp(args[i], "BlurScreenshot") == 0)
        {
            int Size, Strength;
            Get2IntParams(&args[i][FuncNameLen + 1], Size, Strength);
            if (Verbose == 1)printf("Will try to process function : %s with params %d %d\n", args[i], Size, Strength);
            BlurrImage_(Size, Strength, CurScreenshot->Pixels, CurScreenshot->GetWidth(), CurScreenshot->GetHeight());
            continue;
        }
        if (strcmp(args[i], "BlurCache") == 0)
        {
            char *NameStart = NULL;
            int NameLen = GetStringParam(&args[i][FuncNameLen + 1], NameStart);
            NameStart[NameLen] = 0;
            int Size, Strength;
            Get2IntParams(&args[i][FuncNameLen + 1 + NameLen + 1], Size, Strength);
            if (Verbose == 1)printf("Will try to process function : %s with params %s %d %d\n", args[i], NameStart, Size, Strength);
            CachedPicture *cache = CachePicturePrintErrors(NameStart, __FUNCTION__);
            if (cache != NULL)
            {
                LPCOLORREF NewImage = BlurrImage_(Size, Strength, cache->Pixels, cache->Width, cache->Height);
                MY_FREE(cache->Pixels);
                cache->Pixels = NewImage;
            }
            continue;
        }
        if (strcmp(args[i], "LuminosityRemove") == 0)
        {
            if (Verbose == 1)printf("Will try to process function : %s\n", args[i]);
            LuminosityRemove();
            continue;
        }
        if (strcmp(args[i], "GradientReduce") == 0)
        {
            int GradientCount;
            GetIntParam(&args[i][FuncNameLen + 1], GradientCount);
            if (Verbose == 1)printf("Will try to process function : %s with params %d\n", args[i], GradientCount);
            GradientReduce(CurScreenshot->Pixels, CurScreenshot->GetWidth(), CurScreenshot->GetHeight(), GradientCount);
            continue;
        }
        if (strcmp(args[i], "ColorReduceCache") == 0)
        {
            char *NameStart = NULL;
            int NameLen = GetStringParam(&args[i][FuncNameLen + 1], NameStart);
            NameStart[NameLen] = 0;
            int ChannelMaxColorCount;
            GetIntParam(&args[i][FuncNameLen + 1 + NameLen + 1], ChannelMaxColorCount);
            if (Verbose == 1)printf("Will try to process function : %s with params %s %d\n", args[i], NameStart, ChannelMaxColorCount);
            ColorReduceCache(NameStart, ChannelMaxColorCount);
            continue;
        }
        if (strcmp(args[i], "SaveCache") == 0)
        {
            char *NameStart = NULL;
            int NameLen = GetStringParam(&args[i][FuncNameLen + 1], NameStart);
            NameStart[NameLen] = 0;
            char *NameStart2 = NULL;
            int NameLen2 = GetStringParam(&args[i][FuncNameLen + 1 + NameLen + 1], NameStart2);
            NameStart2[NameLen2] = 0;
            if (Verbose == 1)printf("Will try to process function : %s with params %s %s\n", args[i], NameStart, NameStart2);
            CachedPicture *cache = CachePicturePrintErrors(NameStart, __FUNCTION__);
            if (cache != NULL)
                SaveImage(cache->Pixels, cache->Width, cache->Height, NameStart2);
            continue;
        }
        if (strcmp(args[i], "LF_AddImage") == 0)
        {
            char *NameStart = NULL;
            int NameLen = GetStringParam(&args[i][FuncNameLen + 1], NameStart);
            NameStart[NameLen] = 0;
            int LineLength;
            GetIntParam(&args[i][FuncNameLen + 1 + NameLen + 1], LineLength);
            if (Verbose == 1)printf("Will try to process function : %s with params %s %d\n", args[i], NameStart, LineLength);
            LineFilter_AddImage(0, LineLength, NameStart);
            continue;
        }
        if (strcmp(args[i], "LF_AddImageEliminateNonCommon") == 0)
        {
            char *NameStart = NULL;
            int NameLen = GetStringParam(&args[i][FuncNameLen + 1], NameStart);
            NameStart[NameLen] = 0;
            int LineLength;
            GetIntParam(&args[i][FuncNameLen + 1 + NameLen + 1], LineLength);
            if (Verbose == 1)printf("Will try to process function : %s with params %s %d\n", args[i], NameStart, LineLength);
            LineFilter_AddImageEliminateNonCommon(0, LineLength, NameStart);
            continue;
        }
        if (strcmp(args[i], "LF_MarkObjectProbability") == 0)
        {
            char *NameStart = NULL;
            int NameLen = GetStringParam(&args[i][FuncNameLen + 1], NameStart);
            NameStart[NameLen] = 0;
            if (Verbose == 1)printf("Will try to process function : %s with params %s\n", args[i], NameStart);
            LineFilter_MarkObjectProbability(0, NameStart);
            continue;
        }

        if (Verbose == 1)printf("Could not process function : %s\n", args[i]);
    }
}