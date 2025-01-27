#include "StdAfx.h"

void genAHash_8x8(const LPCOLORREF pixels, const size_t stride, ImgHash8x8* out_hash)
{
	size_t sumLuminozity[COLOR_CHANNEL_COUNT] = { 0 };
	for (size_t row = 0; row < 8; row++)
	{
		unsigned char* pixels2 = (unsigned char*)&pixels[row * stride];
		for (size_t col = 0; col < 8; col++)
		{
			sumLuminozity[0] = pixels2[0];
			sumLuminozity[1] = pixels2[1];
			sumLuminozity[2] = pixels2[2];
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

static inline size_t bitCount(size_t num)
{
	size_t setBits = 0;
	while (num > 0) {
		setBits += (num & 1);
		num >>= 1;
	}
	return setBits;
}

void compareHash_8x8(ImgHash8x8* h1, ImgHash8x8* h2, ImgHash8x8_CompareResult* out, bool bIsAcumulate)
{
	if (bIsAcumulate == false) {
		memset(out, 0, sizeof(ImgHash8x8_CompareResult));
	}

	size_t valxor = ((int)h1->rHash) ^ ((int)h2->rHash);
	size_t rBitsMatch = bitCount(valxor);
	valxor = ((int)h1->gHash) ^ ((int)h2->gHash);
	size_t gBitsMatch = bitCount(valxor);
	valxor = ((int)h1->bHash) ^ ((int)h2->bHash);
	size_t bBitsMatch = bitCount(valxor);

	const size_t hashBitcount = 8 * 8;

	out->rBitsMatch += rBitsMatch;
	out->gBitsMatch += gBitsMatch;
	out->bBitsMatch += bBitsMatch;

	out->blocksAcumulated++;

	out->rPctMatch = out->rBitsMatch * 100.0 / double(out->blocksAcumulated * hashBitcount);
	out->gPctMatch = out->gBitsMatch * 100.0 / double(out->blocksAcumulated * hashBitcount);
	out->bPctMatch = out->bBitsMatch * 100.0 / double(out->blocksAcumulated * hashBitcount);

	out->pctMatchAvg = (out->rPctMatch + out->gPctMatch + out->bPctMatch) / 3;
}

static void GenHashesForGenericImage(LPCOLORREF Pixels, int Width, int Height, int Stride, ImgHashWholeIage* out_hashes)
{
	const size_t kernel_half_size = 1;
	LPCOLORREF blurredImg = BlurrImage2_<kernel_half_size>(Pixels, Width, Height, Stride, 1.0);
	if (blurredImg == NULL)
	{
		FileDebug("GenHashesForCachedImage: mem allocation error");
		return;
	}

	// the blurring will be unusable at the edge of the image
	size_t usable_width = Width - 2 * kernel_half_size;
	size_t usable_height = Height - 2 * kernel_half_size;
	size_t usable_stride = Stride;
	LPCOLORREF usable_pixels = &blurredImg[kernel_half_size + kernel_half_size * usable_stride];

	// store multiple out_hashes
	if (out_hashes->hashes != NULL && (out_hashes->rows != (usable_height / 8) || out_hashes->cols != (usable_width / 8)))
	{
		MY_FREE(out_hashes->hashes);
		out_hashes->hashes = NULL;
	}
	out_hashes->rows = usable_height / 8;
	out_hashes->cols = usable_width / 8;
	if (out_hashes->rows <= 0 || out_hashes->cols <= 0)
	{
		return;
	}
	if (out_hashes->hashes == NULL)
	{
		out_hashes->hashes = (ImgHash8x8_All*)MY_ALLOC(out_hashes->rows * out_hashes->cols * sizeof(ImgHash8x8_All));
	}
	if (out_hashes->hashes == NULL)
	{
		FileDebug("GenHashesForCachedImage: mem allocation error");
		return;
	}

	for (size_t row_hash = 0; row_hash < out_hashes->rows; row_hash++)
	{
		for (size_t col_hash = 0; col_hash < out_hashes->cols; col_hash++)
		{
			LPCOLORREF usable_pixels2 = &usable_pixels[row_hash * 8 * usable_stride + col_hash * 8];
			ImgHash8x8_All* hashes2 = &out_hashes->hashes[row_hash * out_hashes->cols + col_hash];
			genAHash_8x8(usable_pixels2, usable_stride, &hashes2->AHash);
			genBHash_8x8(usable_pixels2, usable_stride, &hashes2->BHash);
			genCHash_8x8(usable_pixels2, usable_stride, &hashes2->CHash);
		}
	}

	MY_FREE(blurredImg);
}

void GenHashesForCachedImage(CachedPicture* pic, ImgHashWholeIage* out_hashes)
{
	GenHashesForGenericImage(pic->Pixels, pic->Width, pic->Height, pic->Width, out_hashes);
}

void GenHashesOnScreenshotForCachedImage(CachedPicture* pic, ScreenshotStruct* ss, int atX, int atY, ImgHashWholeIage* out_hashes)
{
	LPCOLORREF PixelsAt = &ss->Pixels[atY * ss->Width + atX];
	GenHashesForGenericImage(PixelsAt, pic->Width, pic->Height, ss->Width, out_hashes);
}

void compareHash(ImgHashWholeIage* hash1, ImgHashWholeIage* hash2, ImgHash8x8_CompareResult* out)
{
	memset(out, 0, sizeof(ImgHash8x8_CompareResult));
	const size_t cols = min(hash1->cols, hash2->cols);
	const size_t rows = min(hash1->rows, hash2->rows);
	for (size_t row = 0; row < rows; row++)
	{
		for (size_t col = 0; col < cols; col++)
		{
			compareHash_8x8(&hash1->hashes[row * hash1->rows + col].AHash, &hash2->hashes[row * hash2->rows + col].AHash, out, true);
			compareHash_8x8(&hash1->hashes[row * hash1->rows + col].BHash, &hash2->hashes[row * hash2->rows + col].BHash, out, true);
			compareHash_8x8(&hash1->hashes[row * hash1->rows + col].CHash, &hash2->hashes[row * hash2->rows + col].CHash, out, true);
		}
	}
}

void FreeHashAllocatedData(ImgHashWholeIage* out_hashes)
{
	if (out_hashes->hashes) {
		MY_FREE(out_hashes->hashes);
		out_hashes->hashes = NULL;
	}
}