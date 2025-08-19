#pragma once

struct CachedPicture;
class ScreenshotStruct;
// !! hashes are very susceptible to edge changes
// !! it is very advised to do a blurring on the input images to avoid sharp edges
// !! due to blurry radius, it might be impossible to hash small images ( because at the edge there are no neighbours )

#define MAX_HASH_BITS_PER_CHANNEL 64
#define MAX_HASH_BYTES_PER_CHANNEL ((MAX_HASH_BITS_PER_CHANNEL+63)/8)
#define COLOR_CHANNEL_COUNT		3
#define BYTES_PER_PIXEL			4
#define PUSH_BIT(store, bit)	(*(uint64_t*)&store)=(((*(uint64_t*)&store)<<(uint64_t)1)|(uint64_t)bit)

struct ImgHash8x8
{
	uint64_t rHash, gHash, bHash;
};

struct ImgHash8x8_CompareResult
{
	uint64_t blocksAcumulated;
	uint64_t rgbBitsDiffer;
	uint64_t PctDifferAvg;
};

struct ImgHash8x8_All
{
	ImgHash8x8 AHash;
	ImgHash8x8 BHash;
	ImgHash8x8 CHash;
};

struct ImgHashWholeIage
{
	size_t cols, rows;
	ImgHash8x8_All* hashes;
	size_t UniqueFameCounter;
};

// a simple cache is probably not enough. Perform multiple steps so that it becomes easy
void GenHashesForCachedImage(CachedPicture *pic, ImgHashWholeIage* out_hashes);
int GenHashesOnScreenshotForCachedImage(CachedPicture* pic, ScreenshotStruct* ss, int atX, int atY, ImgHashWholeIage* out_hashes);
void FreeHashAllocatedData(ImgHashWholeIage* out_hashes);
void ReinitScreenshotHashCache(ScreenshotStruct* ss);

// get the average luminosity of the image, compare each pixel of the image to the avg. If lower than avg produce a 0, else 1 bit
// pro : mostly ignores luminozity related image changes + similar to SAD but can be cached
// cons : pixel locations do not matter to produce the same hash
void genAHash_8x8(const LPCOLORREF pixels, const size_t stride, ImgHash8x8* out_hash);
// based on a left to right gradient. If smaller than left than 0, else 1
// pro : mostly ignores luminozity changes + semi retains column pixel locations
// cons : only retains column related locations + single extra column added might produce a 100% different hash
void genBHash_8x8(const LPCOLORREF pixels, const size_t stride, ImgHash8x8* out_hash);
// based on a up to down gradient. If smaller than upper than 0, else 1
// pro : mostly ignores luminozity changes + semi retains row pixel locations
// cons : only retains row related locations + single extra row added might produce a 100% different hash
void genCHash_8x8(const LPCOLORREF pixels, const size_t stride, ImgHash8x8* out_hash);
void compareHash_8x8(ImgHash8x8* h1, ImgHash8x8* h2, ImgHash8x8_CompareResult* out, bool bIsAcumulate);
int compareHash(ImgHashWholeIage* hash1, ImgHashWholeIage* hash2, ImgHash8x8_CompareResult* out);