#include "extract_spell_icons.hpp"

#include <cstring>
#include <string>

#include <clx2pixels.hpp>
#include <clx_decode.hpp>
#include <pixels2clx.hpp>

namespace devilution_mpq_tools {

namespace {

constexpr uint8_t TransparentColor = 255;

void RemoveBackground(uint8_t *pixels, unsigned width, unsigned height,
    unsigned outerTop, unsigned outerRight, unsigned outerLeft, unsigned outerBottom,
    const uint8_t *bg)
{
	// Remove top border:
	std::memset(pixels, TransparentColor, width * outerTop);

	const unsigned innerWidth = width - outerLeft - outerRight;
	const unsigned innerHeight = height - outerBottom - outerTop;

	// First round: remove borders, diff against the background.
	// Unfortunately, this alone is not enough because the backgrounds
	// are all slightly different (looks like noise).
	for (unsigned y = outerTop, yEnd = outerTop + innerHeight; y < yEnd; ++y) {
		// Remove left border:
		std::memset(&pixels[static_cast<size_t>(y * width)],
		    TransparentColor, outerLeft);
		for (unsigned x = outerLeft, xEnd = outerLeft + innerWidth; x < xEnd; ++x) {
			uint8_t &pixel = pixels[y * width + x];
			if (bg[y * width + x] == pixel)
				pixel = TransparentColor;
		}
		// Remove right border:
		std::memset(&pixels[y * width + outerLeft + innerWidth],
		    TransparentColor, outerRight);
	}
	// Remove bottom border:
	std::memset(&pixels[static_cast<size_t>((outerTop + innerHeight) * width)],
	    TransparentColor, width * outerBottom);

	// Remove pixels that have more than 4 neighbours that are
	// either already transparent or are in the definitely-background color range.
	for (unsigned y = outerTop, yEnd = outerTop + innerHeight; y < yEnd; ++y) {
		for (unsigned x = outerLeft, xEnd = outerLeft + innerWidth; x < xEnd; ++x) {
			uint8_t &pixel = pixels[y * width + x];
			if (pixel == TransparentColor || pixel < 192 || pixel > 205)
				continue;
			unsigned numTransparent = 0;
			for (auto [dx, dy] : std::initializer_list<std::pair<int, int>> {
			         { -1, -1 }, { -1, 0 }, { -1, 1 },
			         { 0, -1 }, { 0, 1 },
			         { 1, -1 }, { 1, 0 }, { 1, 1 } }) {
				const uint8_t color = pixels[(y + dy) * width + (x + dx)];
				if (color == TransparentColor || (color >= 192 && color < 199))
					++numTransparent;
			}
			if (numTransparent > 4)
				pixel = TransparentColor;
		}
	}
}

} // namespace

std::string ExtractSpellIcons(std::span<const uint8_t> clxData,
    std::vector<uint8_t> &iconBackground, std::vector<uint8_t> &iconsWithoutBackground)
{
	std::vector<uint8_t> pixels;
	const std::optional<dvl_gfx::IoError> clxError = dvl_gfx::Clx2Pixels(clxData, TransparentColor, pixels);
	if (clxError.has_value())
		return "Failed CLX->Pixels conversion: " + clxError->message;

	size_t numSprites = dvl_gfx::GetNumSpritesFromClxList(clxData.data());
	const std::span<const uint8_t> firstSprite = dvl_gfx::GetSpriteDataFromClxList(clxData.data(), 0);
	const unsigned width = dvl_gfx::GetClxSpriteWidth(firstSprite.data());
	const unsigned height = dvl_gfx::GetClxSpriteHeight(firstSprite.data());

	unsigned outerTop;
	unsigned outerRight;
	unsigned outerLeft;
	unsigned outerBottom;
	const size_t emptySprite = 26;
	if (width == 37 && height == 38) {
		// `spelli2`, the last sprite is unused
		--numSprites;
		outerLeft = outerBottom = 1;
		outerRight = outerTop = 2;
	} else if (width == 56 && height == 56) {
		// `spelicon`, the last 9 sprites are overlays, unused in DevilutionX.
		numSprites -= 9;
		outerLeft = outerBottom = 5;
		outerRight = outerTop = 4;
	} else {
		return "Unsupported icon size";
	}

	dvl_gfx::Pixels2Clx(&pixels[emptySprite * width * height],
	    /*pitch=*/width, width, /*frameHeight=*/height, /*numFrames=*/1,
	    TransparentColor, iconBackground);

	for (size_t frame = 0; frame < numSprites; ++frame) {
		if (frame == emptySprite)
			continue;
		RemoveBackground(&pixels[frame * width * height], width, height,
		    outerTop, outerRight, outerBottom, outerLeft, &pixels[emptySprite * width * height]);
	}
	std::memset(&pixels[emptySprite * width * height], TransparentColor, width * height);

	dvl_gfx::Pixels2Clx(pixels.data(),
	    /*pitch=*/width, width, /*frameHeight=*/height, numSprites,
	    TransparentColor, iconsWithoutBackground);
	return "";
}

} // namespace devilution_mpq_tools
