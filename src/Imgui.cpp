/*
TODO:
    - when fading out, the gui still accepts input, needs a way to toggle input processing
    - when dragging a container, the titlebar lags behind
*/

union ImGuiHandle {
	struct {
		uint32 base;
		uint16 index;
		int8 container;
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

	DropGroupRetracted,
	DropGroupExpanded,

	ComboButton,

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

	float containerWidth;

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

	result.containerWidth = 200;

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

struct ImGuiContainerState {
	rectf rect;
	vec2 addPosition;
	int32 horizontalCount; // how many controls to place horizontally instead of vertically
	rectf lastRect;
	int8 z;
	bool8 minimized;
	bool8 hidden;
	bool8 dragging;
	Mesh* bgMesh;

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

ImGuiContainerState defaultImGuiContainerSate( rectfarg rect );
#define ImGuiMaxContainers 8

struct ImGuiSliderState {
	rectf knobRect;
	bool dragging;
};

struct ImGuiControlState {
	enum { type_none, type_editbox, type_slider } type;
	union {
		ImGuiEditboxState editbox;
		ImGuiSliderState slider;
	};
};

struct ImmediateModeGui {
	RenderCommands* renderer;
	GameInputs* inputs;
	Font* font;
	void* base;
	// gui bounds
	rectf bounds;
	ImGuiStyle style;

	int8 container;         // current container
	int8 hoverContainer;    // the container the mouse is currently over
	int8 captureContainer;  // the container that has the mouse currently captured
	ImGuiHandle focus;
	vec2 mouseOffset;  // offset of mouse when dragging to the leftTop position of the container

	char editboxStatic[100];
	int32 editboxStaticCount;
	ImGuiControlState state;
	float caretBlinkTime;

	int32 containersCount;
	ImGuiContainerState containers[ImGuiMaxContainers];

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
	using namespace ImGuiTexCoords;
	auto style   = &ImGui->style;
	style->atlas = platform->loadTexture( "Data/Images/gui_atlas.png" );
	auto itw     = 1.0f / 100.0f;
	auto ith     = 1.0f / 100.0f;

	style->texCoords[RadioboxUnchecked] = RectWH( 0, 0, 13 * itw, 13 * ith );
	style->texCoords[RadioboxChecked]   = RectWH( 14 * itw, 0, 13 * itw, 13 * ith );
	style->texCoords[CheckboxUnchecked] = RectWH( 0, 14 * ith, 13 * itw, 13 * ith );
	style->texCoords[CheckboxChecked]   = RectWH( 14 * itw, 14 * ith, 13 * itw, 13 * ith );

	style->texCoords[DropGroupRetracted] = RectWH( 14 * itw, 28 * ith, 13 * itw, 13 * ith );
	style->texCoords[DropGroupExpanded]  = RectWH( 28 * itw, 28 * ith, 13 * itw, 13 * ith );

	style->texCoords[ComboButton] = RectWH( 28 * itw, 28 * ith, 13 * itw, 13 * ith );

	style->texCoords[SliderKnob] = RectWH( 0, 28 * ith, 13 * itw, 13 * ith );

	style->rects[RadioboxUnchecked] = {0, 0, 13, 13};
	style->rects[RadioboxChecked]   = {0, 0, 13, 13};
	style->rects[CheckboxUnchecked] = {0, 0, 13, 13};
	style->rects[CheckboxChecked]   = {0, 0, 13, 13};
	style->rects[SliderKnob]        = {0, 0, 13, 13};

	style->rects[DropGroupRetracted] = {0, 0, 13, 13};
	style->rects[DropGroupExpanded]  = {0, 0, 13, 13};

	style->rects[ComboButton] = {0, 0, 13, 13};
}

ImGuiContainerState defaultImGuiContainerSate( rectfarg rect )
{
	return {RectWH( rect.left, rect.top, ImGui->style.containerWidth, 0.0f )};
}
int32 imguiGenerateContainer( rectfarg rect = {} )
{
	assert( ImGui->containersCount < ImGuiMaxContainers );
	auto result = ImGui->containersCount;
	auto container  = &ImGui->containers[ImGui->containersCount];
	*container      = defaultImGuiContainerSate( {} );
	container->rect = rect;
	container->z    = safe_truncate< int8 >( ImGui->containersCount );
	++ImGui->containersCount;
	return result;
}
ImGuiContainerState* imguiGetContainer( int32 index )
{
	assert( index >= 0 && index < countof( ImGui->containers ) );
	return &ImGui->containers[index];
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
	ImGui->container                  = -1;
	ImGui->renderCommandJumpFirst = nullptr;
	ImGui->renderCommandJumpLast  = nullptr;
	FOR( container : ImGui->containers ) {
		container.bgMesh = nullptr;
	}

	if( isKeyUp( ImGui->inputs, KC_LButton ) ) {
		ImGui->captureContainer = -1;
	}
	// set hover container
	auto mousePosition = ImGui->inputs->mouse.position;
	if( ImGui->captureContainer >= 0 ) {
		// if mouse is captured, treat the capturing container as the hover container
		ImGui->hoverContainer = ImGui->captureContainer;
	} else {
		ImGui->hoverContainer = -1;
		if( auto font = ImGui->font ) {
			int8 z = 0;
			for( intmax i = 0, count = ImGui->containersCount; i < count; ++i ) {
				auto container = &ImGui->containers[i];
				if( container->hidden ) {
					continue;
				}
				rectf rect;
				if( container->minimized ) {
					rect = imguiGetTitlebar( container->rect );
				} else {
					rect = container->rect;
				}
				if( isPointInside( rect, mousePosition ) && container->z >= z ) {
					ImGui->hoverContainer = safe_truncate< int8 >( i );
					z                 = container->z;
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
	return {( uint32 )( (uintptr)ptr - (uintptr)ImGui->base ), 0, ImGui->container};
}
ImGuiHandle imguiSetCurrentContainer( ImGuiContainerState* container )
{
	assert( container >= ImGui->containers && container < ImGui->containers + ImGuiMaxContainers );
	ImGuiHandle result = {};
	result.container       = safe_truncate< int8 >( container - ImGui->containers );
	ImGui->container       = result.container;
	return result;
}

ImGuiHandle imguiMakeIndexHandle( uint16 index )
{
	ImGuiHandle result = {};
	result.base        = 0;
	result.index       = index;
	result.container       = ImGui->container;
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
	result.container = ImGui->container;
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
bool imguiIsHover( ImGuiHandle handle ) { return ImGui->hoverContainer == handle.container; }
void imguiCapture( ImGuiHandle handle )
{
	ImGui->captureContainer = ImGui->hoverContainer = handle.container;
}
void imguiBringToFront( int8 container )
{
	assert( container >= 0 && container < ImGui->containersCount );
	auto current = &ImGui->containers[container];
	auto z       = current->z;
	for( intmax i = 0, count = ImGui->containersCount; i < count; ++i ) {
		auto entry = &ImGui->containers[i];
		if( entry->z > z ) {
			--entry->z;
		}
	}
	current->z = safe_truncate< int8 >( ImGui->containersCount - 1 );
}
void imguiFocus( ImGuiHandle handle )
{
	ImGui->focus = handle;
	imguiBringToFront( handle.container );
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

bool imguiDialog( StringView name, int32 containerIndex )
{
	auto container = imguiGetContainer( containerIndex );
	auto handle    = imguiSetCurrentContainer( container );
	if( container->hidden ) {
		return false;
	}

	auto style    = &ImGui->style;
	auto renderer = ImGui->renderer;
	auto font     = ImGui->font;
	auto inputs   = ImGui->inputs;

	if( width( container->rect ) <= 0 || height( container->rect ) <= 0 ) {
		// container is uninitialized
		*container      = defaultImGuiContainerSate( container->rect );
		container->rect = imguiGetTitlebar( container->rect );
	}

	rectf rect          = container->rect;
	rectf titlebarRect  = imguiGetTitlebar( container->rect );
	auto titlebarClient = shrink( titlebarRect, style->innerPadding );
	auto buttonSize     = height( titlebarClient );
	auto draggableRect  = imguiGetDraggableRect( container->rect );

	auto closeButton        = RectSetLeft( titlebarClient, titlebarClient.right - buttonSize );
	auto closeButtonHandle  = handle;
	closeButtonHandle.index = 0;
	closeButtonHandle.flags |= ImGuiHandleFlags::Internal;
	auto minButton        = translate( closeButton, -buttonSize - style->innerPadding, 0.0f );
	auto minButtonHandle  = handle;
	minButtonHandle.index = 1;
	minButtonHandle.flags |= ImGuiHandleFlags::Internal;

	auto wasDragging = container->dragging;
	if( isKeyPressed( inputs, KC_LButton ) && imguiIsHover( handle ) ) {
		imguiFocus( handle );
		if( isPointInside( draggableRect, inputs->mouse.position ) ) {
			imguiCapture( handle );
			ImGui->mouseOffset = inputs->mouse.position - rect.leftTop;
			// drag
			container->dragging = true;
		}
	}
	if( imguiButton( closeButtonHandle, closeButton ) ) {
		container->hidden = true;
		return false;
	} else if( imguiButton( minButtonHandle, minButton ) ) {
		container->minimized = !container->minimized;
	}

	if( container->minimized ) {
		rect.bottom     = titlebarRect.bottom;
		container->rect = rect;
	}

	if( isKeyDown( inputs, KC_LButton ) && imguiHasFocus( handle ) ) {
		if( wasDragging ) {
			auto delta      = inputs->mouse.position - ImGui->mouseOffset - rect.leftTop;
			delta           = imguiBoundedTranslation( draggableRect, delta );
			container->rect = rect = translate( rect, delta );
			titlebarRect           = imguiGetTitlebar( container->rect );
		}
	} else {
		container->dragging = false;
	}

	ImGui->renderCommandJumpLast = addRenderCommandJump( renderer, ImGui->renderCommandJumpLast,
	                                                     imguiToUserData( container->z ) );
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
	auto meshCommand = addRenderCommandMesh( renderer, 4, 6 );
	container->bgMesh = &meshCommand->mesh;
	{
		auto bgStream  = makeMeshStream( container->bgMesh );
		bgStream.color = Color::argb( bgColor );
		pushQuad( &bgStream, rect );
	}
	MESH_STREAM_BLOCK( stream, renderer ) {
		stream->color = Color::argb( buttonColor );
		pushQuad( stream, closeButton );
		pushQuad( stream, minButton );
	}

	renderer->color = Color::argb( textColor );
	renderTextCenteredClipped( renderer, font, name, textRect );

	renderer->color = prevColor;

	if( container->minimized ) {
		return false;
	}
	container->addPosition = {titlebarRect.left, titlebarRect.bottom + style->innerPadding};
	return true;
}

ImGuiContainerState* imguiCurrentContainer()
{
	assert( ImGui->container >= 0 && ImGui->container < ImGui->containersCount );
	return &ImGui->containers[ImGui->container];
}
rectf imguiAddItem( float width, float height )
{
	auto container = imguiCurrentContainer();
	auto result    = RectWH( container->addPosition.x + ImGui->style.innerPadding,
	                         container->addPosition.y, width, height );
	bool newline = false;
	if( container->horizontalCount > 0 ) {
		// change layout horizontally
		container->addPosition.x += width + ImGui->style.innerPadding;
		--container->horizontalCount;
		if( container->horizontalCount == 0 ) {
			newline = true;
			container->addPosition.x = container->rect.left;
		}
	} else {
		newline = true;
	}
	if( newline ) {
		// change layout vertically	
		container->addPosition.y += height + ImGui->style.innerPadding;
		container->rect.bottom = container->addPosition.y;
		if( auto mesh = container->bgMesh ) {
			assert( mesh->verticesCount == 4 );
			mesh->vertices[3].position.y = mesh->vertices[2].position.y = container->rect.bottom;
		}
	}
	return result;
}
void imguiSameLine( int32 count )
{
	auto container  = imguiCurrentContainer();
	container->horizontalCount = count;
}
float imguiClientWidth( ImGuiContainerState* container )
{
	assert( container );
	return width( container->rect ) - ImGui->style.innerPadding * 2;
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
	auto container = imguiCurrentContainer();
	auto width = imguiClientWidth( container );
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
			set_variant( ImGui->state, editbox ) = {};
		}
		imguiFocus( handle );
		state = &get_variant( ImGui->state, editbox );

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
		state = &get_variant( ImGui->state, editbox );

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
			state->selectionEnd = clamp( state->selectionEnd, 0, text.size() );
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
	switch( ImGui->state.type ) {
		case ImGuiControlState::type_editbox: {
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

static bool imguiKeypressButton( ImGuiHandle handle, rectfarg rect )
{
	auto inputs   = ImGui->inputs;
	if( isKeyPressed( inputs, KC_LButton ) && imguiIsHover( handle )
	    && isPointInside( rect, inputs->mouse.position ) ) {
		imguiFocus( handle );
		return true;
	}
	return false;
}

bool imguiCheckbox( StringView name, bool* checked )
{
	assert( checked );

	auto renderer = ImGui->renderer;
	auto font     = ImGui->font;
	auto style    = &ImGui->style;

	auto handle  = imguiMakeHandle( checked );
	auto changed = false;

	using namespace ImGuiTexCoords;
	auto index = ( *checked ) ? ( CheckboxChecked ) : ( CheckboxUnchecked );

	auto width =
	    stringWidth( font, name ) + style->innerPadding * 2 + ::width( style->rects[index] );
	auto height =
	    max( stringHeight( font ), ::height( style->rects[index] ) ) + style->innerPadding * 2;
	auto rect      = imguiAddItem( width, height );
	auto inner     = imguiInnerRect( rect );
	auto checkRect = translate( style->rects[index], inner.leftTop );

	if( imguiKeypressButton( handle, checkRect ) ) {
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

	auto renderer  = ImGui->renderer;
	auto font      = ImGui->font;
	auto style     = &ImGui->style;
	auto container = imguiCurrentContainer();

	auto handle  = imguiMakeHandle( checked );
	auto changed = false;

	using namespace ImGuiTexCoords;
	auto index = ( *checked ) ? ( RadioboxChecked ) : ( RadioboxUnchecked );

	auto width =
	    stringWidth( font, name ) + style->innerPadding * 2 + ::width( style->rects[index] );
	auto height =
	    max( stringHeight( font ), ::height( style->rects[index] ) ) + style->innerPadding * 2;
	auto rect      = imguiAddItem( width, height );
	auto inner     = imguiInnerRect( rect );
	auto checkRect = translate( style->rects[index], inner.leftTop );

	auto isChecked =
	    ( ( container->checkedRadiobox == handle ) || ( !container->checkedRadiobox && *checked ) );
	changed  = *checked != isChecked;
	*checked = isChecked;
	if( !container->checkedRadiobox && *checked ) {
		container->checkedRadiobox = handle;
	}

	if( imguiKeypressButton( handle, checkRect ) ) {
		if( !*checked ) {
			changed                    = true;
			*checked                   = true;
			container->checkedRadiobox = handle;
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

bool imguiBeginDropGroup( StringView name, bool* expanded )
{
	assert( expanded );

	auto renderer  = ImGui->renderer;
	auto font      = ImGui->font;
	auto style     = &ImGui->style;
	auto container = imguiCurrentContainer();

	auto handle  = imguiMakeHandle( expanded );

	using namespace ImGuiTexCoords;
	auto index = ( *expanded ) ? ( DropGroupExpanded ) : ( DropGroupRetracted );

	auto width = ::width( container->rect ) - style->innerPadding * 2;
	auto height =
	    max( stringHeight( font ), ::height( style->rects[index] ) ) + style->innerPadding * 2;
	auto rect      = imguiAddItem( width, height );
	auto inner     = imguiInnerRect( rect );
	auto buttonRect = translate( style->rects[index], inner.leftTop );

	if( imguiKeypressButton( handle, inner ) ) {
		*expanded = !*expanded;
	}
	if( *expanded ) {
		container->rect.left += style->innerPadding;
		container->addPosition.x = container->rect.left;
		container->horizontalCount = 0;
	}

	RENDER_COMMANDS_STATE_BLOCK( renderer ) {
		renderer->color = multiply( renderer->color, Color::White );
		setTexture( renderer, 0, null );
		MESH_STREAM_BLOCK( stream, renderer ) {
			stream->color = multiply( renderer->color, {0x80544C9A} );
			pushQuad( stream, inner );
		}
		setTexture( renderer, 0, style->atlas );
		MESH_STREAM_BLOCK( stream, renderer ) {
			pushQuad( stream, buttonRect, 0, makeQuadTexCoords( style->texCoords[index] ) );
		}
		auto textArea   = inner;
		textArea.left += ::width( buttonRect ) + style->innerPadding;
		renderTextCenteredClipped( renderer, font, name, textArea );
	}

	return *expanded;
}
void imguiEndDropGroup()
{
	auto style     = &ImGui->style;
	auto container = imguiCurrentContainer();
	container->rect.left -= style->innerPadding;
	container->addPosition.x = container->rect.left;
	container->horizontalCount = 0;
}

struct ImGuiComboState {
	int32* selectedIndex;
	rectf textRect;
	bool showComboEntries;
};
ImGuiComboState imguiCombo( StringView name, int32* selectedIndex )
{
	assert( selectedIndex );
	using namespace ImGuiTexCoords;
	auto renderer = ImGui->renderer;
	auto font     = ImGui->font;
	auto style    = &ImGui->style;

	auto handle    = imguiMakeHandle( selectedIndex );
	auto container = imguiCurrentContainer();
	auto width     = ::width( container->rect ) - style->innerPadding * 2;
	auto height    = ::max( stringHeight( font ), ::height( style->rects[ComboButton] ) )
	              + style->innerPadding * 4;
	auto rect  = imguiAddItem( width, height );
	auto inner = imguiInnerRect( rect );
	inner.left -= style->innerPadding;

	auto buttonWidth = ::width( style->rects[ComboButton] );
	auto buttonRect  = RectSetLeft( inner, inner.right - buttonWidth - style->innerPadding * 2 );
	buttonRect = alignCenter( buttonRect, buttonWidth, ::height( style->rects[ComboButton] ) );

	ImGuiComboState result  = {};
	result.selectedIndex    = selectedIndex;
	result.textRect         = imguiInnerRect( RectSetRight( inner, buttonRect.left ) );
	result.showComboEntries = imguiKeypressButton( handle, buttonRect );

	RENDER_COMMANDS_STATE_BLOCK( renderer ) {
		auto color      = renderer->color;
		renderer->color = multiply( color, {0x800000FF} );
		setTexture( renderer, 0, null );
		addRenderCommandSingleQuad( renderer, inner );
		renderer->color = multiply( color, Color::White );
		setTexture( renderer, 0, style->atlas );
		addRenderCommandSingleQuad( renderer, buttonRect, 0,
		                            makeQuadTexCoords( style->texCoords[ComboButton] ) );
	}

	return result;
}
bool imguiComboEntry( const ImGuiComboState& combo, StringView name )
{
	assert( combo.selectedIndex );
	return false;
}

bool imguiSlider( StringView name, float* value, float min, float max )
{
	assert( value );

	using namespace ImGuiTexCoords;

	auto renderer  = ImGui->renderer;
	auto font      = ImGui->font;
	auto inputs    = ImGui->inputs;
	auto style     = &ImGui->style;
	auto container = imguiCurrentContainer();

	auto handle  = imguiMakeHandle( value );
	auto changed = false;

	auto width  = ::width( container->rect );
	auto height = ::max( stringHeight( font ), ::height( style->rects[SliderKnob] ) )
	              + style->innerPadding * 2;
	auto rect  = imguiAddItem( width, height );
	auto inner = imguiInnerRect( rect );

	auto sliderArea = inner;
	sliderArea.left += stringWidth( font, name ) + style->innerPadding;
	*value = clamp( *value, min, max );
	sliderArea.right -= ::width( style->rects[SliderKnob] );
	float knobPosition = ( ( *value - min ) / ( max - min ) ) * ::width( sliderArea );
	rectf knobRect =
	    translate( style->rects[SliderKnob], sliderArea.left + knobPosition, sliderArea.top );

	ImGuiSliderState* state = nullptr;
	if( imguiHasFocus( handle ) ) {
		state = &get_variant( ImGui->state, slider );
	}
	bool wasDragging = true;
	if( isKeyPressed( inputs, KC_LButton ) && imguiIsHover( handle )
	    && isPointInside( knobRect, inputs->mouse.position ) ) {
		imguiFocus( handle );
		imguiCapture( handle );
		ImGui->mouseOffset = inputs->mouse.position - rect.leftTop;
		state              = &set_variant( ImGui->state, slider );
		*state             = {};
		state->knobRect    = knobRect;
		wasDragging        = false;
		state->dragging    = true;
	}

	if( isKeyDown( inputs, KC_LButton ) && imguiHasFocus( handle ) && state->dragging ) {
		auto delta     = inputs->mouse.position - ImGui->mouseOffset - rect.leftTop;
		knobRect       = translate( state->knobRect, delta.x, 0.0f );
		knobRect.left  = clamp( knobRect.left, sliderArea.left, sliderArea.right );
		knobRect.right = knobRect.left + ::width( style->rects[SliderKnob] );
		knobPosition   = knobRect.left - sliderArea.left;
		*value         = ( knobPosition / ::width( sliderArea ) ) + min;
	} else if( state ) {
		state->dragging = false;
	}

	RENDER_COMMANDS_STATE_BLOCK( renderer ) {
		setTexture( renderer, 0, style->atlas );
		MESH_STREAM_BLOCK( stream, renderer ) {
			pushQuad( stream, knobRect, 0, makeQuadTexCoords( style->texCoords[SliderKnob] ) );
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
		    renderer, ImGui->renderCommandJumpLast, imguiToUserData( ImGui->containersCount ) );
		sortRenderCommandJumps( renderer, ImGui->renderCommandJumpFirst,
		                        ImGui->renderCommandJumpLast,
		                        []( const RenderCommandJump& a, const RenderCommandJump& b ) {
			                        return imguiToZ( a.userData ) < imguiToZ( b.userData );
			                    } );
	}
}