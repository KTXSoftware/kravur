#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#include <stdio.h>
#include <fstream>

typedef unsigned char u8;
typedef short s16;
typedef int s32;

static int round(float value) {
	return (int)floorf(value + 0.5f);
}

static void writeS16(std::ofstream& out, s16 value) {
	out.put((value & 0xff00) >> 8);
	out.put((value & 0xff) >> 0);
}

static void writeS32(std::ofstream& out, s32 value) {
	out.put((value & 0xff000000) >> 24);
	out.put((value & 0xff0000) >> 16);
	out.put((value & 0xff00) >> 8);
	out.put((value & 0xff) >> 0);
}

static void writeFloat(std::ofstream& out, float value) {
	writeS32(out, *reinterpret_cast<int*>(&value));
}

int main(int argc, char** argv) {
	if (argc < 4) {
		printf("Usage: kravur infile fontsize outfile");
		return 1;
	}

	float size = static_cast<float>(atoi(argv[2]));

	if (size <= 0) {
		printf("Weird fontsize");
		return 1;
	}

	FILE* in = fopen(argv[1], "rb");
	if (!in) {
		printf("Error: unable to open input file: %s\n", argv[1]);
		return 1;
	}

	fseek(in, 0, SEEK_END);
	int length = ftell(in);
	rewind(in);

	u8* data = new u8[length];
	fread(data, 1, length, in);
	
	fclose(in);

	int width = 128;
	int height = 64;
	stbtt_bakedchar baked[256 - 32];

	u8* pixels = nullptr;

	int status = -1;
	while (status < 0) {
		if (height < width) height *= 2;
		else width *= 2;
		delete pixels;
		pixels = new u8[width * height];
		status = stbtt_BakeFontBitmap(data, 0, size, pixels, width, height, 32, 256 - 32, &baked[0]);
	}
	
	stbtt_fontinfo info;
	stbtt_InitFont(&info, data, 0);
	
	int ascent;
	int descent;
	int lineGap;
	stbtt_GetFontVMetrics(&info, &ascent, &descent, &lineGap);
	float scale = stbtt_ScaleForPixelHeight(&info, size);
	ascent = round(ascent * scale); // equals baseline
	descent = round(descent * scale);
	lineGap = round(lineGap * scale);

	std::ofstream out(argv[3], std::ios_base::binary);
	writeS32(out, (int)size);
	writeS32(out, ascent);
	writeS32(out, descent);
	writeS32(out, lineGap);
	for (int i = 0; i < 256 - 32; ++i) {
		writeS16(out, baked[i].x0);
		writeS16(out, baked[i].y0);
		writeS16(out, baked[i].x1);
		writeS16(out, baked[i].y1);
		writeFloat(out, baked[i].xoff);
		writeFloat(out, baked[i].yoff);
		writeFloat(out, baked[i].xadvance);
	}
	writeS32(out, width);
	writeS32(out, height);
	for (int y = 0; y < height; ++y) for (int x = 0; x < width; ++x) {
		out.put(pixels[y * width + x]);
	}

	printf("Written %s.\n", argv[3]);

	return 0;
}