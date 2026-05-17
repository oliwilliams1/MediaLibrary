#include <MediaLibrary/types.h>
#include <MediaLibrary/MediaLibrary.h>

using namespace SableML;

bool TrackMetadata::isEmpty() const
{
	return title.empty() && artist.empty() && album.empty() &&
		trackNumber == 0 && year == 0;
}
bool AudioProperties::isLossless() const
{
	return format == Format::FLAC || format == Format::WAV || format == Format::AIFF;
}

std::string AudioProperties::formatString() const
{
	switch (format) {
	case Format::MP3:  return "MP3";
	case Format::FLAC: return "FLAC";
	case Format::AAC:  return "AAC";
	case Format::M4A:  return "M4A";
	case Format::OGG:  return "OGG Vorbis";
	case Format::OPUS: return "Opus";
	case Format::WAV:  return "WAV";
	case Format::AIFF: return "AIFF";
	case Format::WMA:  return "WMA";
	default:           return "Unknown";
	}
}

bool FileEntry::exists() const
{
	return fs::exists(path);
}

bool FileEntry::needsRescan() const
{
	if (!exists()) return false;

	try
	{
		auto currentWriteTime = fs::last_write_time(path);
		auto currentSize = fs::file_size(path);

		return currentWriteTime != lastModified || currentSize != fileSize;
	}
	catch (...)
	{
		return true;
	}
}

bool Track::isValid() const
{
	return id != INVALID_TRACK_ID && !fileIds.empty();
}

const FileEntry* Track::getPrimaryFile(const Library& lib) const
{
	return lib.getFile(primaryFileId);
}