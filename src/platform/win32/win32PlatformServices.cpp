// textures

TextureId win32LoadTexture( StringView filename )
{
	TextureId result = {};
	assert( GlobalTextureMap );
	if( auto chached = getTextureInfo( filename ) ) {
		LOG( INFORMATION, "loaded chached texture {}", filename );
		result = chached->id;
	} else {
		auto image = loadImageToMemory( filename );
		bool freeImage = true;
		if( image ) {
			result = toTextureId( win32UploadImageToGpu( image ) );
			if( result ) {
				LOG( INFORMATION, "loaded texture {}", filename );
				if( GlobalTextureMap->entries.remaining() ) {
					auto entry = GlobalTextureMap->entries.emplace_back();
					entry->id = result;
					entry->width = (float)image.width;
					entry->height = (float)image.height;
					entry->filename = filename;
					entry->image = image;
					freeImage = false;
				} else {
					LOG( ERROR, "TextureMap is full" );
				}
			} else {
				LOG( ERROR, "failed to load texture {}", filename );
			}
		}
		if( freeImage ) {
			freeImageData( &image );
		}
	}
	return result;
}
TextureId win32LoadTextureFromMemory( ImageData image )
{
	TextureId result = toTextureId( win32UploadImageToGpu( image ) );
	if( result ) {
		LOG( INFORMATION, "loaded texture from memory" );
	} else {
		LOG( ERROR, "failed to load texture from memory" );
	}
	return result;
}
void win32DeleteTexture( TextureId id )
{
	auto oglId = toOpenGlTextureId( id );
	// glBindTexture( GL_TEXTURE_2D, 0 ); // TODO: is this needed?
	glDeleteTextures( 1, &oglId );
	auto info = getTextureInfo( id );
	freeImageData( &info->image );
	deleteTextureInfo( id );
}

// get open/save filename

int32 win32DoubleNullterminatedStringLengthInclusiveNull( const char* str )
{
	if( !str ) {
		return 0;
	}
	int32 result = 0;
	for( ;; ) {
		if( *str == 0 && ( *( str + 1 ) == 0 ) ) {
			result += 2;
			break;
		}
		++result;
		++str;
	}
	return result;
}

int32 win32OpenFilenameInternal( const char* filter, const char* initialDir, bool multiselect,
                                 char* filenameBuffer, int32 filenameBufferSize, bool openFilename )
{
	int32 result = 0;
	assert( filenameBuffer );
	assert( filenameBufferSize > 0 );

	auto wFilter =
	    WString::fromUtf8( {filter, win32DoubleNullterminatedStringLengthInclusiveNull( filter )} );
	wchar_t* wInitialDir = nullptr;
	WString wInitialDirStorage;
	if( initialDir ) {
		wInitialDirStorage = WString::fromUtf8( initialDir );
		wInitialDir        = wInitialDirStorage;
	}

	// if we pass nullptr into GetCurrentDirectoryW, it returns the required size including
	// nullterminator
	DWORD currentDirSize = GetCurrentDirectoryW( 0, nullptr );
	wchar_t* currentDir  = new wchar_t[currentDirSize];
	// but if we pass in a pointer, it returns the size excluding the nullterminator
	if( ( GetCurrentDirectoryW( currentDirSize, currentDir ) + 1 ) != currentDirSize ) {
		LOG( ERROR, "Win32 internal error, GetCurrentDirectoryW failed" );
		// TODO: do we halt here?
	}

	wchar_t internalBuffer[260];
	internalBuffer[0] = 0;
	OPENFILENAMEW ofn;
	zeroMemory( &ofn, 1 );
	ofn.lStructSize     = sizeof( ofn );
	ofn.hwndOwner       = Win32AppContext.window.hwnd;
	ofn.lpstrFile       = internalBuffer;
	ofn.nMaxFile        = (DWORD)countof( internalBuffer );
	ofn.lpstrFilter     = wFilter;
	ofn.nFilterIndex    = 0;  // currently selected filter
	ofn.lpstrInitialDir = wInitialDir;
	ofn.Flags           = OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
	if( openFilename ) {
		ofn.Flags |= OFN_FILEMUSTEXIST;
		if( multiselect ) {
			ofn.Flags |= OFN_ALLOWMULTISELECT;
		}
	} else {
		ofn.Flags |= OFN_OVERWRITEPROMPT;
	}

	bool dialogResult = false;
	if( openFilename ) {
		dialogResult = ( GetOpenFileNameW( &ofn ) == TRUE );
	} else {
		dialogResult = ( GetSaveFileNameW( &ofn ) == TRUE );
	}
	if( dialogResult ) {
		static_assert(
		    sizeof( wchar_t ) == sizeof( uint16 ) && alignof( wchar_t ) == alignof( uint16 ),
		    "Can't cast wchar_t to uint16 like this" );

		result = utf8::convertUtf16ToUtf8( (uint16*)ofn.lpstrFile,
		                                   safe_truncate< int32 >( wcslen( ofn.lpstrFile ) ),
		                                   filenameBuffer, filenameBufferSize );
	}

	if( !SetCurrentDirectoryW( currentDir ) ) {
		LOG( ERROR, "Win32 internal error, SetCurrentDirectoryW failed" );
		// TODO: do we halt here?
	}

#if _DEBUG
	// paranoid
	{
		DWORD newDirSize = GetCurrentDirectoryW( 0, nullptr );
		wchar_t* newDir  = new wchar_t[newDirSize];
		GetCurrentDirectoryW( newDirSize, newDir );
		if( newDirSize != currentDirSize || memcmp( currentDir, newDir, newDirSize ) != 0 ) {
			LOG( ERROR, "Current directory changes after GetOpenFileNameW" );
		}
		delete[] newDir;
	}
#endif

	delete[] currentDir;
	return result;
}

int32 win32GetOpenFilename( const char* filter, const char* initialDir, bool multiselect,
                            char* filenameBuffer, int32 filenameBufferSize )
{
	return win32OpenFilenameInternal( filter, initialDir, false, filenameBuffer, filenameBufferSize,
	                                  true );
}

int32 win32GetSaveFilename( const char* filter, const char* initialDir, char* filenameBuffer,
                            int32 filenameBufferSize )
{
	return win32OpenFilenameInternal( filter, initialDir, false, filenameBuffer, filenameBufferSize,
	                                  false );
}

struct win32KeyboardKeyName {
	char data[20];
	int32 size;
};
global win32KeyboardKeyName keyboardKeyNames[KC_Count];

void win32PopulateKeyboardKeyNames()
{
	for( auto i = 0; i < KC_Count; ++i ) {
		wchar_t name[200];
		auto scancode = ( MapVirtualKey( (UINT)i, MAPVK_VK_TO_VSC ) << 16 );
		auto len      = GetKeyNameTextW( scancode, name, countof( name ) );
		auto entry    = &keyboardKeyNames[i];
		entry->size   = utf8::convertUtf16ToUtf8( name, len, entry->data, countof( entry->data ) );
	}
}
StringView win32GetKeyboardKeyName( VirtualKeyEnumValues key )
{
	auto entry = &keyboardKeyNames[(int32)key];
	return {entry->data, entry->size};
}