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

#define HashPixelWidth 8
#define HashPixelHeight 8

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
	uint8_t HasHashesGenerated;
	ImgHash8x8 AHash;
	ImgHash8x8 BHash;
	ImgHash8x8 CHash;
};

class ImgHashWholeImage
{
public:
	~ImgHashWholeImage();
	void initHashes() {
		if (hashes == NULL) {
			hashes = (ImgHash8x8_All*)MY_ALLOC(rows * cols * sizeof(ImgHash8x8_All));
		}
		if (hashes) {
			memset(hashes, 0, cols * rows * sizeof(ImgHash8x8_All));
		}
	}
	void reinit() {
		cols = rows = 0;
	}
	virtual inline ImgHash8x8_All* GetHashStore(const size_t x, const size_t y) const { return NULL; }
	size_t cols, rows; // because ss store 1 hash / pixel, but caches store 1 hash / 8 pixel
	size_t imgWidth, imgHeight; // the hash was generated based on these sizes
	ImgHash8x8_All* hashes; // screenshots will have 1 hash8x8 for every pixel. cached images will have 1 hash for every 8x8 pixel
	size_t UniqueFameCounter; // this is the link to the owner frame. If the attached screenshot changes, hash cache needs to be refreshed
};

class ImgHashCache : public ImgHashWholeImage {
public:
	// coordinates are original pixel based
	inline ImgHash8x8_All* GetHashStore(const size_t x, const size_t y) const {
#ifdef _DEBUG
		assert(hashes != NULL);
		assert(x < imgWidth);
		assert(y < imgHeight);
#endif
		return &hashes[(y / HashPixelHeight) * cols + x / HashPixelWidth];
	}
};

class ImgHashSS : public ImgHashWholeImage {
public:
	void reinit() {
		cols = rows = 0;
	}
	// coordinates are original pixel based
	inline ImgHash8x8_All* GetHashStore(const size_t x, const size_t y) const {
#ifdef _DEBUG
		assert(hashes != NULL);
		assert(x < imgWidth);
		assert(y < imgHeight);
#endif
		return &hashes[y * cols + x];
	}
};

// a simple cache is probably not enough. Perform multiple steps so that it becomes easy
void GenHashesForCachedImage(CachedPicture *pic, ImgHashCache* out_hashes);
template <const bool bGenAllPositions>
int64_t GenHashesOnScreenshotForCachedImage(CachedPicture* pic, ScreenshotStruct* ss, size_t atX, size_t atY);
void FreeHashAllocatedData(ImgHashWholeImage* out_hashes);
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
int64_t compareHash(const ImgHashSS* hashSS, const ImgHashCache* hashC, size_t atX, size_t atY, ImgHash8x8_CompareResult* out);