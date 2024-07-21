
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <span>
#include <vector>

#include <dvl_gfx_embedded_palettes.h>

namespace {

constexpr uint8_t MinBgColor = 192;
constexpr uint8_t MaxBgColor = 206;
constexpr float MaxDistance = 250.0F;

float ColorDistanceSquare(uint8_t idxA, uint8_t idxB)
{
	const uint8_t *a = reinterpret_cast<const uint8_t *>(&dvl_gfx_embedded_default_pal_data[idxA * 3]);
	const uint8_t *b = reinterpret_cast<const uint8_t *>(&dvl_gfx_embedded_default_pal_data[idxB * 3]);
	const float wR = 2.0F + static_cast<float>(a[0] + b[0]) / 512.0F;
	const float wG = 4.0F;
	const float wB = 2 + (255 - static_cast<float>(a[0] + b[0]) / 2.0F) / 256.0F;
	const int dR = a[0] - b[0];
	const int dG = a[1] - b[1];
	const int dB = a[2] - b[2];
	// NOLINTNEXTLINE(bugprone-narrowing-conversions)
	return std::sqrt(wR * dR * dR + wG * dG * dG + wB * dB * dB);
}

void printConditionSet(std::span<const unsigned> xs)
{
	bool printed = false;
	const auto printRange = [&printed](unsigned begin, unsigned end) {
		if (begin == end)
			return;
		switch (end - begin) {
		case 1:
			if (printed)
				std::printf(" || ");
			std::printf("d == %d", begin);
			printed = true;
			break;
		case 2:
			if (printed)
				std::printf(" || ");
			if (begin == 0) {
				std::printf("d <= %d", end - 1);
			} else {
				std::printf("d == %d || d == %d", begin, begin + 1);
			}
			printed = true;
			break;
		default:
			if (printed)
				std::printf(" || ");
			if (begin == 0) {
				std::printf("(d <= %d)", end - 1);
			} else {
				std::printf("(d >= %d && d <= %d)", begin, end - 1);
			}
			printed = true;
			break;
		}
		printed = true;
	};
	unsigned rangeBegin = xs[0];
	for (size_t i = 1; i < xs.size(); ++i) {
		if (xs[i - 1] + 1 == xs[i]) {
			continue;
		} else {
			printRange(rangeBegin, i);
			rangeBegin = i;
		}
	}
	if (!printed && xs.back() - rangeBegin > 2) {
		if (rangeBegin == 0) {
			std::printf("d <= %d", xs.back());
		} else {
			std::printf("d >= %d && d <= %d", rangeBegin, xs.back());
		}
	} else {
		printRange(rangeBegin, xs.back() + 1);
	}
}

void printPairs(std::span<const std::pair<int, int>> pairs)
{
	if (pairs.size() == 1) {
		std::printf("a == %d && b == %d", pairs[0].first, pairs[0].second);
		return;
	}
	for (unsigned i = 0; i < pairs.size(); ++i) {
		if (i != 0)
			std::printf(" || ");
		std::printf("(a == %d && b == %d)", pairs[i].first, pairs[i].second);
	}
}

} // namespace

int main()
{
	bool array[MaxBgColor - MinBgColor + 1][MaxBgColor - MinBgColor + 1];

	unsigned numTrueDist[MaxBgColor - MinBgColor + 1] = {};
	unsigned numTotalDist[MaxBgColor - MinBgColor + 1] = {};

	for (uint8_t i = MinBgColor; i <= MaxBgColor; ++i) {
		for (uint8_t j = MinBgColor; j <= i; ++j) {
			const bool ok = ColorDistanceSquare(i, j) < MaxDistance;
			array[i - MinBgColor][j - MinBgColor] = ok;
			if (ok) {
				++numTrueDist[i - j];
			}
			++numTotalDist[i - j];
		}
	}
	std::vector<unsigned> alwaysTrue;
	std::vector<unsigned> alwaysFalse;
	for (unsigned dist = 0; dist <= MaxBgColor - MinBgColor; ++dist) {
		if (numTrueDist[dist] == 0) {
			alwaysFalse.push_back(dist);
		} else if (numTrueDist[dist] == numTotalDist[dist]) {
			alwaysTrue.push_back(dist);
		}
	}
	std::printf("const auto [a, b] = std::minmax({ fg, bg });\n");
	if (!alwaysTrue.empty() || !alwaysFalse.empty()) {
		std::printf("const auto d = static_cast<unsigned>(b - a);\n");
	}
	if (!alwaysTrue.empty()) {
		std::printf("if (");
		printConditionSet(alwaysTrue);
		std::printf(") return true;\n");
	}
	if (!alwaysFalse.empty()) {
		std::printf("if (");
		printConditionSet(alwaysFalse);
		std::printf(") return false;\n");
	}

	unsigned numOk = 0;
	unsigned numTotal = 0;
	for (uint8_t i = MinBgColor; i <= MaxBgColor; ++i) {
		for (uint8_t j = MinBgColor; j <= i; ++j) {
			if (numTrueDist[i - j] == 0 || numTrueDist[i - j] == numTotalDist[i - j]) {
				continue;
			}
			if (array[i - MinBgColor][j - MinBgColor])
				++numOk;
			++numTotal;
		}
	}

	std::vector<std::pair<int, int>> pairs;
	const bool printOk = numOk * 2 < numTotal;
	for (uint8_t i = MinBgColor; i <= MaxBgColor; ++i) {
		for (uint8_t j = MinBgColor; j <= i; ++j) {
			if (numTrueDist[i - j] == 0 || numTrueDist[i - j] == numTotalDist[i - j]) {
				continue;
			}
			const bool ok = array[i - MinBgColor][j - MinBgColor];
			if (printOk == ok) {
				pairs.emplace_back(j, i);
			}
		}
	}
	if (!pairs.empty()) {
		std::printf("if (");
		printPairs(pairs);
		std::printf(") return %s;\n", printOk ? "true" : "false");
	}
	std::printf("return %s;\n", printOk ? "false" : "true");
	return 0;
}
