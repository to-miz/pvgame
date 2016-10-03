enum class ReadWholeFileErrorType {
	Ok,
	FileNotFound,
	IOError,
	FileTooBig
};
struct FileContents {
	char* data;
	size_t size;
	ReadWholeFileErrorType error;

	inline explicit operator bool() const { return error == ReadWholeFileErrorType::Ok; }
};
FileContents win32ReadWholeFile( StringView filename, char* buffer, size_t bufferSize )
{
	FileContents result = {};
	auto wfilename      = WString::fromUtf8( filename );
	auto file = CreateFileW( wfilename, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
	                         FILE_ATTRIBUTE_NORMAL, nullptr );
	if( file == INVALID_HANDLE_VALUE ) {
		LOG( ERROR, "Failed to open file: {}", filename );
		result.error = ReadWholeFileErrorType::FileNotFound;
	} else {
		LARGE_INTEGER size;
		if( !GetFileSizeEx( file, &size ) ) {
			LOG( ERROR, "Unknown IO error: {}", filename );
			result.error = ReadWholeFileErrorType::IOError;
		} else if( size.QuadPart > UINT32_MAX || (size_t)size.QuadPart > bufferSize ) {
			LOG( ERROR, "File too big: {}", filename );
			result.error = ReadWholeFileErrorType::FileTooBig;
		} else {
			auto bytesToRead    = (DWORD)size.QuadPart;
			DWORD bytesRead     = 0;
			auto readFileResult = ReadFile( file, buffer, bytesToRead, &bytesRead, nullptr );
			if( !readFileResult || bytesRead != bytesToRead ) {
				LOG( ERROR, "Unknown IO error: {}", filename );
				result.error = ReadWholeFileErrorType::IOError;
			} else {
				result.data = buffer;
				result.size = bytesToRead;
			}
		}
		CloseHandle( file );
	}
	return result;
}

struct AllocatedFileContents {
	char* data = nullptr;
	size_t size = 0;
	ReadWholeFileErrorType error = ReadWholeFileErrorType::Ok;

	void allocate( size_t size )
	{
		data       = new char[size];
		this->size = size;
	}
	void destroy()
	{
		delete[] data;
		data = nullptr;
		size = 0;
	}
	AllocatedFileContents() = default;
	AllocatedFileContents( AllocatedFileContents&& other ) : data( other.data ), size( other.size )
	{
		other.data = nullptr;
	}
	~AllocatedFileContents() { destroy(); }
	inline explicit operator bool() const { return error == ReadWholeFileErrorType::Ok; }
};
AllocatedFileContents win32ReadWholeFileInternal( StringView filename )
{
	AllocatedFileContents result = {};
	auto wfilename               = WString::fromUtf8( filename );
	auto file = CreateFileW( wfilename, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
	                         FILE_ATTRIBUTE_NORMAL, nullptr );
	if( file == INVALID_HANDLE_VALUE ) {
		LOG( ERROR, "Failed to open file: {}", filename );
		result.error = ReadWholeFileErrorType::FileNotFound;
	} else {
		LARGE_INTEGER size;
		if( !GetFileSizeEx( file, &size ) ) {
			LOG( ERROR, "Unknown IO error: {}", filename );
			result.error = ReadWholeFileErrorType::IOError;
		} else if( size.QuadPart > UINT32_MAX ) {
			LOG( ERROR, "File too big: {}", filename );
			result.error = ReadWholeFileErrorType::FileTooBig;
		} else {
			auto bytesToRead = (DWORD)size.QuadPart;
			result.allocate( bytesToRead );
			DWORD bytesRead     = 0;
			auto readFileResult = ReadFile( file, result.data, bytesToRead, &bytesRead, nullptr );
			if( !readFileResult || bytesRead != bytesToRead ) {
				LOG( ERROR, "Unknown IO error: {}", filename );
				result.error = ReadWholeFileErrorType::IOError;
				result.destroy();
			}
		}
		CloseHandle( file );
	}
	return result;
}
size_t win32ReadFileToBuffer( StringView filename, void* buffer, size_t bufferSize )
{
	auto result = win32ReadWholeFile( filename, (char*)buffer, bufferSize );
	return result.size;
}

void win32WriteBufferToFile( StringView filename, void* buffer, size_t bufferSize )
{
	auto wfilename = WString::fromUtf8( filename );
	auto file      = CreateFileW( wfilename, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
	                         FILE_ATTRIBUTE_NORMAL, nullptr );
	if( file == INVALID_HANDLE_VALUE ) {
		LOG( ERROR, "Failed to open file: {}", filename );
	} else {
		DWORD bytesWritten;
		DWORD truncatedBufferSize = (DWORD)bufferSize;
		if( bufferSize > UINT32_MAX ) {
			LOG( ERROR, "bufferSize too big to write to file {}, truncating", filename );
		}
		if( !WriteFile( file, buffer, truncatedBufferSize, &bytesWritten, nullptr ) ) {
			LOG( ERROR, "Failed to write to file: {}", filename );
		}
		CloseHandle( file );
	}
}