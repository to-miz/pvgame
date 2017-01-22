struct TextureMapEntry {
	TextureId id;
	union {
		vec2 dim;
		struct {
			float width;
			float height;
		};
	};
	StringView filename;
	ImageData image;
};

struct TextureMap {
	UArray< TextureMapEntry > entries;
};

extern global TextureMap* GlobalTextureMap;

TextureMapEntry* getTextureInfo( TextureId id )
{
	assert( GlobalTextureMap );
	auto result = find_first_where( GlobalTextureMap->entries, entry.id == id );
	assert( result );
	return result;
}

TextureMapEntry* getTextureInfo( StringView filename )
{
	assert( GlobalTextureMap );
	return find_first_where( GlobalTextureMap->entries, entry.filename == filename );
}
void deleteTextureInfo( TextureId id )
{
	GlobalTextureMap->entries.erase( getTextureInfo( id ) );
}