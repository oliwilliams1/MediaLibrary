#pragma once
#include <MediaLibrary/types.h>
#include <filesystem>
#include <unordered_map>
#include <optional>
#include <vector>

namespace SableML
{
	namespace fs = std::filesystem;

	class Library
	{
	public:
		Library() = default;
		~Library() = default;

		Library(const Library&) = delete;
		Library& operator=(const Library&) = delete;

		static void Initialise();
		static void Shutdown();
		static Library& GetInstance();

		void ScanDirectory(const fs::path& path, bool recursive = true);

		bool save(const fs::path& path) const;
		bool load(const fs::path& path);

		const std::vector<Track>* GetTracks() const;

		const FileEntry* getFile(FileID id) const;
		Track* getTrackMutable(TrackID id);

	private:
		std::vector<Track> m_tracks;
		std::vector<FileEntry> m_files;

		std::unordered_map<TrackID, size_t> m_trackIndex;
		std::unordered_map<FileID, size_t> m_fileIndex;
		std::unordered_map<fs::path, FileID> m_pathToFile;
		std::unordered_map<AudioID, TrackID> m_audioToTrack;
		std::unordered_map<FileID, TrackID> m_fileToTrack;

		std::optional<TrackMetadata> readMetadata(const fs::path& path);
		std::optional<AudioProperties> readProperties(const fs::path& path);

		TrackID m_nextTrackId = 1;
		FileID m_nextFileId = 1;

		bool m_enableAudioHash = false;

		AudioID computeAudioHash(const fs::path& path);

		TrackID createTrack(AudioID audioId, const TrackMetadata& metadata,
			const AudioProperties& properties, FileID primaryFile);
		FileID createFile(const fs::path& path, AudioID audioId,
			const AudioProperties& properties);
	};
}