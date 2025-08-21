#include "StdAfx.h"
#include <intrin.h>

#define USE_INTRINSICS

void genAHash_8x8(const LPCOLORREF pixels, const size_t stride, ImgHash8x8* out_hash)
{
	size_t sumLuminozity[COLOR_CHANNEL_COUNT] = { 0 };
	for (size_t row = 0; row < 8; row++)
	{
		unsigned char* pixels2 = (unsigned char*)&pixels[row * stride];
		for (size_t col = 0; col < 8; col++)
		{
			sumLuminozity[0] += pixels2[0];
			sumLuminozity[1] += pixels2[1];
			sumLuminozity[2] += pixels2[2];
			pixels2 += 4;
		}
	}

	// get the avg 
	sumLuminozity[0] = sumLuminozity[0] / (8 * 8);
	sumLuminozity[1] = sumLuminozity[1] / (8 * 8);
	sumLuminozity[2] = sumLuminozity[2] / (8 * 8);

	// generate bits
	out_hash->rHash = out_hash->gHash = out_hash->bHash = 0;

	for (size_t row = 0; row < 8; row++)
	{
		unsigned char* pixels2 = (unsigned char*)&pixels[row * stride];
		for (size_t col = 0; col < 8; col++)
		{
			size_t bit;
			bit = pixels2[0] < sumLuminozity[0];
			PUSH_BIT(out_hash->bHash, bit);
			bit = pixels2[1] < sumLuminozity[1];
			PUSH_BIT(out_hash->gHash, bit);
			bit = pixels2[2] < sumLuminozity[2];
			PUSH_BIT(out_hash->rHash, bit);
			pixels2 += 4;
		}
	}
}

void genBHash_8x8(const LPCOLORREF pixels, const size_t stride, ImgHash8x8* out_hash)
{
	size_t prevR, prevG, prevB;

	// generate bits
	out_hash->rHash = out_hash->gHash = out_hash->bHash = 0;

	prevR = prevG = prevB = 127;
	for (size_t row = 0; row < 8; row++)
	{
		unsigned char* pixels2 = (unsigned char*)&pixels[row * stride];
		for (size_t col = 0; col < 8; col++)
		{
			size_t bit;
			bit = pixels2[0] < prevR;
			PUSH_BIT(out_hash->bHash, bit);
			prevR = pixels2[0];

			bit = pixels2[1] < prevG;
			PUSH_BIT(out_hash->gHash, bit);
			prevG = pixels2[1];

			bit = pixels2[2] < prevB;
			PUSH_BIT(out_hash->rHash, bit);
			prevB = pixels2[2];

			pixels2 += 4;
		}
	}
}

void genCHash_8x8(const LPCOLORREF pixels, const size_t stride, ImgHash8x8* out_hash)
{
	size_t prevR, prevG, prevB;

	// generate bits
	out_hash->rHash = out_hash->gHash = out_hash->bHash = 0;

	prevR = prevG = prevB = 127;
	for (size_t col = 0; col < 8; col++)
	{
		LPCOLORREF pixels3 = &pixels[col];
		for (size_t row = 0; row < 8; row++)
		{
			unsigned char* pixels2 = (unsigned char*)&pixels3[row * stride];
			size_t bit;
			bit = pixels2[0] < prevR;
			PUSH_BIT(out_hash->bHash, bit);
			prevR = pixels2[0];

			bit = pixels2[1] < prevG;
			PUSH_BIT(out_hash->gHash, bit);
			prevG = pixels2[1];

			bit = pixels2[2] < prevB;
			PUSH_BIT(out_hash->rHash, bit);
			prevB = pixels2[2];
		}
	}
}

#if !defined(USE_INTRINSICS)
static inline size_t bitCount(size_t num)
{
	size_t setBits = 0;
	while (num > 0) {
		setBits += (num & 1);
		num >>= 1;
	}
	return setBits;
}
#else
static inline size_t bitCount(uint64_t x) {
	return __popcnt64(x);
}
#endif

template <const bool bIsAcumulate>
static inline void compareHash_8x8(const ImgHash8x8* __restrict h1, const ImgHash8x8* __restrict h2, ImgHash8x8_CompareResult* out)
{
	if constexpr (bIsAcumulate == false) {
		memset(out, 0, sizeof(ImgHash8x8_CompareResult));
	}

	const uint64_t valxorr = ((uint64_t)h1->rHash) ^ ((uint64_t)h2->rHash);
	const uint64_t rBitsDifferent = bitCount(valxorr);
	const uint64_t valxorg = ((uint64_t)h1->gHash) ^ ((uint64_t)h2->gHash);
	const uint64_t gBitsDifferent = bitCount(valxorg);
	const uint64_t valxorb = ((uint64_t)h1->bHash) ^ ((uint64_t)h2->bHash);
	const uint64_t bBitsDifferent = bitCount(valxorb);

	out->rgbBitsDiffer += rBitsDifferent;
	out->rgbBitsDiffer += gBitsDifferent;
	out->rgbBitsDiffer += bBitsDifferent;

	out->blocksAcumulated++;
}

const size_t kernel_half_size = 1;

// based on input pixels, we generate multiple hashes to cover an area
template <const bool all_positions>
int64_t GenHashesForGenericImage(LPCOLORREF Pixels, int Width, int Height, int Stride,
	size_t atX, size_t atY,
	ImgHashWholeImage* out_hashes)
{
	// sanity checks
	{
		// the blurring will be unusable at the edge of the image
		size_t usable_width = Width - 2 * kernel_half_size;
		size_t usable_height = Height - 2 * kernel_half_size;
		size_t HashColCnt = usable_width / HashPixelWidth;
		size_t HashRowCnt = usable_height / HashPixelHeight;

		// sanity. We can't process too small images
		if (HashRowCnt <= 0 || HashColCnt <= 0) {
			return -1;
		}
	}

	size_t PixelStep;
	if constexpr (all_positions) {
		PixelStep = 1;
	}
	else {
		PixelStep = HashPixelWidth;
	}

	// check if we already have hashes calculated for this image. Only makes sense when working on screenshots
	bool HasAllHashesCalculated = true;
	for (size_t pixelY = kernel_half_size; pixelY <= Height - kernel_half_size - HashPixelHeight; pixelY += PixelStep) {
		for (size_t pixelX = kernel_half_size; pixelX <= Width - kernel_half_size - HashPixelWidth; pixelX += PixelStep) {
			// cols represents RowStride
			// caches store 1 hash for every 8x8 pixel block
			// screenshots store 1 cache for every pixel location to be able to search based on hash comparison
			ImgHash8x8_All* hashes2 = out_hashes->GetHashStore(atX + pixelX, atY + pixelY);
			if (hashes2->HasHashesGenerated == 0) {
				HasAllHashesCalculated = false;
				break;
			}
		}
		if (HasAllHashesCalculated == false) {
			break;
		}
	}
	if (HasAllHashesCalculated == true) {
		FileDebug("GenHashesForCachedImage: Amaizing, found one time when caching was usefull");
		return 0;
	}

	// blurring is advised or else it will be very sensitive to small pixel differences
	LPCOLORREF Pixels2 = &Pixels[atY * Stride + atX];
	LPCOLORREF blurredImg = BlurrImage2_<kernel_half_size, 1.0>(Pixels2, Width, Height, Stride);
	const size_t BlurredImageStride = Width;
	if (blurredImg == NULL)
	{
		FileDebug("GenHashesForCachedImage: mem allocation error");
		return -1;
	}

/*	if (Width != Stride) {
		SaveImage_(Pixels2, Width, Height, Stride, "Original.bmp");
		SaveImage_(blurredImg, Width, Height, Width, "blurred.bmp");
	}/**/

	// how many 8x8 blocks fit in the requested area, we calculate a hash
	for (size_t pixelY = kernel_half_size; pixelY <= Height - kernel_half_size - HashPixelHeight; pixelY += PixelStep) {
		for (size_t pixelX = kernel_half_size; pixelX <= Width - kernel_half_size - HashPixelWidth; pixelX += PixelStep) {
			// caches store 1 hash for every 8x8 pixel block
			// screenshots store 1 cache for every pixel location to be able to search based on hash comparison
			ImgHash8x8_All* hashes2 = out_hashes->GetHashStore(atX + pixelX, atY + pixelY);
			if (hashes2->HasHashesGenerated) {
				continue;
			}
			// make sure we do not recalculate hash for this pixel position next time
			hashes2->HasHashesGenerated = 1;
			// cache pixels are hashed at every 8x8 pixel intervals
			LPCOLORREF usable_pixels2 = &blurredImg[pixelY * BlurredImageStride + pixelX];
			genAHash_8x8(usable_pixels2, BlurredImageStride, &hashes2->AHash);
			genBHash_8x8(usable_pixels2, BlurredImageStride, &hashes2->BHash);
			genCHash_8x8(usable_pixels2, BlurredImageStride, &hashes2->CHash);
		}
	}

	// no more use for the blurred version of the image region
	MY_FREE(blurredImg);

	return 0;
}

// gen all hashes for a staticly cached image
void GenHashesForCachedImage(CachedPicture* pic, ImgHashCache* out_hashes)
{
	// cached images will generate hashes only once
	if (out_hashes->hashes != NULL) {
		return;
	}
	out_hashes->imgWidth = pic->Width;
	out_hashes->imgHeight = pic->Height;
	out_hashes->cols = (pic->Width - 2 * kernel_half_size) / HashPixelWidth;
	out_hashes->rows = (pic->Height - 2 * kernel_half_size) / HashPixelHeight;
	// sanity
	out_hashes->initHashes();
	// for each 8x8 pixel block, generate 1 hash
	GenHashesForGenericImage<false>(pic->Pixels, pic->Width, pic->Height, pic->Width, 0, 0, out_hashes);
}

// when we check a static cahed image if it is present on the screenshot, we need to have the hashes on the screeenshot
template <const bool bGenAllPositions>
int64_t GenHashesOnScreenshotForCachedImage(CachedPicture* pic, ScreenshotStruct* ss, size_t atX, size_t atY)
{
	if (atY + pic->Height >= ss->Height)
	{
		return -1;
	}
	if (atX + pic->Width >= ss->Width)
	{
		return -1;
	}
	return GenHashesForGenericImage<bGenAllPositions>(ss->Pixels, pic->Width, pic->Height, ss->Width, atX, atY, ss->pSSHashCache);
}

int64_t compareHash(const ImgHashSS* __restrict hashSS, const ImgHashCache* __restrict hashC, size_t atX, size_t atY, ImgHash8x8_CompareResult* out)
{
	if (hashSS == NULL || hashC == NULL || out == NULL)
	{
		return -1;
	}
	memset(out, 0, sizeof(ImgHash8x8_CompareResult));
	for (size_t y = kernel_half_size; y <= hashC->imgHeight - kernel_half_size - HashPixelHeight; y+= HashPixelHeight)
	{
		for (size_t x = kernel_half_size; x <= hashC->imgWidth - kernel_half_size - HashPixelWidth; x+= HashPixelWidth)
		{
			ImgHash8x8_All* hSS = hashSS->GetHashStore(atX + x, atY + y);
			ImgHash8x8_All* hC = hashC->GetHashStore(x, y);
#ifdef _DEBUG
			if (hSS->HasHashesGenerated == 0) {
				FileDebug("!!!Hash1 does not have a hash generated for all required locations");
			}
			if (hC->HasHashesGenerated == 0) {
				FileDebug("!!!Hash2 does not have a hash generated for all required locations");
			}
#endif
			compareHash_8x8<true>(&hSS->AHash, &hC->AHash, out);
			compareHash_8x8<true>(&hSS->BHash, &hC->BHash, out);
			compareHash_8x8<true>(&hSS->CHash, &hC->CHash, out);
		}
	}

	const size_t hashBitCount = HashPixelWidth * HashPixelHeight;
	const size_t rgbHashChannels = 3;
	out->PctDifferAvg = out->rgbBitsDiffer * 10000 / ( out->blocksAcumulated * hashBitCount * rgbHashChannels);


	return 0;
}

// we are done with this cache can be freed
void FreeHashAllocatedData(ImgHashWholeImage* out_hashes)
{
	if (out_hashes->hashes) {
		MY_FREE(out_hashes->hashes);
		out_hashes->hashes = NULL;
	}
}

// function called after a new screenshot was taken(or attempted taken)
void ReinitScreenshotHashCache(ScreenshotStruct* ss)
{
	if (ss->pSSHashCache == NULL) {
		// create a new hash storage
		ss->pSSHashCache = new ImgHashSS();
	}
	// no need to reinit. linked image remained the same
	else if (ss->UniqueFameCounter == ss->pSSHashCache->UniqueFameCounter) {
		return;
	}
	// linked image changed. maybe we could keep the allocated mem. Todo : check if width/height changed
	else if (ss->UniqueFameCounter != ss->pSSHashCache->UniqueFameCounter) {
		// ditch previously generated hashes
		FreeHashAllocatedData(ss->pSSHashCache);
	}
	// reinit the struct
	if (ss->pSSHashCache) {
		ss->pSSHashCache->reinit();
		// just in case the input image resolution changed
		ss->pSSHashCache->imgWidth = ss->Width;
		ss->pSSHashCache->imgHeight = ss->Height;
		ss->pSSHashCache->cols = ss->Width;
		ss->pSSHashCache->rows = ss->Height;
		// mark the cache that we are generating hashes for this specific screenshot
		ss->pSSHashCache->UniqueFameCounter = ss->UniqueFameCounter;
		// alloc hash store for the whole image
		ss->pSSHashCache->initHashes();
	}
}

ImgHashWholeImage::~ImgHashWholeImage() {
	FreeHashAllocatedData(this);
}

// force template initialization for later usage
template int64_t GenHashesOnScreenshotForCachedImage<false>(CachedPicture*, ScreenshotStruct*, size_t, size_t);
template int64_t GenHashesOnScreenshotForCachedImage<true>(CachedPicture*, ScreenshotStruct*, size_t, size_t);