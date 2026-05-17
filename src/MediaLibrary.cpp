#include <MediaLibrary/MediaLibrary.h>
#include <MediaLibrary/types.h>

#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/audioproperties.h>
#include <taglib/toolkit/tpropertymap.h>

#include <cstdlib>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

using namespace SableML;

static const std::vector<std::string> SUPPORTED_FORMATS = { ".mp3", ".flac", ".m4a", ".aac", ".ogg", ".opus", ".wav", ".aiff", ".wma" };

static bool isSupportedFile(const fs::path& path)
{
	std::string ext = path.extension().string();
	std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

	return std::find(SUPPORTED_FORMATS.begin(), SUPPORTED_FORMATS.end(), ext)
		!= SUPPORTED_FORMATS.end();
}

void Library::ScanDirectory(const fs::path& path, bool recursive)
{
	if (!fs::exists(path))
	{
		printf("[MEDIA LIBRARY] SCAN ERROR: Path does not exist");
		return;
	}

	std::vector<fs::path> filesToScan;
	
	try
	{
		if (recursive)
		{
			for (const auto& entry : fs::recursive_directory_iterator(path))
				if (entry.is_regular_file() && isSupportedFile(entry.path()))
					filesToScan.push_back(entry.path());
		}
		else
		{
			for (const auto& entry : fs::directory_iterator(path))
				if (entry.is_regular_file() && isSupportedFile(entry.path()))
					filesToScan.push_back(entry.path());
		}
	}
	catch (const fs::filesystem_error& e)
	{
		printf("[MEDIA LIBRARY] SCAN ERROR: %s", e.what());
		return;
	}

	for (const auto& filePath : filesToScan)
	{
		auto pathIt = m_pathToFile.find(filePath);
		bool fileExists = pathIt != m_pathToFile.end();

		if (fileExists)
		{
			FileID fileId = pathIt->second;
			auto fileIt = m_fileIndex.find(fileId);
			if (fileIt != m_fileIndex.end())
			{
				FileEntry& existingFile = m_files[fileIt->second];

				if (existingFile.needsRescan())
				{
					auto metadata = readMetadata(filePath);
					auto properties = readProperties(filePath);

					if (metadata && properties)
					{
						existingFile.fileSize = fs::file_size(filePath);
						existingFile.lastModified = fs::last_write_time(filePath);
						existingFile.properties = *properties;

						auto trackIt = m_fileToTrack.find(fileId);
						if (trackIt != m_fileToTrack.end())
						{
							Track* track = getTrackMutable(trackIt->second);
							if (track)
							{
								track->metadata = *metadata;
								track->properties = *properties;
							}
						}
					}
				}
			}
		}
		else
		{
			auto metadata = readMetadata(filePath);
			auto props = readProperties(filePath);

			if (!metadata || !props) continue;

			AudioID audioId = m_enableAudioHash ?
				computeAudioHash(filePath) :
				static_cast<AudioID>(m_nextFileId);

			auto audioIt = m_audioToTrack.find(audioId);
			if (audioIt != m_audioToTrack.end())
			{
				TrackID trackId = audioIt->second;
				Track* track = getTrackMutable(trackId);

				if (track) {
					FileID newFileId = createFile(filePath, audioId, *props);
					track->fileIds.push_back(newFileId);

					if (const FileEntry* currentPrimary = track->getPrimaryFile(*this))
					{
						if (props->bitrate > currentPrimary->properties.bitrate ||
							props->isLossless())
						{
							track->primaryFileId = newFileId;
							track->properties = *props;
						}
					}
				}
			}
			else
			{
				FileID newFileId = createFile(filePath, audioId, *props);
				TrackID newTrackId = createTrack(audioId, *metadata, *props, newFileId);
			}
		}
	}
}

std::optional<TrackMetadata> Library::readMetadata(const fs::path& path)
{
	TagLib::FileRef fileRef(path.native().c_str());

	if (fileRef.isNull() || !fileRef.tag())
	{
		return std::nullopt;
	}

	TagLib::Tag* tag = fileRef.tag();
	TrackMetadata metadata;

	metadata.title = tag->title().to8Bit(true);
	metadata.artist = tag->artist().to8Bit(true);
	metadata.album = tag->album().to8Bit(true);
	metadata.genre = tag->genre().to8Bit(true);
	metadata.comment = tag->comment().to8Bit(true);
	metadata.year = tag->year();
	metadata.trackNumber = tag->track();

	TagLib::PropertyMap props = fileRef.file()->properties();

	if (props.contains("ALBUMARTIST") && !props["ALBUMARTIST"].isEmpty())
		metadata.albumArtist = props["ALBUMARTIST"].front().to8Bit(true);

	if (props.contains("COMPOSER") && !props["COMPOSER"].isEmpty())
		metadata.composer = props["COMPOSER"].front().to8Bit(true);

	if (props.contains("DISCNUMBER") && !props["DISCNUMBER"].isEmpty())
	{
		std::string discStr = props["DISCNUMBER"].front().to8Bit(true);
		metadata.discNumber = std::atoi(discStr.c_str());
	}

	return metadata;
}

std::optional<AudioProperties> Library::readProperties(const fs::path& path)
{
	TagLib::FileRef fileRef(path.native().c_str());

	if (fileRef.isNull() || !fileRef.audioProperties())
		return std::nullopt;

	TagLib::AudioProperties* audioProps = fileRef.audioProperties();
	AudioProperties props;

	props.sampleRate = audioProps->sampleRate();
	props.channels = audioProps->channels();
	props.bitrate = audioProps->bitrate();
	props.durationMs = audioProps->lengthInMilliseconds();

	std::string ext = path.extension().string();
	std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

	if (ext == ".mp3")			props.format = AudioProperties::Format::MP3;
	else if (ext == ".flac")	props.format = AudioProperties::Format::FLAC;
	else if (ext == ".m4a")		props.format = AudioProperties::Format::M4A;
	else if (ext == ".aac")		props.format = AudioProperties::Format::AAC;
	else if (ext == ".ogg")		props.format = AudioProperties::Format::OGG;
	else if (ext == ".opus")	props.format = AudioProperties::Format::OPUS;
	else if (ext == ".wav")		props.format = AudioProperties::Format::WAV;
	else if (ext == ".aiff")	props.format = AudioProperties::Format::AIFF;
	else if (ext == ".wma")		props.format = AudioProperties::Format::WMA;

	return props;
}

const FileEntry* Library::getFile(FileID id) const
{
	auto it = m_fileIndex.find(id);
	if (it != m_fileIndex.end() && it->second < m_files.size())
		return &m_files[it->second];

	return nullptr;
}

Track* Library::getTrackMutable(TrackID id)
{
	auto it = m_trackIndex.find(id);
	if (it != m_trackIndex.end() && it->second < m_tracks.size())
		return &m_tracks[it->second];

	return nullptr;
}

AudioID Library::computeAudioHash(const fs::path& path)
{
	// placeholder
	std::hash<std::string> hasher;
	return hasher(path.string());
}

TrackID Library::createTrack(AudioID audioId, const TrackMetadata& metadata,
	const AudioProperties& properties, FileID primaryFile)
{
	Track track;
	track.id = m_nextTrackId++;
	track.audioId = audioId;
	track.metadata = metadata;
	track.properties = properties;
	track.primaryFileId = primaryFile;
	track.fileIds.push_back(primaryFile);
	track.dateAdded = std::chrono::system_clock::now();

	m_trackIndex[track.id] = m_tracks.size();
	m_audioToTrack[audioId] = track.id;
	m_fileToTrack[primaryFile] = track.id;

	m_tracks.push_back(track);

	return track.id;
}

FileID Library::createFile(const fs::path& path, AudioID audioId,
	const AudioProperties& properties)
{
	FileEntry file;
	file.id = m_nextFileId++;
	file.path = path;
	file.audioId = audioId;
	file.properties = properties;

	try
	{
		file.fileSize = fs::file_size(path);
		file.lastModified = fs::last_write_time(path);
	}
	catch (...)
	{
		file.fileSize = 0;
	}

	m_fileIndex[file.id] = m_files.size();
	m_pathToFile[path] = file.id;

	m_files.push_back(file);

	return file.id;
}