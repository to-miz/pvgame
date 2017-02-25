namespace GameInputKeyFlags
{
enum Values : uint8 {
	WasDown   = BITFIELD( 7 ),
	Repeated  = BITFIELD( 6 ),
	CountMask = ( uint8 )( ~( WasDown | Repeated ) ),
};
}
struct GameInputKey {
	uint8 composite;
};
struct GameMouseInput {
	vec2 position;
	vec2 prev;

	// mouse position relative to window size
	vec2 relative;

	// how much the mouse moved since last frame
	vec2 delta;

	// how much the mouse moved since last frame relative to window size
	vec2 relativeDelta;

	// scalar mouse wheel delta
	float wheel;

	// tell the platform layer to lock mouse position
	bool locked;
};
struct GameInputs {
	GameInputKey keys[256];
	GameMouseInput mouse;

	// at most 16 characters per frame can be processed
	char chars[16];
	int32 count;

	bool disableEscapeForQuickExit;
};

StringView asStringView( GameInputs* inputs ) { return {inputs->chars, inputs->count}; }

inline bool isKeyPressed( GameInputs* inputs, int32 keycode )
{
	assert( keycode >= 0 && keycode <= KC_Count );
	auto key     = inputs->keys[keycode];
	auto wasDown = ( key.composite & GameInputKeyFlags::WasDown );
	auto count   = ( key.composite & GameInputKeyFlags::CountMask );
	return !wasDown && count;
}
inline bool isKeyReleased( GameInputs* inputs, int32 keycode )
{
	assert( keycode >= 0 && keycode <= KC_Count );
	auto key     = inputs->keys[keycode];
	auto wasDown = ( key.composite & GameInputKeyFlags::WasDown );
	auto count   = ( key.composite & GameInputKeyFlags::CountMask );
	return ( wasDown && ( count % 2 ) ) || ( !wasDown && count && !( count % 2 ) );
}
inline bool isKeyDown( GameInputKey key )
{
	auto wasDown = ( key.composite & GameInputKeyFlags::WasDown );
	auto count   = ( key.composite & GameInputKeyFlags::CountMask );
	return ( wasDown && !( count % 2 ) ) || ( !wasDown && ( count % 2 ) );
}
inline bool isKeyDown( GameInputs* inputs, int32 keycode )
{
	assert( keycode >= 0 && keycode <= KC_Count );
	auto key     = inputs->keys[keycode];
	auto wasDown = ( key.composite & GameInputKeyFlags::WasDown );
	auto count   = ( key.composite & GameInputKeyFlags::CountMask );
	return ( wasDown && !( count % 2 ) ) || ( !wasDown && ( count % 2 ) );
}
inline bool isKeyUp( GameInputs* inputs, int32 keycode ) { return !isKeyDown( inputs, keycode ); }

inline bool isKeyPressedRepeated( GameInputs* inputs, int32 keycode )
{
	assert( keycode >= 0 && keycode <= KC_Count );
	auto key      = inputs->keys[keycode];
	auto wasDown  = ( key.composite & GameInputKeyFlags::WasDown );
	auto count    = ( key.composite & GameInputKeyFlags::CountMask );
	auto repeated = ( key.composite & GameInputKeyFlags::Repeated );
	return ( !wasDown && count ) || repeated;
}

int32 getDownKeys( GameInputs* inputs, VirtualKeyEnumValues* buffer, int32 count )
{
	auto result = 0;
	for( auto i = 0; i < countof( inputs->keys ) && count > 0; ++i ) {
		if( isKeyDown( inputs->keys[i] ) ) {
			buffer[result++] = (VirtualKeyEnumValues)i;
			--count;
		}
	}
	return result;
}
void getDownKeys( GameInputs* inputs, UArray< VirtualKeyEnumValues >* buffer )
{
	assert( buffer );
	buffer->resize( getDownKeys( inputs, buffer->data(), buffer->capacity() ) );
}

const char* toVirtualKeyString( VirtualKeyEnumValues key )
{
	auto index = (int32)key;
	assert( index >= 0 && index < countof( VirtualKeyStrings ) );
	return VirtualKeyStrings[index];
}

bool isHotkeyPressed( GameInputs* inputs, int32 key, int32 modifier )
{
	return isKeyPressed( inputs, key ) && isKeyDown( inputs, modifier );
}
bool isHotkeyPressed( GameInputs* inputs, int32 key, int32 modifier1, int32 modifier2 )
{
	return isKeyPressed( inputs, key ) && isKeyDown( inputs, modifier1 )
	       && isKeyDown( inputs, modifier2 );
}

void resetInputs( GameInputs* inputs )
{
	for( auto& entry : inputs->keys ) {
		auto wasDown = isKeyDown( entry );
		if( wasDown ) {
			entry.composite = GameInputKeyFlags::WasDown;
		} else {
			entry.composite = 0;
		}
	}
	inputs->mouse.prev  = inputs->mouse.position;
	inputs->mouse.wheel = 0;
	inputs->count       = 0;
}