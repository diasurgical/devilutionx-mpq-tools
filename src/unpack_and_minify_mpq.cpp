#include <algorithm>
#include <cerrno>
#include <charconv>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

#include <cel2clx.hpp>
#include <cl22clx.hpp>
#include <libmpq/mpq.h>
#include <pcx2clx.hpp>

#include "embedded_files.h"

namespace {

constexpr char kHelp[] = R"(Usage: unpack_and_minify_mpq [-h] [--output-dir OUTPUT_DIR] [--listfile LISTFILE] [--mp3] [mpq ...]

Unpacks Diablo and/or Hellfire MPQ(s), converts all the graphics to CLX, and, optionally, converts audio to MP3.
If no MPQs are passed on the command line, converts all the MPQs in the current directory.

Options:
  --mp3                       Convert WAV files to MP3.
  --output-dir OUTPUT_DIR     Override output directory. Default: current directory.
)";

void PrintHelp()
{
	std::cerr << kHelp << std::endl;
}

std::string SrcName(const std::filesystem::path &mpq)
{
	std::string result = mpq.stem();
	if (result == "DIABDAT")
		result = "diabdat";
	return result;
}

std::string DestName(const std::string &srcName)
{
	if (srcName == "hfmonk" || srcName == "hfmusic" || srcName == "hfvoice")
		return "hellfire";
	return srcName;
}

std::span<const char *const> GetMpqFiles(std::string_view srcName)
{
	if (srcName == "spawn")
		return { embedded_spawn_listfile_data, embedded_spawn_listfile_size };
	if (srcName == "diabdat")
		return { embedded_diabdat_listfile_data, embedded_diabdat_listfile_size };
	if (srcName == "hellfire")
		return { embedded_hellfire_listfile_data, embedded_hellfire_listfile_size };
	if (srcName == "hfmonk")
		return { embedded_hfmonk_listfile_data, embedded_hfmonk_listfile_size };
	if (srcName == "hfmusic")
		return { embedded_hfmusic_listfile_data, embedded_hfmusic_listfile_size };
	if (srcName == "hfvoice")
		return { embedded_hfvoice_listfile_data, embedded_hfvoice_listfile_size };
	return {};
}

std::span<const char *const> GetExcludedFiles(std::string_view srcName)
{
	if (srcName == "spawn")
		return { embedded_spawn_rm_data, embedded_spawn_rm_size };
	if (srcName == "diabdat")
		return { embedded_diabdat_rm_data, embedded_diabdat_rm_size };
	if (srcName == "hellfire")
		return { embedded_hellfire_rm_data, embedded_hellfire_rm_size };
	if (srcName == "hfmonk")
		return { embedded_hfmonk_rm_data, embedded_hfmonk_rm_size };
	if (srcName == "hfmusic")
		return { embedded_hfmusic_rm_data, embedded_hfmusic_rm_size };
	if (srcName == "hfvoice")
		return { embedded_hfvoice_rm_data, embedded_hfvoice_rm_size };
	return {};
}

std::span<const char *const> GetClxCommands(std::string_view srcName)
{
	if (srcName == "spawn")
		return { embedded_spawn_clx_data, embedded_spawn_clx_size };
	if (srcName == "diabdat")
		return { embedded_diabdat_clx_data, embedded_diabdat_clx_size };
	if (srcName == "hellfire")
		return { embedded_hellfire_clx_data, embedded_hellfire_clx_size };
	if (srcName == "hfmonk")
		return { embedded_hfmonk_clx_data, embedded_hfmonk_clx_size };
	return {};
}

struct Cl2ToClxCommand {
	std::vector<uint16_t> widths;
};
struct CelToClxCommand {
	std::vector<uint16_t> widths;
};
struct PcxToClxCommand {
	size_t numFrames = 1;
	std::optional<uint8_t> transparentColor;
	bool exportPalette = false;
};

using ClxCommand = std::variant<Cl2ToClxCommand, CelToClxCommand, PcxToClxCommand>;

struct ClxCommandAndFiles {
	ClxCommand command;
	std::vector<std::string> files;
};

template <typename IntT>
IntT ParseInt(
    std::string_view str, IntT min = std::numeric_limits<IntT>::min(),
    IntT max = std::numeric_limits<IntT>::max())
{
	IntT result;
	auto [ptr, ec] { std::from_chars(str.begin(), str.end(), result) };
	if (ec == std::errc::result_out_of_range || result < min || result > max) {
		std::cerr << "expected a number between " << min << " and " << max << ", got " << str << std::endl;
		std::exit(1);
	}
	if (ec != std::errc()) {
		std::cerr << "expected a number, got " << str << std::endl;
		std::exit(1);
	}
	return result;
}

template <typename IntT>
std::vector<IntT> ParseIntList(
    std::string_view str, IntT min = std::numeric_limits<IntT>::min(),
    IntT max = std::numeric_limits<IntT>::max())
{
	std::vector<IntT> result;
	std::string_view remaining = str;
	std::string_view::size_type commaPos;
	while (true) {
		commaPos = remaining.find(',');
		const std::string_view part = remaining.substr(0, commaPos);
		result.push_back(ParseInt(part, min, max));
		if (commaPos == std::string_view::npos)
			break;
		remaining.remove_prefix(commaPos + 1);
	}
	return result;
}

ClxCommandAndFiles
ParseCl2ToClxCommand(std::string_view line)
{
	Cl2ToClxCommand command;
	std::vector<std::string> files;
	while (!line.empty()) {
		const std::string_view arg = line.substr(0, line.find(' '));
		line.remove_prefix(std::min(arg.size() + 1, line.size()));
		if (arg.empty())
			continue;
		if (arg == "--width") {
			const std::string_view widths = line.substr(0, line.find(' '));
			line.remove_prefix(std::min(widths.size() + 1, line.size()));
			command.widths = ParseIntList<uint16_t>(widths);
		} else if (arg[0] == '-') {
			std::cerr << "Unknown argument: " << arg << std::endl;
			std::exit(1);
		} else {
			files.emplace_back(arg);
		}
	}
	return ClxCommandAndFiles { std::move(command), std::move(files) };
}

ClxCommandAndFiles
ParseCelToClxCommand(std::string_view line)
{
	CelToClxCommand command;
	std::vector<std::string> files;
	while (!line.empty()) {
		const std::string_view arg = line.substr(0, line.find(' '));
		line.remove_prefix(std::min(arg.size() + 1, line.size()));
		if (arg.empty())
			continue;
		if (arg == "--width") {
			const std::string_view widths = line.substr(0, line.find(' '));
			line.remove_prefix(std::min(widths.size() + 1, line.size()));
			command.widths = ParseIntList<uint16_t>(widths);
		} else if (arg[0] == '-') {
			std::cerr << "Unknown argument: " << arg << std::endl;
			std::exit(1);
		} else {
			files.emplace_back(arg);
		}
	}
	return ClxCommandAndFiles { std::move(command), std::move(files) };
}

ClxCommandAndFiles
ParsePcxToClxCommand(std::string_view line)
{
	PcxToClxCommand command;
	std::vector<std::string> files;
	while (!line.empty()) {
		const std::string_view arg = line.substr(0, line.find(' '));
		line.remove_prefix(std::min(arg.size() + 1, line.size()));
		if (arg.empty())
			continue;
		if (arg == "--num-sprites") {
			const std::string_view numStr = line.substr(0, line.find(' '));
			line.remove_prefix(std::min(numStr.size() + 1, line.size()));
			command.numFrames = ParseInt<size_t>(numStr);
		} else if (arg == "--transparent-color") {
			const std::string_view colorStr = line.substr(0, line.find(' '));
			line.remove_prefix(std::min(colorStr.size() + 1, line.size()));
			command.transparentColor = ParseInt<uint8_t>(colorStr);
		} else if (arg == "--export-palette") {
			command.exportPalette = true;
		} else if (arg[0] == '-') {
			std::cerr << "Unknown argument: " << arg << std::endl;
			std::exit(1);
		} else {
			files.emplace_back(arg);
		}
	}
	return ClxCommandAndFiles { std::move(command), std::move(files) };
}

std::optional<ClxCommandAndFiles>
ParseClxCommand(std::string_view line)
{
	const std::string_view arg = line.substr(0, line.find(' '));
	line.remove_prefix(std::min(arg.size() + 1, line.size()));
	if (arg.empty() || arg[0] == '#')
		return std::nullopt;
	if (arg == "cl22clx")
		return ParseCl2ToClxCommand(line);
	if (arg == "cel2clx")
		return ParseCelToClxCommand(line);
	if (arg == "pcx2clx")
		return ParsePcxToClxCommand(line);
	std::cerr << "Unknown command: " << arg << std::endl;
	std::exit(1);
}

struct string_hash {
	using is_transparent = void;
	[[nodiscard]] size_t operator()(const char *txt) const
	{
		return std::hash<std::string_view> {}(txt);
	}
	[[nodiscard]] size_t operator()(std::string_view txt) const
	{
		return std::hash<std::string_view> {}(txt);
	}
	[[nodiscard]] size_t operator()(const std::string &txt) const
	{
		return std::hash<std::string> {}(txt);
	}
};
template <typename T>
using HeterogenousUnorderedStringMap = std::unordered_map<std::string, T, string_hash, std::equal_to<>>;

HeterogenousUnorderedStringMap<ClxCommand>
ParseClxCommands(std::span<const char *const> clxCommands)
{
	HeterogenousUnorderedStringMap<ClxCommand> result;
	for (const std::string_view str : clxCommands) {
		std::optional<ClxCommandAndFiles> parsed = ParseClxCommand(str);
		if (!parsed.has_value())
			continue;
		for (std::string &file : parsed->files) {
			if (!result.emplace(std::move(file), parsed->command).second) {
				std::cerr << "More than 1 CLX conversion command for " << file << std::endl;
				std::exit(1);
			}
		}
	}
	return result;
}

void WriteOutput(const std::filesystem::path &outputPath, const uint8_t *data, size_t size)
{
	std::filesystem::create_directories(outputPath.parent_path());
	std::ofstream out { outputPath.c_str(), std::ios::binary };
	if (out.fail()) {
		std::cerr << "Failed to open " << outputPath << " for writing: " << std::strerror(errno) << std::endl;
	}
	out.write(reinterpret_cast<const char *>(data), static_cast<std::streamsize>(size));
	if (out.fail()) {
		std::cerr << "Failed to write " << outputPath << ": " << std::strerror(errno) << std::endl;
	}
	out.close();
	if (out.fail()) {
		std::cerr << "Failed to close " << outputPath << ": " << std::strerror(errno) << std::endl;
	}
};

class MpqArchive {
public:
	explicit MpqArchive(const std::filesystem::path &path)
	{
		const int32_t error = libmpq__archive_open(&archive_, path.c_str(), 0);
		if (error != 0) {
			std::cerr << "Failed to open MPQ at " << path << ": " << libmpq__strerror(error) << std::endl;
			std::exit(1);
		}
	}

	size_t ReadFile(const char *mpqPath, std::vector<uint8_t> &buf)
	{
		uint32_t mpqFileNumber;
		libmpq__off_t mpqFileSize;
		int32_t error;
		if ((error = libmpq__file_number(archive_, mpqPath, &mpqFileNumber)) != 0
		    || (error = libmpq__file_size_unpacked(archive_, mpqFileNumber, &mpqFileSize)) != 0) {
			std::cerr << "Failed to read MPQ file " << mpqPath << ": " << libmpq__strerror(error) << " " << mpqPath << std::endl;
			std::exit(1);
		}

		if (buf.size() < static_cast<size_t>(mpqFileSize)) {
			buf.resize(static_cast<size_t>(mpqFileSize));
			tmp_buf_.resize(static_cast<size_t>(mpqFileSize));
		}
		if ((error = libmpq__file_read_with_temporary_buffer(archive_, mpqFileNumber, buf.data(), mpqFileSize, tmp_buf_.data(), mpqFileSize, nullptr)) != 0) {
			std::cerr << "Failed to read MPQ file " << mpqPath << ": " << libmpq__strerror(error) << " " << mpqPath << std::endl;
			std::exit(1);
		}
		return static_cast<size_t>(mpqFileSize);
	}

	~MpqArchive()
	{
		const int32_t error = libmpq__archive_close(archive_);
		if (error != 0) {
			std::cerr << "Failed to close MPQ: " << libmpq__strerror(error) << std::endl;
			std::exit(1);
		}
	}

private:
	mpq_archive_s *archive_;
	std::vector<uint8_t> tmp_buf_;
};

void PrintStatus(std::string_view status, size_t i, size_t n)
{
	std::clog << "\r                                                           \r"
	          << "[" << i << "/" << n << "] " << status;
	std::clog.flush();
}

void Process(const std::filesystem::path &mpq, const std::filesystem::path &outputRoot)
{
	const std::string srcName = SrcName(mpq);
	const std::string destName = DestName(srcName);
	const std::filesystem::path outputDirectory = outputRoot / destName;

	std::clog << "Processing " << mpq << std::endl;
	MpqArchive archive { mpq };

	std::span<const char *const> mpqFiles = GetMpqFiles(srcName);

	std::vector<const char *> listfileEntries;
	std::vector<uint8_t> listfileData;
	if (mpqFiles.empty()) {
		const size_t listfileSize = archive.ReadFile("(listfile)", listfileData);
		std::replace(listfileData.begin(), listfileData.end(), static_cast<uint8_t>('\r'), static_cast<uint8_t>('\0'));
		std::replace(listfileData.begin(), listfileData.end(), static_cast<uint8_t>('\n'), static_cast<uint8_t>('\0'));
		std::string_view listfileStr { reinterpret_cast<char *>(listfileData.data()), listfileSize };
		while (!listfileStr.empty()) {
			const std::string_view str = listfileStr.substr(0, listfileStr.find('\0'));
			if (!str.empty())
				listfileEntries.push_back(str.data());
			listfileStr.remove_prefix(std::min(str.size() + 1, listfileStr.size()));
		}
		mpqFiles = listfileEntries;
	}

	std::span<const char *const> excludedFiles = GetExcludedFiles(srcName);
	std::unordered_set<std::string_view> excludedFilesMap { excludedFiles.begin(), excludedFiles.end() };

	std::span<const char *const> clxCommands = GetClxCommands(srcName);
	HeterogenousUnorderedStringMap<ClxCommand> clxCommandsMap = ParseClxCommands(clxCommands);

	std::vector<uint8_t> fileBuf;
	std::vector<uint8_t> clxData;
	size_t i = 0;
	for (const char *const mpqPath : mpqFiles) {
		++i;
		std::string mpqPathWithForwardSlash { mpqPath };
		std::replace(mpqPathWithForwardSlash.begin(), mpqPathWithForwardSlash.end(), '\\', '/');
		if (excludedFilesMap.contains(mpqPathWithForwardSlash)) {
			PrintStatus(std::string("Skipping ") + mpqPath, i, mpqFiles.size());
			continue;
		}
		const size_t mpqFileSize = archive.ReadFile(mpqPath, fileBuf);

		std::filesystem::path outputPath = outputDirectory / mpqPathWithForwardSlash;

		if (const auto it = clxCommandsMap.find(mpqPathWithForwardSlash); it != clxCommandsMap.end()) {
			PrintStatus(std::string("Converting ") + mpqPath + " to CLX", i, mpqFiles.size());
			outputPath.replace_extension(".clx");
			if (std::holds_alternative<Cl2ToClxCommand>(it->second)) {
				const Cl2ToClxCommand &command = std::get<Cl2ToClxCommand>(it->second);
				std::optional<dvl_gfx::IoError> clxError = dvl_gfx::Cl2ToClx(fileBuf.data(), mpqFileSize, command.widths.data(), command.widths.size());
				if (clxError.has_value()) {
					std::cerr << "Failed CL2->CLX conversion: " << clxError->message << " " << mpqPath << std::endl;
					std::exit(1);
				}
				WriteOutput(outputPath, fileBuf.data(), mpqFileSize);
			} else if (std::holds_alternative<CelToClxCommand>(it->second)) {
				const CelToClxCommand &command = std::get<CelToClxCommand>(it->second);
				clxData.clear();
				const std::optional<dvl_gfx::IoError> clxError = dvl_gfx::CelToClx(fileBuf.data(), mpqFileSize, command.widths.data(), command.widths.size(), clxData);
				if (clxError.has_value()) {
					std::cerr << "Failed CL2->CLX conversion: " << clxError->message << " " << mpqPath << std::endl;
					std::exit(1);
				}
				WriteOutput(outputPath, clxData.data(), clxData.size());
			} else if (std::holds_alternative<PcxToClxCommand>(it->second)) {
				const PcxToClxCommand &command = std::get<PcxToClxCommand>(it->second);
				clxData.clear();
				std::array<uint8_t, 256 * 3> paletteData;
				const std::optional<dvl_gfx::IoError> clxError = dvl_gfx::PcxToClx(fileBuf.data(), mpqFileSize, command.numFrames, command.transparentColor, clxData, command.exportPalette ? paletteData.data() : nullptr);
				if (clxError.has_value()) {
					std::cerr << "Failed CL2->CLX conversion: " << clxError->message << " " << mpqPath << std::endl;
					std::exit(1);
				}
				WriteOutput(outputPath, clxData.data(), clxData.size());
				if (command.exportPalette) {
					outputPath.replace_extension(".pal");
					WriteOutput(outputPath, paletteData.data(), paletteData.size());
				}
			} else {
				std::cerr << "Internal error" << std::endl;
				std::exit(1);
			}
		} else {
			PrintStatus(std::string("Extracting ") + mpqPath, i, mpqFiles.size());
			WriteOutput(outputPath, fileBuf.data(), mpqFileSize);
		}
	}
	std::clog << std::endl;
}

} // namespace

int main(int argc, char *argv[])
{
	bool mp3 = false;
	std::string outputRoot = ".";
	std::vector<std::filesystem::path> mpqs;
	for (int i = 1; i < argc; ++i) {
		const std::string_view arg = argv[i];
		if (arg == "-h" || arg == "--help") {
			PrintHelp();
			std::exit(0);
		}
		if (arg == "--mp3") {
			mp3 = true;
		} else if (arg == "--output-dir") {
			if (i + 1 == argc) {
				std::cerr << "--output-dir requires an argument" << std::endl;
				std::exit(64);
			}
			outputRoot = argv[++i];
		} else if (!arg.empty() && arg[0] != '-') {
			mpqs.emplace_back(arg);
		} else {
			std::cerr << "unknown argument: " << arg << std::endl;
		}
	}
	if (mpqs.empty()) {
		for (const std::filesystem::directory_entry &entry :
		    std::filesystem::directory_iterator(std::filesystem::current_path(), std::filesystem::directory_options::skip_permission_denied)) {
			if (entry.is_regular_file()
			    && (entry.path().extension() == ".mpq" || entry.path().extension() == ".MPQ")) {
				mpqs.emplace_back(entry.path());
			}
		}
	}
	if (mpqs.empty()) {
		std::cerr << "Error: No MPQs found in the current directory or in the command line\n\n";
		PrintHelp();
		std::exit(1);
	}
	for (const std::filesystem::path &mpq : mpqs) {
		Process(mpq, outputRoot);
	}
	return 0;
}
