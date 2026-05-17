#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <chrono>
#include <filesystem>
#include <cstdint>

namespace SableML
{
	using AudioID = uint64_t;
	using FileID = uint32_t;
	using TrackID = uint32_t;
	
	constexpr AudioID INVALID_AUDIO_ID = 0;
	constexpr FileID INVALID_FILE_ID = 0;
	constexpr TrackID INVALID_TRACK_ID = 0;

	struct TrackMetadata {
		std::string title;
		std::string artist;
		std::string album;
		std::string albumArtist;
		std::string genre;
		std::string composer;
		std::string comment;

		uint32_t trackNumber = 0;
		uint32_t trackTotal = 0;
		uint32_t discNumber = 0;
		uint32_t discTotal = 0;
		uint32_t year = 0;

		std::unordered_map<std::string, std::string> customTags;

		bool isEmpty() const;
	};

	struct AudioProperties {
		uint32_t sampleRate = 0;
		uint16_t channels = 0;
		uint32_t bitrate = 0;
		uint32_t durationMs = 0;
		uint16_t bitsPerSample = 0;

		enum class Format {
			Unknown,
			MP3,
			FLAC,
			AAC,
			M4A,
			OGG,
			OPUS,
			WAV,
			AIFF,
			WMA
		};
		Format format = Format::Unknown;

		bool isLossless() const;
		std::string formatString() const;
	};

	struct FileEntry {
		FileID id = INVALID_FILE_ID;
		std::filesystem::path path;
		uint64_t fileSize = 0;
		std::filesystem::file_time_type lastModified;

		AudioID audioId = INVALID_AUDIO_ID;
		AudioProperties properties;

		bool exists() const;
		bool needsRescan() const;
	};

	struct Track {
		TrackID id = INVALID_TRACK_ID;
		AudioID audioId = INVALID_AUDIO_ID;

		TrackMetadata metadata;
		AudioProperties properties;

		std::vector<FileID> fileIds;
		FileID primaryFileId = INVALID_FILE_ID;

		std::chrono::system_clock::time_point dateAdded;
		uint32_t playCount = 0;
		std::chrono::system_clock::time_point lastPlayed;

		bool isValid() const;
		const FileEntry* getPrimaryFile(const class Library& lib) const;
	};
}
