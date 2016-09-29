namespace GameInputKeyFlags
{
enum Values : uint8 {
	WasDown   = ( 1 << 7 ),
	CountMask = 0x7F,  // every bit set except the WasDown bit
};
}
struct GameInputKey {
	uint8 composite;
};
struct GameMouseInput {
	union {
		vec2 position;
		struct {
			float x;
			float y;
		};
	};
	vec2 prev;

	// mouse position relative to window size
	vec2 relative;

	// how much the mouse moved since last frame
	vec2 delta;

	// how much the mouse moved since last frame relative to window size
	vec2 relativeDelta;

	// tell the platform layer to lock mouse position
	bool locked;
};
struct GameInputs {
	GameInputKey keys[256];
	GameMouseInput mouse;

	// at most 16 characters per frame can be processed
	char chars[16];
	int32 count;
};

StringView asStringView( GameInputs* inputs )
{
	return {inputs->chars, inputs->count};
}

inline bool isKeyPressed( GameInputs* inputs, intmax keycode )
{
	assert( keycode >= 0 && keycode <= KC_Count );
	auto key     = inputs->keys[keycode];
	auto wasDown = ( key.composite & GameInputKeyFlags::WasDown );
	auto count   = ( key.composite & GameInputKeyFlags::CountMask );
	return !wasDown && count;
}
inline bool isKeyReleased( GameInputs* inputs, intmax keycode )
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
inline bool isKeyDown( GameInputs* inputs, intmax keycode )
{
	assert( keycode >= 0 && keycode <= KC_Count );
	auto key     = inputs->keys[keycode];
	auto wasDown = ( key.composite & GameInputKeyFlags::WasDown );
	auto count   = ( key.composite & GameInputKeyFlags::CountMask );
	return ( wasDown && !( count % 2 ) ) || ( !wasDown && ( count % 2 ) );
}
inline bool isKeyUp( GameInputs* inputs, intmax keycode ) { return !isKeyDown( inputs, keycode ); }

inline bool isKeyPressedRepeated( GameInputs* inputs, intmax keycode )
{
	// TODO: implement properly
	return isKeyPressed( inputs, keycode );
}