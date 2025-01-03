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
	prevR = prevG = prevB = 0;
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
	prevR = prevG = prevB = 0;
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

			pixels2 += 4;
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

ImgHashWholeIage *GenHashesForCachedImage(CachedPicture* pic)
{
	const size_t kernel_half_size = 2;
	LPCOLORREF blurredImg = BlurrImage2_<kernel_half_size>(pic->Pixels, pic->Width, pic->Height, pic->Width, 1.0);
	if (blurredImg == NULL)
	{
		FileDebug("GenHashesForCachedImage: mem allocation error");
		return NULL;
	}
	
	// the blurring will be unusable at the edge of the image
	size_t usable_width = pic->Width - 2 * kernel_half_size;
	size_t usable_height = pic->Height - 2 * kernel_half_size;
	size_t usable_stride = pic->Width;
	LPCOLORREF usable_pixels = &blurredImg[kernel_half_size + kernel_half_size * usable_stride];

	// store multiple hashes
	ImgHashWholeIage *hashes = (ImgHashWholeIage*)malloc(sizeof(ImgHashWholeIage));
	if (hashes == NULL)
	{
		FileDebug("GenHashesForCachedImage: mem allocation error");
		return NULL;
	}
	memset(hashes, 0, sizeof(hashes));
	hashes->rows = usable_height / 8;
	hashes->cols = usable_width / 8;
	hashes->hashes = (ImgHash8x8_All*)malloc(hashes->rows * hashes->cols * sizeof(ImgHash8x8_All));
	if (hashes->hashes == NULL)
	{
		FileDebug("GenHashesForCachedImage: mem allocation error");
		return NULL;
	}

	for (size_t row_hash = 0; row_hash < hashes->rows; row_hash++)
	{
		for (size_t col_hash = 0; col_hash < hashes->cols; col_hash++)
		{
			LPCOLORREF usable_pixels2 = &usable_pixels[row_hash * 8 * usable_stride + col_hash * 8];
			ImgHash8x8_All *hashes2 = &hashes->hashes[row_hash * hashes->cols + col_hash];
			genAHash_8x8(usable_pixels2, usable_stride, &hashes2->AHash);
			genBHash_8x8(usable_pixels2, usable_stride, &hashes2->BHash);
			genCHash_8x8(usable_pixels2, usable_stride, &hashes2->CHash);
		}
	}

	_aligned_free(blurredImg);

	return hashes;
}