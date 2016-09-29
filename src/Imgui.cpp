/*
TODO:
    - when fading out, the gui still accepts input, needs a way to toggle input processing
    - when dragging a group, the titlebar lags behind
*/

union ImGuiHandle {
	struct {
		uint32 base;
		uint16 index;
		int8 group;
		uint8 flags;
	};
	uint64 bits;

	inline explicit operator bool() const { return bits != 0; }
};
static_assert( sizeof( ImGuiHandle ) == sizeof( uint64 ), "ImGuiHandle is invalid" );
inline bool operator==( ImGuiHandle a, ImGuiHandle b ) { return a.bits == b.bits; }
inline bool operator!=( ImGuiHandle a, ImGuiHandle b ) { return a.bits != b.bits; }
namespace ImGuiHandleFlags
{
enum Values : uint8 {
	Unbased  = ( 1 << 0 ),
	Internal = ( 1 << 1 ),
};
}

namespace ImGuiTexCoords
{
enum Values : int32 {
	RadioboxUnchecked,
	RadioboxChecked,
	CheckboxUnchecked,
	CheckboxChecked,

	SliderKnob,

	Count
};
}

struct ImGuiStyle {
	float innerPadding;
	float outerPadding;
	float buttonWidth;
	float buttonHeight;

	float editboxWidth;
	float editboxHeight;

	float groupWidth;

	Color buttonBg;
	Color buttonText;
	Color text;
	Color editboxBg;
	Color editboxText;
	Color editboxTextSelectedBg;
	Color editboxTextSelected;
	Color editboxCaret;

	Color name;

	TextureId atlas;
	rectf texCoords[ImGuiTexCoords::Count];
	rectf rects[ImGuiTexCoords::Count];
};
ImGuiStyle defaultImGuiStyle()
{
	ImGuiStyle result = {};

	result.innerPadding = 2;
	result.outerPadding = 1;
	result.buttonWidth  = 88;
	result.buttonHeight = 18;

	result.editboxWidth  = 88;
	result.editboxHeight = 18;

	result.groupWidth = 200;

	result.buttonBg   = Color::White;
	result.buttonText = Color::Black;
	result.text       = Color::White;

	result.editboxBg             = Color::White;
	result.editboxText           = Color::Black;
	result.editboxTextSelectedBg = setAlpha( Color::Blue, 0x80 );
	result.editboxTextSelected   = Color::Red;
	result.editboxCaret          = Color::Black;

	result.name = Color::White;

	return result;
}

struct ImGuiGroupState {
	rectf rect;
	vec2 addPosition;
	int32 horizontalCount; // how many controls to place horizontally instead of vertically
	rectf lastRect;
	int8 z;
	bool8 minimized;
	bool8 hidden;
	bool8 dragging;

	ImGuiHandle checkedRadiobox;
};
struct ImGuiEditboxState {
	int32 selectionBegin;
	int32 selectionEnd;
	float textOffset;

	float timePassed;
	bool hideCaret;

	int32 textCount;

	inline rangei selection()
	{
		if( selectionBegin < selectionEnd ) {
			return {selectionBegin, selectionEnd};
		} else {
			return {selectionEnd, selectionBegin};
		}
	}
	inline void resetCaret()
	{
		timePassed = 0;
		hideCaret  = false;
	}
};

ImGuiGroupState defaultImGuiGroupSate( rectfarg rect );
#define ImGuiMaxGroups 8

enum class ImGuiStateType {
	None,
	Editbox,
	Slider,
};

struct ImGuiSliderState {
	rectf knobRect;
	bool dragging;
};

struct ImmediateModeGui {
	RenderCommands* renderer;
	GameInputs* inputs;
	Font* font;
	void* base;
	// gui bounds
	rectf bounds;
	ImGuiStyle style;

	int8 group;         // current group
	int8 hoverGroup;    // the group the mouse is currently over
	int8 captureGroup;  // the group that has the mouse currently captured
	ImGuiHandle focus;
	vec2 mouseOffset;  // offset of mouse when dragging to the leftTop position of the group

	char editboxStatic[100];
	int32 editboxStaticCount;
	ImGuiStateType stateType;
	union {
		ImGuiEditboxState editbox;
		ImGuiSliderState slider;
	} state;
	float caretBlinkTime;

	int32 groupsCount;
	ImGuiGroupState groups[ImGuiMaxGroups];

	RenderCommandJump* renderCommandJumpFirst;
	RenderCommandJump* renderCommandJumpLast;
};
ImmediateModeGui defaultImmediateModeGui()
{
	ImmediateModeGui result = {};
	result.style            = defaultImGuiStyle();
	result.caretBlinkTime   = 15;
	return result;
}
extern global ImmediateModeGui* ImGui;

void imguiLoadDefaultStyle( PlatformServices* platform )
{
	ImGui->style.atlas = platform->loadTexture( "Data/Images/gui_atlas.png" );
	auto itw           = 1.0f / 100.0f;
	auto ith           = 1.0f / 100.0f;
	ImGui->style.texCoords[ImGuiTexCoords::RadioboxUnchecked] =
	    RectWH( 0.0f, 0.0f, 13 * itw, 13 * ith );
	ImGui->style.texCoords[ImGuiTexCoords::RadioboxChecked] =
	    RectWH( 14 * itw, 0.0f, 13 * itw, 13 * ith );
	ImGui->style.texCoords[ImGuiTexCoords::CheckboxUnchecked] =
	    RectWH( 0.0f, 14 * ith, 13 * itw, 13 * ith );
	ImGui->style.texCoords[ImGuiTexCoords::CheckboxChecked] =
	    RectWH( 14 * itw, 14 * ith, 13 * itw, 13 * ith );

	ImGui->style.texCoords[ImGuiTexCoords::SliderKnob] =
	    RectWH( 0.0f, 28 * ith, 13 * itw, 13 * ith );

	ImGui->style.rects[ImGuiTexCoords::RadioboxUnchecked] = {0, 0, 13, 13};
	ImGui->style.rects[ImGuiTexCoords::RadioboxChecked]   = {0, 0, 13, 13};
	ImGui->style.rects[ImGuiTexCoords::CheckboxUnchecked] = {0, 0, 13, 13};
	ImGui->style.rects[ImGuiTexCoords::CheckboxChecked]   = {0, 0, 13, 13};
	ImGui->style.rects[ImGuiTexCoords::SliderKnob]        = {0, 0, 13, 13};
}

ImGuiGroupState defaultImGuiGroupSate( rectfarg rect )
{
	return {RectWH( rect.left, rect.top, ImGui->style.groupWidth, 0.0f )};
}
int32 imguiGenerateGroup( rectfarg rect = {} )
{
	assert( ImGui->groupsCount < ImGuiMaxGroups );
	auto result = ImGui->groupsCount;
	auto group  = &ImGui->groups[ImGui->groupsCount];
	*group      = defaultImGuiGroupSate( {} );
	group->rect = rect;
	group->z    = safe_truncate< int8 >( ImGui->groupsCount );
	++ImGui->groupsCount;
	return result;
}
ImGuiGroupState* imguiGetGroup( int32 index )
{
	assert( index >= 0 && index < countof( ImGui->groups ) );
	return &ImGui->groups[index];
}

rectf imguiGetTitlebar( rectfarg bounds )
{
	auto result   = bounds;
	result.bottom = result.top + stringHeight( ImGui->font ) + ImGui->style.innerPadding * 2;
	return result;
}
rectf imguiGetDraggableRect( rectfarg bounds )
{
	auto result = imguiGetTitlebar( bounds );
	result.right -= height( result ) * 2 - ImGui->style.innerPadding;
	return result;
}

void imguiClear()
{
	ImGui->group                  = -1;
	ImGui->renderCommandJumpFirst = nullptr;
	ImGui->renderCommandJumpLast  = nullptr;
	if( isKeyUp( ImGui->inputs, KC_LButton ) ) {
		ImGui->captureGroup = -1;
	}
	// set hover group
	auto mousePosition = ImGui->inputs->mouse.position;
	if( ImGui->captureGroup >= 0 ) {
		// if mouse is captured, treat the capturing group as the hover group
		ImGui->hoverGroup = ImGui->captureGroup;
	} else {
		ImGui->hoverGroup = -1;
		if( auto font = ImGui->font ) {
			int8 z = 0;
			for( intmax i = 0, count = ImGui->groupsCount; i < count; ++i ) {
				auto group = &ImGui->groups[i];
				if( group->hidden ) {
					continue;
				}
				rectf rect;
				if( group->minimized ) {
					rect = imguiGetTitlebar( group->rect );
				} else {
					rect = group->rect;
				}
				if( isPointInside( rect, mousePosition ) && group->z >= z ) {
					ImGui->hoverGroup = safe_truncate< int8 >( i );
					z                 = group->z;
				}
			}
		}
	}
}
void imguiBind( ImmediateModeGui* guiState, RenderCommands* renderer, Font* font,
                GameInputs* inputs, void* base, rectfarg bounds )
{
	ImGui           = guiState;
	ImGui->renderer = renderer;
	ImGui->inputs   = inputs;
	ImGui->font     = font;
	ImGui->base     = base;
	ImGui->bounds   = bounds;
	imguiClear();
}

ImGuiHandle imguiMakeHandle( void* ptr )
{
	return {( uint32 )( (uintptr)ptr - (uintptr)ImGui->base ), 0, ImGui->group};
}
ImGuiHandle imguiSetCurrentGroup( ImGuiGroupState* group )
{
	assert( group >= ImGui->groups && group < ImGui->groups + ImGuiMaxGroups );
	ImGuiHandle result = {};
	result.group       = safe_truncate< int8 >( group - ImGui->groups );
	ImGui->group       = result.group;
	return result;
}

ImGuiHandle imguiMakeIndexHandle( uint16 index )
{
	ImGuiHandle result = {};
	result.base        = 0;
	result.index       = index;
	result.group       = ImGui->group;
	result.flags       = ImGuiHandleFlags::Unbased;
	return result;
}
ImGuiHandle imguiMakeIndexHandle( void* ptr )
{
	ImGuiHandle result = {};
#ifdef ARCHITECTURE_X64
	result.base = ( uint32 )( (uintptr)ptr >> 32 );
#elif ARCHITECTURE_X86
	result.base = ( uint32 )( (uintptr)ptr );
#else
#error unknown architecture
#endif
	result.group = ImGui->group;
	result.index = ( ( uint16 )( (uintptr)ptr - (uintptr)ImGui->base ) );
	result.flags = ImGuiHandleFlags::Unbased;
	return result;
}
struct ImGuiStringPair {
	StringView string;
	StringView base;
};
ImGuiStringPair imguiBaseStr( StringView str )
{
	int32 index = findLast( str, {"#", 1} );
	if( index == StringView::npos || index == str.size() - 1 ) {
		return {str, str};
	}
	return {substr( str, 0, index ), substr( str, index + 1 )};
}
struct ImGuiStringHandlePair {
	StringView string;
	ImGuiHandle handle;
};
ImGuiStringHandlePair imguiMakeStringHandle( StringView str )
{
	ImGuiStringHandlePair result = {};
	auto baseResult              = imguiBaseStr( str );
	result.string                = baseResult.string;
	result.handle                = imguiMakeIndexHandle( (void*)baseResult.base.data() );
	return result;
}
#if 0
bool imguiButton( StringView name, const rectf& rect )
{
	auto handle = imguiMakeIndexHandle( name );
}
#endif

static void renderTextCenteredClipped( RenderCommands* renderer, Font* font, StringView text,
                                       const rectf& rect )
{
	ClippingRect clip( renderer, rect );

	reset( font );
	font->wrappingMode  = WrappingMode::None;
	font->align         = FontAlign::Center;
	font->verticalAlign = FontVerticalAlign::Center;
	renderText( renderer, font, text, rect );
}
static void renderTextClipped( RenderCommands* renderer, Font* font, StringView text,
                               const rectf& rect )
{
	ClippingRect clip( renderer, rect );

	reset( font );
	font->wrappingMode = WrappingMode::None;
	renderText( renderer, font, text, rect );
}
static void renderTextClippedOffset( RenderCommands* renderer, Font* font, StringView text,
                                     const rectf& clippingRect, float offset )
{
	ClippingRect clip( renderer, clippingRect );

	reset( font );
	font->wrappingMode = WrappingMode::None;
	auto textRect      = translate( clippingRect, offset, 0.0f );
	renderText( renderer, font, text, textRect );
}

bool imguiHasFocus( ImGuiHandle handle ) { return ImGui->focus == handle; }
bool imguiIsHover( ImGuiHandle handle ) { return ImGui->hoverGroup == handle.group; }
void imguiCapture( ImGuiHandle handle ) { ImGui->captureGroup = ImGui->hoverGroup = handle.group; }
void imguiBringToFront( int8 group )
{
	assert( group >= 0 && group < ImGui->groupsCount );
	auto current = &ImGui->groups[group];
	auto z       = current->z;
	for( intmax i = 0, count = ImGui->groupsCount; i < count; ++i ) {
		auto entry = &ImGui->groups[i];
		if( entry->z > z ) {
			--entry->z;
		}
	}
	current->z = safe_truncate< int8 >( ImGui->groupsCount - 1 );
}
void imguiFocus( ImGuiHandle handle )
{
	ImGui->focus = handle;
	imguiBringToFront( handle.group );
}

vec2 imguiBoundedTranslation( rectfarg draggable, vec2 trans )
{
	rectf bounds     = ImGui->bounds;
	rectf translated = translate( draggable, trans );
	auto h           = height( draggable );
	if( bounds.right - translated.left < h ) {
		trans.x -= h - ( bounds.right - translated.left );
	} else if( translated.right - bounds.left < h ) {
		trans.x += h - ( translated.right - bounds.left );
	}
	if( bounds.bottom - translated.top < h ) {
		trans.y -= h - ( bounds.bottom - translated.top );
	} else if( translated.top < 0 ) {
		trans.y += -translated.top;
	}
	return trans;
}

void* imguiToUserData( int32 z ) { return (void*)( (uintmax)unsignedof( z ) ); }
int32 imguiToZ( void* userData ) { return ( int32 )( ( uint32 )( (uintmax)userData ) ); }

// handle logic of button
bool imguiButton( ImGuiHandle handle, rectfarg rect )
{
	auto inputs = ImGui->inputs;
	if( imguiIsHover( handle ) && isPointInside( rect, inputs->mouse.position ) ) {
		if( imguiHasFocus( handle ) && isKeyReleased( inputs, KC_LButton ) ) {
			return true;
		}
		if( isKeyPressed( inputs, KC_LButton ) ) {
			imguiFocus( handle );
		}
	}
	return false;
}

bool imguiGroup( StringView name, int32 groupIndex )
{
	auto groupState = imguiGetGroup( groupIndex );
	auto handle     = imguiSetCurrentGroup( groupState );
	if( groupState->hidden ) {
		return false;
	}

	auto style    = &ImGui->style;
	auto renderer = ImGui->renderer;
	auto font     = ImGui->font;
	auto inputs   = ImGui->inputs;

	if( width( groupState->rect ) <= 0 || height( groupState->rect ) <= 0 ) {
		// group is uninitialized
		*groupState      = defaultImGuiGroupSate( groupState->rect );
		groupState->rect = imguiGetTitlebar( groupState->rect );
	}

	rectf rect          = groupState->rect;
	rectf titlebarRect  = imguiGetTitlebar( groupState->rect );
	auto titlebarClient = shrink( titlebarRect, style->innerPadding );
	auto buttonSize     = height( titlebarClient );
	auto draggableRect  = imguiGetDraggableRect( groupState->rect );

	auto closeButton        = RectSetLeft( titlebarClient, titlebarClient.right - buttonSize );
	auto closeButtonHandle  = handle;
	closeButtonHandle.index = 0;
	closeButtonHandle.flags |= ImGuiHandleFlags::Internal;
	auto minButton        = translate( closeButton, -buttonSize - style->innerPadding, 0.0f );
	auto minButtonHandle  = handle;
	minButtonHandle.index = 1;
	minButtonHandle.flags |= ImGuiHandleFlags::Internal;

	auto wasDragging = groupState->dragging;
	if( isKeyPressed( inputs, KC_LButton ) && imguiIsHover( handle ) ) {
		imguiFocus( handle );
		if( isPointInside( draggableRect, inputs->mouse.position ) ) {
			imguiCapture( handle );
			ImGui->mouseOffset = inputs->mouse.position - rect.leftTop;
			// drag
			groupState->dragging = true;
		}
	}
	if( imguiButton( closeButtonHandle, closeButton ) ) {
		groupState->hidden = true;
		return false;
	} else if( imguiButton( minButtonHandle, minButton ) ) {
		groupState->minimized = !groupState->minimized;
	}

	if( groupState->minimized ) {
		rect.bottom      = titlebarRect.bottom;
		groupState->rect = rect;
	}

	if( isKeyDown( inputs, KC_LButton ) && imguiHasFocus( handle ) ) {
		if( wasDragging ) {
			auto delta       = inputs->mouse.position - ImGui->mouseOffset - rect.leftTop;
			delta            = imguiBoundedTranslation( draggableRect, delta );
			groupState->rect = rect = translate( rect, delta );
			titlebarRect            = imguiGetTitlebar( groupState->rect );
		}
	} else {
		groupState->dragging = false;
	}

	ImGui->renderCommandJumpLast = addRenderCommandJump( renderer, ImGui->renderCommandJumpLast,
	                                                     imguiToUserData( groupState->z ) );
	if( !ImGui->renderCommandJumpFirst ) {
		ImGui->renderCommandJumpFirst = ImGui->renderCommandJumpLast;
	}
	auto prevColor   = renderer->color;
	auto color       = getColorF( renderer->color );
	auto bgColor     = multiplyComponents( vec4{.5f, 0, 0, 1}, color );
	auto buttonColor = multiplyComponents( vec4{1, 1, 1, 1}, color );
	auto textColor   = color;

	auto textRect = titlebarClient;
	textRect.right -= ( style->innerPadding + buttonSize ) * 2;

	setTexture( renderer, 0, null );
	MESH_STREAM_BLOCK( stream, renderer ) {
		stream->color = Color::argb( bgColor );
		pushQuad( stream, rect );

		stream->color = Color::argb( buttonColor );
		pushQuad( stream, closeButton );
		pushQuad( stream, minButton );
	}

	renderer->color = Color::argb( textColor );
	renderTextCenteredClipped( renderer, font, name, textRect );

	renderer->color = prevColor;

	if( groupState->minimized ) {
		return false;
	}
	groupState->addPosition = {titlebarRect.left, titlebarRect.bottom + style->innerPadding};
	return true;
}

ImGuiGroupState* imguiCurrentGroup()
{
	assert( ImGui->group >= 0 && ImGui->group < ImGui->groupsCount );
	return &ImGui->groups[ImGui->group];
}
rectf imguiAddItem( float width, float height )
{
	auto group  = imguiCurrentGroup();
	auto result = RectWH( group->addPosition.x + ImGui->style.innerPadding, group->addPosition.y,
		                 width, height );
	if( group->horizontalCount > 0 ) {
		// change layout horizontally
		group->addPosition.x += width + ImGui->style.innerPadding;
		--group->horizontalCount;
		if( group->horizontalCount == 0 ) {
			group->addPosition.y += height + ImGui->style.innerPadding;
			group->rect.bottom = group->addPosition.y;
			group->addPosition.x = group->rect.left;
		}
	} else {
		// change layout vertically
		group->addPosition.y += height + ImGui->style.innerPadding;
		group->rect.bottom = group->addPosition.y;
	}
	return result;
}
void imguiSameLine( int32 count )
{
	auto group  = imguiCurrentGroup();
	group->horizontalCount = count;
}
float imguiClientWidth( ImGuiGroupState* group )
{
	assert( group );
	return width( group->rect ) - ImGui->style.innerPadding * 2;
}

void imguiText( StringView text, float width, float height )
{
	auto renderer = ImGui->renderer;
	auto font     = ImGui->font;

	reset( font );
	auto rect = imguiAddItem( width, height );

	RENDER_COMMANDS_STATE_BLOCK( renderer ) {
		renderer->color = multiply( renderer->color, ImGui->style.text );
		renderText( renderer, font, text, rect );
	}
}
void imguiText( StringView text )
{
	auto font     = ImGui->font;
	auto group = imguiCurrentGroup();
	auto width = imguiClientWidth( group );
	imguiText( text, width, stringHeight( font, text, width ) );
}

rectf imguiInnerRect( rectfarg rect ) { return shrink( rect, ImGui->style.innerPadding ); }

bool imguiButton( ImGuiHandle handle, StringView name, float width, float height )
{
	auto renderer = ImGui->renderer;
	auto font     = ImGui->font;

	reset( font );
	auto textWidth  = stringWidth( font, name ) + ImGui->style.innerPadding * 2;
	auto textHeight = stringHeight( font, name ) + ImGui->style.innerPadding * 2;
	width           = MAX( textWidth, width );
	height          = MAX( textHeight, height );

	auto rect   = imguiAddItem( width, height );
	vec2 offset = {};
	if( imguiHasFocus( handle ) && isPointInside( rect, ImGui->inputs->mouse.position )
	    && isKeyDown( ImGui->inputs, KC_LButton ) ) {
		offset = {1, 1};
	}

	RENDER_COMMANDS_STATE_BLOCK( renderer ) {
		setTexture( renderer, 0, null );
		renderer->color = multiply( renderer->color, ImGui->style.buttonBg );
		MESH_STREAM_BLOCK( stream, renderer ) {
			pushQuad( stream, rect );
		}
		renderer->color = multiply( renderer->color, ImGui->style.buttonText );
		auto textArea   = imguiInnerRect( rect );
		textArea.leftTop += offset;
		renderTextCenteredClipped( renderer, font, name, textArea );
	}

	return imguiButton( handle, rect );
}

bool imguiButton( StringView name )
{
	auto stringHandle = imguiMakeStringHandle( name );
	return imguiButton( stringHandle.handle, stringHandle.string, ImGui->style.buttonWidth,
	                    ImGui->style.buttonHeight );
}
bool imguiButton( StringView name, float width, float height )
{
	auto stringHandle = imguiMakeStringHandle( name );
	return imguiButton( stringHandle.handle, stringHandle.string, width, height );
}

bool imguiEditbox( ImGuiHandle handle, StringView name, char* data, int32* length, int32 size,
                   float width, float height )
{

	assert( length );
	assert( *length <= size );

	auto renderer = ImGui->renderer;
	auto font     = ImGui->font;
	auto inputs   = ImGui->inputs;
	auto style    = &ImGui->style;

	reset( font );
	auto nameWidth   = stringWidth( font, name );
	auto rect        = imguiAddItem( width + nameWidth + style->innerPadding, height );
	auto editboxRect = rect;
	editboxRect.left += nameWidth + style->innerPadding;
	editboxRect.right = editboxRect.left + width;
	auto textRect     = imguiInnerRect( editboxRect );
	// auto nameRect = imguiInnerRect( rect );

	ImGuiEditboxState* state = nullptr;
	string text              = makeInitializedArrayView( data, *length, size );
	if( isKeyPressed( inputs, KC_LButton ) && imguiIsHover( handle )
	    && isPointInside( editboxRect, inputs->mouse.position ) ) {

		if( !imguiHasFocus( handle ) ) {
			ImGui->state.editbox = {};
			ImGui->stateType     = ImGuiStateType::Editbox;
		}
		imguiFocus( handle );
		state = &ImGui->state.editbox;

		auto prev = state->selectionBegin;
		auto rel = inputs->mouse.position.x - textRect.left - state->textOffset;
		if( isKeyDown( inputs, KC_Shift ) ) {
			state->selectionBegin = prev;
			state->selectionEnd = charCount( font, text, rel ).index;
		} else {
			state->selectionBegin = charCount( font, text, rel ).index;
		}
	}

	if( imguiHasFocus( handle ) ) {
		state = &ImGui->state.editbox;

		if( isKeyDown( inputs, KC_LButton ) ) {
			state->resetCaret();
			auto rel            = inputs->mouse.position.x - textRect.left - state->textOffset;
			state->selectionEnd = charCount( font, text, rel ).index;
		}
	}

	rectf caretRect     = textRect;
	rectf selectionRect = textRect;

	bool changed = false;
	if( state ) {
		auto beforeCaret = substr( text, 0, state->selectionEnd );

		auto selection         = state->selection();
		auto selectedText      = substr( text, selection.min, selection.max - selection.min );
		auto selectedTextWidth = stringWidth( font, selectedText );
		auto preSelectedText   = substr( text, 0, selection.min );
		auto postSelectedText  = substr( text, selection.max );
		selectionRect.left += stringWidth( font, preSelectedText );
		selectionRect.right = selectionRect.left + selectedTextWidth;

		auto caretPos = stringWidth( font, beforeCaret );
		caretRect.top -= 1;
		caretRect.bottom += 1;
		caretRect.left += caretPos;
		caretRect.right = caretRect.left + 1;

		// insert text at caret pos
		auto inputString = asStringView( inputs );
		if( inputString.size() ) {
			auto selection = state->selection();
			text.erase( text.begin() + selection.min, text.begin() + selection.max );
			auto count = min( inputString.size(), text.remaining() );
			text.insert( text.begin() + state->selectionEnd, inputString.begin(),
			             inputString.begin() + count );
			state->resetCaret();
			state->selectionBegin = state->selectionEnd = selection.min + count;
			changed = true;
		}

		auto backPressed   = isKeyPressedRepeated( inputs, KC_Back );
		auto deletePressed = isKeyPressedRepeated( inputs, KC_Delete );
		if( backPressed || deletePressed ) {
			auto selection = state->selection();
			if( state->selectionBegin != state->selectionEnd ) {
				// there is selected text
				text.erase( text.begin() + selection.min, text.begin() + selection.max );
				state->selectionBegin = state->selectionEnd = selection.min;
			} else {
				if( backPressed && state->selectionEnd > 0 ) {
					auto prev =
					    state->selectionEnd - utf8::retreat( text.begin(), state->selectionEnd );
					text.erase( text.begin() + prev, text.begin() + state->selectionEnd );
					state->selectionEnd   = prev;
					state->selectionBegin = state->selectionEnd;
				} else if( deletePressed && state->selectionEnd < text.size() ) {
					auto next = state->selectionEnd
					            + utf8::advance( text.begin(), state->selectionEnd, text.size() );
					text.erase( text.begin() + state->selectionEnd, text.begin() + next );
				}
			}
			changed = true;
			state->resetCaret();
		}

		auto shiftDown        = isKeyDown( inputs, KC_Shift );
		auto processCaretMove = [&]( int32 advance ) {
			if( state->selectionBegin == state->selectionEnd || shiftDown ) {
				if( ( state->selectionEnd > 0 && advance < 0 )
				    || ( state->selectionEnd < text.size() && advance > 0 ) ) {

					if( advance > 0 ) {
						state->selectionEnd +=
						    utf8::advance( text.begin(), state->selectionEnd, text.size() );
					} else {
						state->selectionEnd -= utf8::retreat( text.begin(), state->selectionEnd );
					}
				}
				if( !shiftDown ) {
					state->selectionBegin = state->selectionEnd;
				}
			} else {
				state->selectionBegin = state->selectionEnd;
			}
			state->resetCaret();
		};
		if( isKeyPressedRepeated( inputs, KC_Left ) ) {
			processCaretMove( -1 );
		} else if( isKeyPressedRepeated( inputs, KC_Right ) ) {
			processCaretMove( 1 );
		}

		{
			auto selection        = state->selection();
			auto textOffset       = state->textOffset;
			auto beforeCaret      = substr( text, 0, state->selectionEnd );
			auto beforeCaretWidth = stringWidth( font, beforeCaret ) + textOffset;
			auto textRectWidth    = ::width( textRect );

			if( beforeCaretWidth > textRectWidth ) {
				textOffset += textRectWidth - beforeCaretWidth;
			} else if( beforeCaretWidth < 0 ) {
				textOffset -= beforeCaretWidth;
			} else {
				auto textWidth = stringWidth( font, text ) + textOffset;
				if( textWidth < textRectWidth ) {
					if( textOffset < 0 ) {
						textOffset += textRectWidth - textWidth;
					}
					if( textOffset > 0 ) {
						textOffset = 0;
					}
				}
			}
			state->textOffset = textOffset;

			caretRect.left += textOffset;
			caretRect.right += textOffset;
			selectionRect.left += textOffset;
			selectionRect.right += textOffset;
		}
	}

	RENDER_COMMANDS_STATE_BLOCK( renderer ) {
		auto color = renderer->color;
		setTexture( renderer, 0, null );
		renderer->color = multiply( color, ImGui->style.editboxBg );
		MESH_STREAM_BLOCK( stream, renderer ) {
			pushQuad( stream, editboxRect );
			if( state ) {
				stream->color = multiply( color, ImGui->style.editboxTextSelectedBg );
				pushQuad( stream, selectionRect );
			}

			// draw caret
			if( state && !state->hideCaret ) {
				stream->color = multiply( color, ImGui->style.editboxCaret );
				pushQuad( stream, caretRect );
			}
		}

		renderer->color = multiply( color, ImGui->style.name );
		renderTextClipped( renderer, font, name, rect );

		renderer->color = multiply( color, ImGui->style.editboxText );
		if( state ) {
			renderTextClippedOffset( renderer, font, text, textRect, state->textOffset );
		} else {
			renderTextClipped( renderer, font, text, textRect );
		}
	}

	*length = text.size();
	return changed;
}
bool imguiEditbox( StringView name, char* data, int32* length, int32 size, float width,
                   float height )
{
	return imguiEditbox( imguiMakeHandle( data ), name, data, length, size, width, height );
}

bool imguiEditbox( StringView name, char* data, int32* length, int32 size )
{
	return imguiEditbox( name, data, length, size, ImGui->style.editboxWidth,
	                     ImGui->style.editboxHeight );
}

bool imguiEditbox( StringView name, float* value )
{
	assert( value );
	auto handle = imguiMakeHandle( value );
	if( imguiHasFocus( handle ) ) {
		if( imguiEditbox( handle, name, ImGui->editboxStatic, &ImGui->editboxStaticCount,
		                  countof( ImGui->editboxStatic ), ImGui->style.editboxWidth,
		                  ImGui->style.editboxHeight ) ) {
			*value = to_float( ImGui->editboxStatic, ImGui->editboxStaticCount, 0 );
			return true;
		}
		return false;
	} else {
		auto str    = toNumberString( *value );
		auto result = imguiEditbox( handle, name, str.data, &str.count, str.count,
		                            ImGui->style.editboxWidth, ImGui->style.editboxHeight );
		if( imguiHasFocus( handle ) ) {
			memcpy( ImGui->editboxStatic, str.data, str.count );
			ImGui->editboxStaticCount = str.count;
		}
		return result;
	}
}
bool imguiEditbox( StringView name, int32* value )
{
	assert( value );
	auto handle = imguiMakeHandle( value );
	if( imguiHasFocus( handle ) ) {
		if( imguiEditbox( handle, name, ImGui->editboxStatic, &ImGui->editboxStaticCount,
		                  countof( ImGui->editboxStatic ), ImGui->style.editboxWidth,
		                  ImGui->style.editboxHeight ) ) {
			*value = to_i32( ImGui->editboxStatic, ImGui->editboxStaticCount, 0 );
			return true;
		}
		return false;
	} else {
		auto str    = toNumberString( *value );
		auto result = imguiEditbox( handle, name, str.data, &str.count, str.count,
		                            ImGui->style.editboxWidth, ImGui->style.editboxHeight );
		if( imguiHasFocus( handle ) ) {
			memcpy( ImGui->editboxStatic, str.data, str.count );
			ImGui->editboxStaticCount = str.count;
		}
		return result;
	}
}

void imguiUpdate( float dt )
{
	switch( ImGui->stateType ) {
		case ImGuiStateType::Editbox: {
			auto state = &ImGui->state.editbox;
			state->timePassed += dt;
			if( state->timePassed > ImGui->caretBlinkTime ) {
				state->timePassed = 0;
				state->hideCaret  = !state->hideCaret;
			}
			break;
		}
	}
}

bool imguiCheckbox( StringView name, bool* checked )
{
	assert( checked );

	auto renderer = ImGui->renderer;
	auto font     = ImGui->font;
	auto inputs   = ImGui->inputs;
	auto style    = &ImGui->style;

	auto handle  = imguiMakeHandle( checked );
	auto changed = false;

	auto index =
	    ( *checked ) ? ( ImGuiTexCoords::CheckboxChecked ) : ( ImGuiTexCoords::CheckboxUnchecked );

	auto width =
	    stringWidth( font, name ) + style->innerPadding * 2 + ::width( style->rects[index] );
	auto height =
	    max( stringHeight( font ), ::height( style->rects[index] ) ) + style->innerPadding * 2;
	auto rect      = imguiAddItem( width, height );
	auto inner     = imguiInnerRect( rect );
	auto checkRect = translate( style->rects[index], inner.leftTop );

	if( isKeyPressed( inputs, KC_LButton ) && imguiIsHover( handle )
	    && isPointInside( checkRect, inputs->mouse.position ) ) {
		imguiFocus( handle );
		*checked = !*checked;
		changed  = true;
	}

	RENDER_COMMANDS_STATE_BLOCK( renderer ) {
		setTexture( renderer, 0, style->atlas );
		MESH_STREAM_BLOCK( stream, renderer ) {
			pushQuad( stream, checkRect, 0, makeQuadTexCoords( style->texCoords[index] ) );
		}
		renderer->color = multiply( renderer->color, Color::White );
		auto textArea   = inner;
		textArea.left += ::width( checkRect ) + style->innerPadding;
		renderTextCenteredClipped( renderer, font, name, textArea );
	}

	return changed;
}
bool imguiRadiobox( StringView name, bool* checked )
{
	assert( checked );

	auto renderer = ImGui->renderer;
	auto font     = ImGui->font;
	auto inputs   = ImGui->inputs;
	auto style    = &ImGui->style;
	auto group    = imguiCurrentGroup();

	auto handle  = imguiMakeHandle( checked );
	auto changed = false;

	auto index =
	    ( *checked ) ? ( ImGuiTexCoords::RadioboxChecked ) : ( ImGuiTexCoords::RadioboxUnchecked );

	auto width =
	    stringWidth( font, name ) + style->innerPadding * 2 + ::width( style->rects[index] );
	auto height =
	    max( stringHeight( font ), ::height( style->rects[index] ) ) + style->innerPadding * 2;
	auto rect      = imguiAddItem( width, height );
	auto inner     = imguiInnerRect( rect );
	auto checkRect = translate( style->rects[index], inner.leftTop );

	auto isChecked =
	    ( ( group->checkedRadiobox == handle ) || ( !group->checkedRadiobox && *checked ) );
	changed  = *checked != isChecked;
	*checked = isChecked;
	if( !group->checkedRadiobox && *checked ) {
		group->checkedRadiobox = handle;
	}

	if( isKeyPressed( inputs, KC_LButton ) && imguiIsHover( handle )
	    && isPointInside( checkRect, inputs->mouse.position ) ) {
		imguiFocus( handle );
		if( !*checked ) {
			changed                = true;
			*checked               = true;
			group->checkedRadiobox = handle;
		}
	}

	RENDER_COMMANDS_STATE_BLOCK( renderer ) {
		setTexture( renderer, 0, style->atlas );
		MESH_STREAM_BLOCK( stream, renderer ) {
			pushQuad( stream, checkRect, 0, makeQuadTexCoords( style->texCoords[index] ) );
		}
		renderer->color = multiply( renderer->color, Color::White );
		auto textArea   = inner;
		textArea.left += ::width( checkRect ) + style->innerPadding;
		renderTextCenteredClipped( renderer, font, name, textArea );
	}

	return changed;
}

bool imguiSlider( StringView name, float* value, float min, float max )
{
	assert( value );

	auto renderer = ImGui->renderer;
	auto font     = ImGui->font;
	auto inputs   = ImGui->inputs;
	auto style    = &ImGui->style;
	auto group    = imguiCurrentGroup();

	auto handle  = imguiMakeHandle( value );
	auto changed = false;

	auto width = ::width( group->rect );
	auto height =
	    ::max( stringHeight( font ), ::height( style->rects[ImGuiTexCoords::SliderKnob] ) )
	    + style->innerPadding * 2;
	auto rect  = imguiAddItem( width, height );
	auto inner = imguiInnerRect( rect );

	auto sliderArea = inner;
	sliderArea.left += stringWidth( font, name ) + style->innerPadding;
	*value = clamp( *value, min, max );
	sliderArea.right -= ::width( style->rects[ImGuiTexCoords::SliderKnob] );
	float knobPosition = ( ( *value - min ) / ( max - min ) ) * ::width( sliderArea );
	rectf knobRect     = translate( style->rects[ImGuiTexCoords::SliderKnob],
	                            sliderArea.left + knobPosition, sliderArea.top );

	ImGuiSliderState* state = nullptr;
	if( imguiHasFocus( handle ) ) {
		state = &ImGui->state.slider;
	}
	bool wasDragging = true;
	if( isKeyPressed( inputs, KC_LButton ) && imguiIsHover( handle )
	    && isPointInside( knobRect, inputs->mouse.position ) ) {
		imguiFocus( handle );
		imguiCapture( handle );
		ImGui->mouseOffset = inputs->mouse.position - rect.leftTop;
		state              = &ImGui->state.slider;
		*state             = {};
		ImGui->stateType   = ImGuiStateType::Slider;
		state->knobRect    = knobRect;
		wasDragging        = false;
		state->dragging    = true;
	}

	if( isKeyDown( inputs, KC_LButton ) && imguiHasFocus( handle ) && state->dragging ) {
		auto delta     = inputs->mouse.position - ImGui->mouseOffset - rect.leftTop;
		knobRect       = translate( state->knobRect, delta.x, 0.0f );
		knobRect.left  = clamp( knobRect.left, sliderArea.left, sliderArea.right );
		knobRect.right = knobRect.left + ::width( style->rects[ImGuiTexCoords::SliderKnob] );
		knobPosition   = knobRect.left - sliderArea.left;
		*value         = ( knobPosition / ::width( sliderArea ) ) + min;
	} else if( state ) {
		state->dragging = false;
	}

	RENDER_COMMANDS_STATE_BLOCK( renderer ) {
		setTexture( renderer, 0, style->atlas );
		MESH_STREAM_BLOCK( stream, renderer ) {
			pushQuad( stream, knobRect, 0,
			          makeQuadTexCoords( style->texCoords[ImGuiTexCoords::SliderKnob] ) );
		}
		renderer->color = multiply( renderer->color, Color::White );
		auto textArea   = inner;
		renderTextCenteredClipped( renderer, font, name, textArea );
	}

	return changed;
}

void imguiFinalize()
{
	if( ImGui->renderCommandJumpFirst ) {
		auto renderer                = ImGui->renderer;
		ImGui->renderCommandJumpLast = addRenderCommandJump(
		    renderer, ImGui->renderCommandJumpLast, imguiToUserData( ImGui->groupsCount ) );
		sortRenderCommandJumps( renderer, ImGui->renderCommandJumpFirst,
		                        ImGui->renderCommandJumpLast,
		                        []( const RenderCommandJump& a, const RenderCommandJump& b ) {
			                        return imguiToZ( a.userData ) < imguiToZ( b.userData );
			                    } );
	}
}