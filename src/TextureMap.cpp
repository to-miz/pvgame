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
	auto result = find_first_where( GlobalTextureMap->entries, it.id == id );
	assert( result );
	return result;
}