#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace devilution_mpq_tools {

std::string ExtractSpellIcons(std::span<const uint8_t> clxData,
    std::vector<uint8_t> &iconBackground, std::vector<uint8_t> &iconsWithoutBackground);

} // namespace devilution_mpq_tools
