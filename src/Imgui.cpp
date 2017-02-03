/*
TODO:
    - when dragging a container, the titlebar lags behind
	- make specifying widths/heights more unified and allow for percentage based widths/heights
		see imguiButton and ImGuiSize
*/

static const constexpr int8 ImGuiMaxValidZ = 120;
static const constexpr int8 ImGuiMaxZ = 127;

enum class ImGuiControlType : uint8 {
	None,
	Button,
	PushButton,
	Radiobox,
	Checkbox,
	Combobox,
	Editbox,
	Listbox,
	Slider,
	Scrollbar,
	DropGroup,
	Point,
	Rect,
	Scrollable,
	ScrollableRegion,

	Custom
};
union ImGuiHandle {
	struct {
		uint32 base;
		union {
			struct {
				uint8 shortIndex;
				ImGuiControlType type;
			};
			uint16 index;
		};
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

	ExpandBox,
	RetractBox,

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

	float scrollWidth;
	float scrollHeight;

	Color buttonBg;
	Color buttonText;
	Color text;
	Color editboxBg;
	Color editboxText;
	Color editboxTextSelectedBg;
	Color editboxTextSelected;
	Color editboxCaret;

	Color menuBg;
	Color menuSelectionBg;

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

	result.scrollWidth = 14;
	result.scrollHeight = 14;

	result.buttonBg   = Color::White;
	result.buttonText = Color::Black;
	result.text       = Color::White;

	result.editboxBg             = Color::White;
	result.editboxText           = Color::Black;
	result.editboxTextSelectedBg = setAlpha( Color::Blue, 0x80 );
	result.editboxTextSelected   = Color::Red;
	result.editboxCaret          = Color::Black;

	result.menuBg = 0xFF1E1E1E;
	result.menuSelectionBg = 0x80517478;

	result.name = Color::White;

	return result;
}

namespace ImGuiContainerStateFlags
{
enum Values : uint16 {
	Minimized         = BITFIELD( 0 ),
	Hidden            = BITFIELD( 1 ),
	Dragging          = BITFIELD( 2 ),
	Modal             = BITFIELD( 3 ),
	CanGrowHorizontal = BITFIELD( 4 ),
};
}

struct ImGuiContainerState {
	rectf rect;
	vec2 addPosition;
	int32 horizontalCount;     // how many controls to place horizontally instead of vertically
	int32 maxHorizontalCount;  // how many controls are to be placed horizontally in total
	rectf lastRect;
	float xMax;
	int8 z;
	uint16 flags;
	Mesh* bgMesh;

	ImGuiHandle checkedRadiobox;

	uint16 isHidden() const { return flags & ImGuiContainerStateFlags::Hidden; }
	uint16 isMinimized() const { return flags & ImGuiContainerStateFlags::Minimized; }
	uint16 isDragging() const { return flags & ImGuiContainerStateFlags::Dragging; }
	uint16 isModal() const { return flags & ImGuiContainerStateFlags::Modal; }

	void setHidden( bool val ) { setFlagCond( flags, ImGuiContainerStateFlags::Hidden, val ); }
	void setMinimized( bool val )
	{
		setFlagCond( flags, ImGuiContainerStateFlags::Minimized, val );
	}
	void setDragging( bool val ) { setFlagCond( flags, ImGuiContainerStateFlags::Dragging, val ); }
	void setModal( bool val ) { setFlagCond( flags, ImGuiContainerStateFlags::Modal, val ); }
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
struct ImGuiComboboxState {
	rectf listRect;
	bool expanded;
	float scrollPos;
};

struct ImGuiSize {
	float width;
	float height;
};
ImGuiContainerState* imguiCurrentContainer();
ImGuiSize imguiRelative( float x, float y )
{
	auto container = imguiCurrentContainer();
	return {width( container->rect ) * x, height( container->rect ) * y};
}
ImGuiSize imguiRelative()
{
	auto container = imguiCurrentContainer();
	return {width( container->rect ) * safeDivide( 1.0f, (float)container->maxHorizontalCount ), 0};
}
ImGuiSize imguiMakeSize( ImGuiSize size, float w, float h )
{
	if( size.width == 0 ) {
		size.width = w;
	}
	if( size.height == 0 ) {
		size.height = h;
	}
	return size;
}

ImGuiContainerState defaultImGuiContainerSate( rectfarg rect );
#define ImGuiMaxContainers 8

struct ImGuiControlState {
	enum {
		type_none,
		type_editbox,
		type_combobox,
	} type;
	union {
		ImGuiEditboxState editbox;
		ImGuiComboboxState combobox;
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

	int8 container;       // current container
	int8 hoverContainer;  // the container the mouse is currently over
	int8 modalContainer;
	bool8 processInputs;
	bool8 clearCapture;
	uint8 captureKey;
	ImGuiHandle capture;  // the container that has the mouse currently captured
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
	result.caretBlinkTime   = 400;
	result.containersCount  = 1;
	result.modalContainer   = -1;
	return result;
}
extern global_var ImmediateModeGui* ImGui;

void imguiLoadDefaultStyle( ImmediateModeGui* gui, PlatformServices* platform )
{
	using namespace ImGuiTexCoords;
	auto style   = &gui->style;
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

	style->texCoords[ExpandBox]  = RectWH( 40 * itw, 0, 11 * itw, 11 * ith );
	style->texCoords[RetractBox] = RectWH( 28 * itw, 0, 11 * itw, 11 * ith );

	style->rects[RadioboxUnchecked] = {0, 0, 13, 13};
	style->rects[RadioboxChecked]   = {0, 0, 13, 13};
	style->rects[CheckboxUnchecked] = {0, 0, 13, 13};
	style->rects[CheckboxChecked]   = {0, 0, 13, 13};
	style->rects[SliderKnob]        = {0, 0, 13, 13};

	style->rects[DropGroupRetracted] = {0, 0, 13, 13};
	style->rects[DropGroupExpanded]  = {0, 0, 13, 13};

	style->rects[ComboButton] = {0, 0, 13, 13};

	style->rects[ExpandBox]  = {0, 0, 11, 11};
	style->rects[RetractBox] = {0, 0, 11, 11};
}

ImGuiContainerState defaultImGuiContainerSate( rectfarg rect )
{
	return {RectWH( rect.left, rect.top, ImGui->style.containerWidth, 0.0f )};
}
typedef int32 ImGuiContainerId;
ImGuiContainerId imguiGenerateContainer( ImmediateModeGui* gui, rectfarg rect = {},
                                         bool hidden = false )
{
	assert( gui->containersCount < ImGuiMaxContainers );
	auto result     = gui->containersCount;
	auto container  = &gui->containers[gui->containersCount];
	*container      = defaultImGuiContainerSate( {} );
	container->rect = rect;
	container->z    = safe_truncate< int8 >( gui->containersCount );
	++gui->containersCount;
	container->setHidden( hidden );
	return result;
}
ImGuiContainerState* imguiGetContainer( ImmediateModeGui* gui, int32 index )
{
	assert( index >= 0 && index < countof( gui->containers ) );
	return &gui->containers[index];
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
	ImGui->container              = 0;
	ImGui->renderCommandJumpFirst = nullptr;
	ImGui->renderCommandJumpLast  = nullptr;
	assert( ImGui->containersCount > 0 );
	ImGui->containers[0].addPosition = {};
	ImGui->containers[0].z = -1;
	FOR( container : ImGui->containers ) {
		container.bgMesh = nullptr;
		container.xMax   = 0;
	}

	if( ImGui->clearCapture ) {
		ImGui->capture      = {};
		ImGui->clearCapture = false;
	}
	if( ImGui->capture && isKeyUp( ImGui->inputs, ImGui->captureKey ) ) {
		assert( ImGui->captureKey );
		// clear capture delayed by one frame, so that capturing controls can process key up
		ImGui->clearCapture = true;
		ImGui->captureKey   = 0;
	}

	// set hover container
	auto mousePosition = ImGui->inputs->mouse.position;
	if( ImGui->capture ) {
		// if mouse is captured, treat the capturing container as the hover container
		ImGui->hoverContainer = ImGui->capture.container;
	} else {
		ImGui->hoverContainer = -1;
		if( auto font = ImGui->font ) {
			int8 z = -1;
			for( auto i = 0, count = ImGui->containersCount; i < count; ++i ) {
				auto container = &ImGui->containers[i];
				if( container->isHidden() ) {
					continue;
				}
				rectf rect;
				if( container->isMinimized() ) {
					rect = imguiGetTitlebar( container->rect );
				} else {
					rect = container->rect;
				}
				if( isPointInside( rect, mousePosition ) && container->z >= z ) {
					ImGui->hoverContainer = safe_truncate< int8 >( i );
					z                     = container->z;
				}
			}
		}
		auto combo = query_variant( ImGui->state, combobox );
		if( combo && ImGui->focus.type == ImGuiControlType::Combobox && combo->expanded
		    && isPointInside( combo->listRect, mousePosition ) ) {

			// the combo listRect might extend outside of its container, so we enforce it here
			ImGui->hoverContainer = ImGui->focus.container;
		}
	}
}
void imguiBind( ImmediateModeGui* guiState ) { ImGui = guiState; }
void imguiBind( ImmediateModeGui* guiState, RenderCommands* renderer, Font* font,
                GameInputs* inputs, void* base, rectfarg bounds, bool processInputs = true )
{
	ImGui                     = guiState;
	ImGui->processInputs      = processInputs;
	ImGui->renderer           = renderer;
	ImGui->inputs             = inputs;
	ImGui->font               = font;
	ImGui->base               = base;
	ImGui->bounds             = bounds;
	ImGui->containers[0].rect = bounds;
	imguiClear();
}

ImGuiHandle imguiMakeHandle( void* ptr, ImGuiControlType type = ImGuiControlType::None )
{
	return {( uint32 )( (uintptr)ptr - (uintptr)ImGui->base ), 0, type, ImGui->container};
}
ImGuiHandle imguiMakeHandle( void* ptr, ImGuiControlType type, int32 index )
{
	ImGuiHandle result = imguiMakeHandle( ptr, type );
	result.shortIndex = (uint8)index;
	return result;
}
ImGuiHandle imguiSetCurrentContainer( ImGuiContainerState* container )
{
	assert( container >= ImGui->containers && container < ImGui->containers + ImGuiMaxContainers );
	ImGuiHandle result = {};
	result.container   = safe_truncate< int8 >( container - ImGui->containers );
	ImGui->container   = result.container;
	return result;
}
ImGuiHandle imguiSetCurrentContainer( int32 index )
{
	return imguiSetCurrentContainer( imguiGetContainer( index ) );
}

ImGuiHandle imguiMakeIndexHandle( uint16 index )
{
	ImGuiHandle result = {};
	result.base        = 0;
	result.index       = index;
	result.container   = ImGui->container;
	result.flags       = ImGuiHandleFlags::Unbased;
	return result;
}
ImGuiHandle imguiMakeIndexHandle( void* ptr )
{
	ImGuiHandle result = {};
#ifdef ARCHITECTURE_X64
	result.base = ( uint32 )( ( (uintptr)ptr >> 32 ) ^ ( ( uintptr )ptr ) );
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
static void renderTextVCenteredClipped( RenderCommands* renderer, Font* font, StringView text,
                               const rectf& rect )
{
	ClippingRect clip( renderer, rect );

	reset( font );
	font->wrappingMode = WrappingMode::None;
	font->verticalAlign = FontVerticalAlign::Center;
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
bool imguiHasMyChildFocus( ImGuiHandle handle )
{
	auto focus        = ImGui->focus;
	focus.type        = {};
	focus.shortIndex  = 0;
	handle.type       = {};
	handle.shortIndex = 0;
	return focus == handle;
}
bool imguiHasCapture( ImGuiHandle handle ) { return ImGui->capture == handle; }
bool imguiIsHover( int32 container )
{
	return ( ImGui->processInputs )
	       && ( ( ImGui->modalContainer < 0 && ImGui->hoverContainer == container )
	            || ( ImGui->modalContainer == container ) );
}
bool imguiIsHover( ImGuiHandle handle )
{
	return imguiIsHover( handle.container );
}
void imguiCapture( ImGuiHandle handle, uint8 captureKey = KC_LButton )
{
	ImGui->capture        = handle;
	ImGui->captureKey     = captureKey;
	ImGui->hoverContainer = handle.container;
}
void imguiBringToFront( int8 container )
{
	if( container == 0 ) {
		return;
	}
	assert( container >= 0 && container < ImGui->containersCount );
	auto current = &ImGui->containers[container];
	auto z       = current->z;
	for( intmax i = 1, count = ImGui->containersCount; i < count; ++i ) {
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

void imguiShowModal( int32 containerIndex )
{
	auto container = imguiGetContainer( containerIndex );
	container->setModal( true );
	container->setHidden( false );
	container->setMinimized( false );
	ImGui->modalContainer = safe_truncate< int8 >( containerIndex );
	imguiBringToFront( ImGui->modalContainer );

	// center in screen
	container->rect = RectCentered( center( ImGui->bounds ), width( container->rect ),
	                                height( container->rect ) );
}
void imguiClose( int32 containerIndex )
{
	auto container = imguiGetContainer( containerIndex );
	container->setHidden( true );
	if( container->isModal() ) {
		container->setModal( false );
		ImGui->modalContainer = -1;
	}
}

void imguiRenderSortableBlock( int8 z )
{
	ImGui->renderCommandJumpLast =
	    addRenderCommandJump( ImGui->renderer, ImGui->renderCommandJumpLast, imguiToUserData( z ) );
	if( !ImGui->renderCommandJumpFirst ) {
		ImGui->renderCommandJumpFirst = ImGui->renderCommandJumpLast;
	}
}
void imguiEndContainer() { ImGui->container = 0; }

bool imguiDialog( StringView name, int32 containerIndex )
{
	auto container = imguiGetContainer( containerIndex );
	auto handle    = imguiSetCurrentContainer( container );
	if( container->isHidden() ) {
		return false;
	}
	if( container->isModal() ) {
		imguiBringToFront( handle.container );
	}

	auto style    = &ImGui->style;
	auto renderer = ImGui->renderer;
	auto font     = ImGui->font;
	auto inputs   = ImGui->inputs;

	if( width( container->rect ) <= 0 || height( container->rect ) <= 0 ) {
		// container is uninitialized
		auto flags       = container->flags;
		*container       = defaultImGuiContainerSate( container->rect );
		container->flags = flags;
		container->rect  = imguiGetTitlebar( container->rect );
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

	auto wasDragging = container->isDragging();
	if( isKeyPressed( inputs, KC_LButton ) && imguiIsHover( handle ) ) {
		imguiFocus( handle );
		if( isPointInside( draggableRect, inputs->mouse.position ) ) {
			imguiCapture( handle );
			ImGui->mouseOffset = inputs->mouse.position - rect.leftTop;
			// drag
			container->setDragging( true );
		}
	}
	if( imguiButton( closeButtonHandle, closeButton ) ) {
		container->setHidden( true );
		if( container->isModal() ) {
			container->setModal( false );
			ImGui->modalContainer = -1;
		}
		return false;
	} else if( imguiButton( minButtonHandle, minButton ) ) {
		container->setMinimized( !container->isMinimized() );
	}

	if( container->isMinimized() ) {
		rect.bottom     = titlebarRect.bottom;
		container->rect = rect;
	}

	if( isKeyDown( inputs, KC_LButton ) && imguiHasFocus( handle ) && imguiIsHover( handle ) ) {
		if( wasDragging ) {
			auto delta      = inputs->mouse.position - ImGui->mouseOffset - rect.leftTop;
			delta           = imguiBoundedTranslation( draggableRect, delta );
			container->rect = rect = translate( rect, delta );
			titlebarRect           = imguiGetTitlebar( container->rect );
		}
	} else {
		container->setDragging( false );
	}

	imguiRenderSortableBlock( container->z );
	auto prevColor   = renderer->color;
	auto color       = getColorF( renderer->color );
	auto bgColor     = multiplyComponents( vec4{.5f, 0, 0, 1}, color );
	auto buttonColor = multiplyComponents( vec4{1, 1, 1, 1}, color );
	auto textColor   = color;

	auto textRect = titlebarClient;
	textRect.right -= ( style->innerPadding + buttonSize ) * 2;

	setTexture( renderer, 0, null );
	if( container->isModal() ) {
		renderer->color = multiply( prevColor, 0x80000000 );
		addRenderCommandSingleQuad( renderer, ImGui->bounds );
	}
	auto meshCommand  = addRenderCommandMesh( renderer, 4, 6 );
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

	if( container->isMinimized() ) {
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
	auto container         = imguiCurrentContainer();
	auto canGrowHorizontal = ( container->flags & ImGuiContainerStateFlags::CanGrowHorizontal );
	if( !canGrowHorizontal ) {
		width = MIN( width, ::width( container->rect ) );
	}
	auto result    = RectWH( container->addPosition.x + ImGui->style.innerPadding,
	                         container->addPosition.y, width, height );
	if( result.right > container->rect.right ) {
		if( !canGrowHorizontal ) {
			// clamp result right since we can't grow horizontal
			result.right = container->rect.right;
		} else {
			container->rect.right = result.right;
		}
	}
	bool newline = false;
	if( container->horizontalCount > 0 ) {
		// change layout horizontally
		container->addPosition.x += width + ImGui->style.innerPadding;
		--container->horizontalCount;
		if( container->horizontalCount == 0 ) {
			newline                  = true;
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
	if( result.right > container->xMax ) {
		container->xMax = result.right;
	}
	container->lastRect = result;
	return result;
}
rectf imguiAddItemSameLine( float width, float height )
{
	auto container         = imguiCurrentContainer();
	container->addPosition.x = container->lastRect.right;
	container->addPosition.y = container->lastRect.top;
	auto result = imguiAddItem( width, height );
	return result;
}

void imguiSameLine( int32 count )
{
	auto container                = imguiCurrentContainer();
	container->horizontalCount    = count;
	container->maxHorizontalCount = count;
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
	auto font      = ImGui->font;
	auto width     = stringWidth( font, text ) + ImGui->style.innerPadding * 2;
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
		textArea.rightBottom += offset;
		renderTextCenteredClipped( renderer, font, name, textArea );
	}

	return imguiButton( handle, rect );
}

bool imguiButton( StringView name, ImGuiSize size = {} )
{
	size              = imguiMakeSize( size, ImGui->style.buttonWidth, ImGui->style.buttonHeight );
	auto stringHandle = imguiMakeStringHandle( name );
	return imguiButton( stringHandle.handle, stringHandle.string, size.width, size.height );
}
bool imguiButton( StringView name, float width, float height )
{
	auto stringHandle = imguiMakeStringHandle( name );
	return imguiButton( stringHandle.handle, stringHandle.string, width, height );
}
bool imguiButton( StringView name, float width )
{
	return imguiButton( name, width, ImGui->style.buttonHeight );
}

bool imguiPushButton( ImGuiHandle handle, StringView name, bool* pushed, float width, float height )
{
	assert( pushed );

	auto renderer = ImGui->renderer;
	auto font     = ImGui->font;

	handle.type = ImGuiControlType::PushButton;
	reset( font );
	auto textWidth  = stringWidth( font, name ) + ImGui->style.innerPadding * 2;
	auto textHeight = stringHeight( font, name ) + ImGui->style.innerPadding * 2;
	width           = MAX( textWidth, width );
	height          = MAX( textHeight, height );

	bool changed  = false;
	auto rect     = imguiAddItem( width, height );
	vec2 offset   = {};
	bool heldDown = imguiHasFocus( handle ) && isPointInside( rect, ImGui->inputs->mouse.position )
	                && isKeyDown( ImGui->inputs, KC_LButton );
	if( imguiButton( handle, rect ) ) {
		*pushed = !*pushed;
		changed = true;
	}
	bool pushVisual = ( ( *pushed ) != heldDown );
	if( pushVisual ) {
		offset = {1, 1};
	}

	RENDER_COMMANDS_STATE_BLOCK( renderer ) {
		setTexture( renderer, 0, null );
		if( pushVisual ) {
			renderer->color = multiply( renderer->color, 0xFF808080 );
		} else {
			renderer->color = multiply( renderer->color, ImGui->style.buttonBg );
		}
		MESH_STREAM_BLOCK( stream, renderer ) {
			pushQuad( stream, rect );
		}
		renderer->color = multiply( renderer->color, ImGui->style.buttonText );
		auto textArea   = imguiInnerRect( rect );
		textArea.leftTop += offset;
		textArea.rightBottom += offset;
		renderTextCenteredClipped( renderer, font, name, textArea );
	}

	return changed;
}
bool imguiPushButton( ImGuiHandle handle, StringView name, bool pushed, ImGuiSize size = {} )
{
	size = imguiMakeSize( size, ImGui->style.buttonWidth, ImGui->style.buttonHeight );
	return imguiPushButton( handle, name, &pushed, size.width, size.height );
}
bool imguiPushButton( StringView name, bool* pushed )
{
	auto handle = imguiMakeHandle( pushed, ImGuiControlType::PushButton );
	return imguiPushButton( handle, name, pushed, ImGui->style.buttonWidth,
	                        ImGui->style.buttonHeight );
}
bool imguiPushButton( StringView name, bool* pushed, float width, float height )
{
	auto handle = imguiMakeHandle( pushed, ImGuiControlType::PushButton );
	return imguiPushButton( handle, name, pushed, width, height );
}
bool imguiPushButton( StringView name, bool* pushed, float width )
{
	auto handle = imguiMakeHandle( pushed, ImGuiControlType::PushButton );
	return imguiPushButton( handle, name, pushed, width, ImGui->style.buttonHeight );
}

bool imguiKeypressButton( ImGuiHandle handle, rectfarg rect )
{
	auto inputs   = ImGui->inputs;
	if( isKeyPressed( inputs, KC_LButton ) && imguiIsHover( handle )
	    && isPointInside( rect, inputs->mouse.position ) ) {
		imguiFocus( handle );
		return true;
	}
	return false;
}

bool imguiIconButton( ImGuiHandle handle, float width, float height, TextureId icon,
                      const QuadTexCoords& texCoords = makeQuadTexCoordsDef(), float iconWidth = 0,
                      float iconHeight = 0 )
{
	auto renderer = ImGui->renderer;

	auto rect = imguiAddItem( width, height );
	vec2 offset = {};
	if( imguiHasFocus( handle ) && isPointInside( rect, ImGui->inputs->mouse.position )
	    && isKeyDown( ImGui->inputs, KC_LButton ) ) {
		offset = {1, 1};
	}

	auto iconRect = rect;
	if( iconWidth > 0 && iconHeight > 0 ) {
		iconRect = RectCentered( center( rect ), iconWidth, iconHeight );
	}
	setTexture( renderer, 0, icon );
	addRenderCommandSingleQuad( renderer, translate( iconRect, offset ), 0, texCoords );

	return imguiButton( handle, rect );
}

void imguiShowContainerAt( int32 containerIndex, vec2arg position )
{
	auto container  = imguiGetContainer( containerIndex );
	container->rect = RectWH( position, dimensions( container->rect ) );
	container->setHidden( false );
	container->setMinimized( false );
	imguiBringToFront( safe_truncate< int8 >( containerIndex ) );
}

bool imguiMenu( bool show )
{
	if( !show ) {
		return false;
	}
	auto style     = &ImGui->style;
	auto renderer  = ImGui->renderer;
	auto container = imguiCurrentContainer();

	auto padding = style->innerPadding;
	auto rect    = RectSetHeight( container->rect, 2 * padding + stringHeight( ImGui->font ) );
	// set container->lastRect so that imguiAddItemSameLine puts first item inside the menu
	container->lastRect.right = rect.left;
	container->lastRect.top   = rect.top;

	RENDER_COMMANDS_STATE_BLOCK( renderer ) {
		renderer->color = multiply( renderer->color, style->menuBg );
		setTexture( renderer, 0, null );
		addRenderCommandSingleQuad( renderer, rect );
	}

	return true;
}
bool imguiMenuItem( ImGuiHandle handle, StringView text, int32 containerIndex = -1 )
{
	handle.shortIndex = (uint8)containerIndex;

	auto style    = &ImGui->style;
	auto renderer = ImGui->renderer;
	auto font     = ImGui->font;
	auto inputs   = ImGui->inputs;

	auto width   = stringWidth( font, text );
	auto padding = style->innerPadding;

	auto rect = imguiAddItemSameLine( width + 4 * padding, 2 * padding + stringHeight( font ) );

	auto textColor  = renderer->color;
	bool menuActive = false;
	if( containerIndex >= 0 ) {
		auto container = imguiGetContainer( containerIndex );
		menuActive     = !container->isHidden();
	}

	auto hover = isPointInside( rect, inputs->mouse.position ) && imguiIsHover( handle );
	if( menuActive || hover ) {
		renderer->color = multiply( renderer->color, style->menuSelectionBg );
		setTexture( renderer, 0, null );
		addRenderCommandSingleQuad( renderer, rect );
	}
	auto textRect = rect;
	textRect.left += 2 *padding;
	renderer->color = textColor;
	renderTextVCenteredClipped( renderer, font, text, textRect );

	bool showMenu = false;
	if( hover && imguiHasMyChildFocus( handle ) ) {
		auto other = ImGui->focus;
		auto otherContainerIndex = other.shortIndex;
		if( otherContainerIndex < 255 && otherContainerIndex != containerIndex ) {
			auto container = imguiGetContainer( otherContainerIndex );
			container->setHidden( true );
		}
		showMenu = true;
		imguiFocus( handle );
	}
	if( showMenu || imguiKeypressButton( handle, rect ) ) {
		if( containerIndex >= 0 ) {
			imguiShowContainerAt( containerIndex, {rect.left, rect.bottom} );
			ImGui->processInputs = false;

			imguiFocus( handle );
		}
		return true;
	}

	if( imguiHasFocus( handle ) && !hover && isKeyPressed( inputs, KC_LButton ) ) {
		if( containerIndex >= 0 ) {
			auto container = imguiGetContainer( containerIndex );
			if( !isPointInside( container->rect, inputs->mouse.position ) ) {
				container->setHidden( true );
				imguiFocus( {} );
			}
		} else {
			imguiFocus( {} );
		}
	}

	return false;
}
bool imguiMenuItem( StringView text, int32 containerIndex = -1 )
{
	auto sh = imguiMakeStringHandle( text );
	return imguiMenuItem( sh.handle, sh.string, containerIndex );
}

bool imguiContextMenu( int32 containerIndex )
{
	auto container = imguiGetContainer( containerIndex );
	if( container->isHidden() ) {
		return false;
	}

	auto style    = &ImGui->style;
	auto renderer = ImGui->renderer;
	auto inputs   = ImGui->inputs;

	if( width( container->rect ) <= 0 || height( container->rect ) <= 0 ) {
		// container is uninitialized
		auto flags       = container->flags;
		*container       = defaultImGuiContainerSate( container->rect );
		container->flags = flags;
	}

	rectf rect = container->rect;

	if( ImGui->processInputs && isKeyPressed( inputs, KC_LButton )
	    && ( !isPointInside( rect, inputs->mouse.position ) || !imguiIsHover( containerIndex ) ) ) {

		container->setHidden( true );
		return false;
	}
	auto handle = imguiSetCurrentContainer( container );
	imguiBringToFront( handle.container );

	imguiRenderSortableBlock( container->z );
	auto bgColor = multiply( renderer->color, style->menuBg );

	setTexture( renderer, 0, null );
	auto meshCommand  = addRenderCommandMesh( renderer, 4, 6 );
	container->bgMesh = &meshCommand->mesh;
	{
		auto bgStream  = makeMeshStream( container->bgMesh );
		bgStream.color = bgColor;
		pushQuad( &bgStream, rect );
	}
	container->addPosition = {rect.left, rect.top + style->innerPadding};
	return true;
}
bool imguiContextMenuEntry( StringView text )
{
	auto handle    = imguiMakeStringHandle( text );
	auto container = imguiCurrentContainer();

	auto font     = ImGui->font;
	auto inputs   = ImGui->inputs;
	auto renderer = ImGui->renderer;
	auto style    = &ImGui->style;

	auto padding = ImGui->style.innerPadding;

	auto width  = ::width( container->rect ) - 2 * padding;
	auto height = stringHeight( font ) + 2 * padding;
	auto rect   = imguiAddItem( width, height );

	auto hovered = isPointInside( rect, inputs->mouse.position );

	if( hovered ) {
		RENDER_COMMANDS_STATE_BLOCK( renderer ) {
			setTexture( renderer, 0, null );
			renderer->color = multiply( renderer->color, style->menuSelectionBg );
			addRenderCommandSingleQuad( renderer, rect );
		}
	}

	auto inner = imguiInnerRect( rect );
	renderTextVCenteredClipped( renderer, font, handle.string, inner );

	if( imguiKeypressButton( handle.handle, rect ) ) {
		container->setHidden( true );
		return true;
	}
	return false;
}
void imguiShowContextMenu( int32 containerIndex, vec2arg position )
{
	imguiShowContainerAt( containerIndex, position );
}

bool imguiEditbox( ImGuiHandle handle, char* data, int32* length, int32 size, rectfarg editboxRect )
{
	assert( length );
	assert( *length <= size );

	auto renderer = ImGui->renderer;
	auto font     = ImGui->font;
	auto inputs   = ImGui->inputs;

	reset( font );
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

	if( imguiHasFocus( handle ) && ImGui->processInputs ) {
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
		if( ImGui->processInputs && ( backPressed || deletePressed ) ) {
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
		if( ImGui->processInputs ) {
			if( isKeyPressedRepeated( inputs, KC_Left ) ) {
				processCaretMove( -1 );
			} else if( isKeyPressedRepeated( inputs, KC_Right ) ) {
				processCaretMove( 1 );
			}
		}

		{
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
bool imguiEditbox( ImGuiHandle handle, StringView name, char* data, int32* length, int32 size,
                   float width, float height )
{
	auto renderer = ImGui->renderer;
	auto font     = ImGui->font;
	auto style    = &ImGui->style;

	auto nameWidth   = stringWidth( font, name );
	auto rect        = imguiAddItem( width + nameWidth + style->innerPadding, height );
	auto editboxRect = rect;
	editboxRect.left += nameWidth + style->innerPadding;
	editboxRect.right = editboxRect.left + width;

	RENDER_COMMANDS_STATE_BLOCK( renderer ) {
		renderer->color = multiply( renderer->color, style->name );
		renderTextClipped( renderer, font, name, rect );
	}

	return imguiEditbox( handle, data, length, size, editboxRect );
}
bool imguiEditbox( ImGuiHandle handle, char* data, int32* length, int32 size,
                   float width, float height )
{
	return imguiEditbox( handle, data, length, size, imguiAddItem( width, height ) );
}
bool imguiEditbox( StringView name, char* data, int32* length, int32 size, float width,
                   float height )
{
	return imguiEditbox( imguiMakeHandle( data, ImGuiControlType::Editbox ), name, data, length,
	                     size, width, height );
}

bool imguiEditbox( StringView name, char* data, int32* length, int32 size )
{
	return imguiEditbox( name, data, length, size, ImGui->style.editboxWidth,
	                     ImGui->style.editboxHeight );
}
bool imguiEditbox( StringView name, char* data, uint16* length, int32 size )
{
	int32 length32 = *length;
	if( imguiEditbox( name, data, &length32, size, ImGui->style.editboxWidth,
	                     ImGui->style.editboxHeight ) ) {
		*length = (uint8)length32;
		return true;
	}
	return false;
}

template< class T >
bool imguiEditbox( ImGuiHandle handle, StringView name, T* value )
{
	assert( value );
	handle.type = ImGuiControlType::Editbox;
	if( imguiHasFocus( handle ) ) {
		if( imguiEditbox( handle, name, ImGui->editboxStatic, &ImGui->editboxStaticCount,
		                  countof( ImGui->editboxStatic ), ImGui->style.editboxWidth,
		                  ImGui->style.editboxHeight ) ) {
			StringView view = {ImGui->editboxStatic, ImGui->editboxStaticCount};
			*value          = convert_to< T >( view );
			return true;
		}
		return false;
	} else {
		char buffer[100];
		auto len = snprint( buffer, countof( buffer ), "{}", *value );
		auto result = imguiEditbox( handle, name, buffer, &len, countof( buffer ),
		                            ImGui->style.editboxWidth, ImGui->style.editboxHeight );
		if( imguiHasFocus( handle ) ) {
			memcpy( ImGui->editboxStatic, buffer, len );
			ImGui->editboxStaticCount = len;
		}
		return result;
	}
}
template < class T >
bool imguiEditbox( ImGuiHandle handle, T* value,
                   const PrintFormat& initialFormatting = defaultPrintFormat() )
{
	assert( value );
	handle.type = ImGuiControlType::Editbox;
	if( imguiHasFocus( handle ) ) {
		if( imguiEditbox( handle, ImGui->editboxStatic, &ImGui->editboxStaticCount,
		                  countof( ImGui->editboxStatic ), ImGui->style.editboxWidth,
		                  ImGui->style.editboxHeight ) ) {
			StringView view = {ImGui->editboxStatic, ImGui->editboxStaticCount};
			*value          = convert_to< T >( view );
			return true;
		}
		return false;
	} else {
		char buffer[100];
		auto len = snprint( buffer, countof( buffer ), "{}", initialFormatting, *value );
		auto result = imguiEditbox( handle, buffer, &len, countof( buffer ),
		                            ImGui->style.editboxWidth, ImGui->style.editboxHeight );
		if( imguiHasFocus( handle ) ) {
			memcpy( ImGui->editboxStatic, buffer, len );
			ImGui->editboxStaticCount = len;
		}
		return result;
	}
}
template< class T >
bool imguiEditbox( StringView name, T* value )
{
	auto handle = imguiMakeHandle( value, ImGuiControlType::Editbox );
	return imguiEditbox( handle, name, value );
}
template < class T >
bool imguiEditbox( T* value, const PrintFormat& initialFormatting = defaultPrintFormat() )
{
	auto handle = imguiMakeHandle( value, ImGuiControlType::Editbox );
	return imguiEditbox( handle, value, initialFormatting );
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
		default: {
			break;
		}
	}
}

bool imguiCheckbox( StringView name, bool* checked )
{
	assert( checked );

	auto renderer = ImGui->renderer;
	auto font     = ImGui->font;
	auto style    = &ImGui->style;

	auto handle  = imguiMakeHandle( checked, ImGuiControlType::Checkbox );
	auto changed = false;

	using namespace ImGuiTexCoords;
	auto index = ( *checked ) ? ( CheckboxChecked ) : ( CheckboxUnchecked );

	auto innerPadding = style->innerPadding;
	auto width  = stringWidth( font, name ) + innerPadding * 3 + ::width( style->rects[index] );
	auto height = max( stringHeight( font ), ::height( style->rects[index] ) ) + innerPadding * 2;
	auto rect   = imguiAddItem( width, height );
	auto inner  = imguiInnerRect( rect );
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
		textArea.left += ::width( checkRect ) + innerPadding;
		renderTextCenteredClipped( renderer, font, name, textArea );
	}

	return changed;
}
bool imguiRadiobox( ImGuiHandle handle, StringView name )
{
	auto renderer  = ImGui->renderer;
	auto font      = ImGui->font;
	auto style     = &ImGui->style;
	auto container = imguiCurrentContainer();

	using namespace ImGuiTexCoords;
	auto checked = container->checkedRadiobox == handle;
	auto index   = ( checked ) ? ( RadioboxChecked ) : ( RadioboxUnchecked );

	auto innerPadding = style->innerPadding;
	auto width  = stringWidth( font, name ) + innerPadding * 3 + ::width( style->rects[index] );
	auto height = max( stringHeight( font ), ::height( style->rects[index] ) ) + innerPadding * 2;
	auto rect   = imguiAddItem( width, height );
	auto inner  = imguiInnerRect( rect );
	auto checkRect = translate( style->rects[index], inner.leftTop );

	if( imguiKeypressButton( handle, checkRect ) ) {
		checked                    = true;
		container->checkedRadiobox = handle;
	}

	RENDER_COMMANDS_STATE_BLOCK( renderer ) {
		setTexture( renderer, 0, style->atlas );
		MESH_STREAM_BLOCK( stream, renderer ) {
			pushQuad( stream, checkRect, 0, makeQuadTexCoords( style->texCoords[index] ) );
		}
		renderer->color = multiply( renderer->color, Color::White );
		auto textArea   = inner;
		textArea.left += ::width( checkRect ) + innerPadding;
		renderTextCenteredClipped( renderer, font, name, textArea );
	}

	return checked;
}
bool imguiRadiobox( StringView name )
{
	auto handle = imguiMakeStringHandle( name );
	return imguiRadiobox( handle.handle, handle.string );
}
bool imguiRadiobox( ImGuiHandle handle, StringView name, bool checked )
{
	auto container = imguiCurrentContainer();
	auto isChecked =
	    ( ( container->checkedRadiobox == handle ) || ( !container->checkedRadiobox && checked ) );
	auto changed = checked != isChecked;
	checked      = isChecked;
	if( !container->checkedRadiobox && checked ) {
		container->checkedRadiobox = handle;
	}

	if( imguiRadiobox( handle, name ) ) {
		if( !checked ) {
			changed                    = true;
			checked                    = true;
			container->checkedRadiobox = handle;
		}
	}
	return changed;
}
bool imguiRadiobox( StringView name, bool* checked )
{
	assert( checked );

	auto handle = imguiMakeHandle( checked, ImGuiControlType::Radiobox );
	return imguiRadiobox( handle, name, *checked );
}
bool imguiRadiobox( StringView name, bool checked )
{
	auto handle = imguiMakeStringHandle( name );
	return imguiRadiobox( handle.handle, handle.string, checked );
}

bool imguiBeginDropGroup( ImGuiHandle handle, StringView name, bool expanded )
{
	auto renderer  = ImGui->renderer;
	auto font      = ImGui->font;
	auto style     = &ImGui->style;
	auto container = imguiCurrentContainer();

	using namespace ImGuiTexCoords;
	auto index = ( expanded ) ? ( DropGroupExpanded ) : ( DropGroupRetracted );

	auto width = ::width( container->rect ) - style->innerPadding * 2;
	auto height =
	    max( stringHeight( font ), ::height( style->rects[index] ) ) + style->innerPadding * 2;
	auto rect      = imguiAddItem( width, height );
	auto inner     = imguiInnerRect( rect );
	auto buttonRect = translate( style->rects[index], inner.leftTop );

	if( imguiKeypressButton( handle, inner ) ) {
		expanded = !expanded;
	}
	if( expanded ) {
		container->rect.left += style->innerPadding;
		container->addPosition.x = container->rect.left;
		container->horizontalCount = 0;
	}

	RENDER_COMMANDS_STATE_BLOCK( renderer ) {
		renderer->color = multiply( renderer->color, Color::White );
		setTexture( renderer, 0, null );
		MESH_STREAM_BLOCK( stream, renderer ) {
			stream->color = multiply( renderer->color, 0x80544C9A );
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

	return expanded;
}
bool imguiBeginDropGroup( StringView name, bool* expanded )
{
	assert( expanded );
	auto handle = imguiMakeHandle( expanded, ImGuiControlType::DropGroup );
	*expanded = imguiBeginDropGroup( handle, name, *expanded );
	return *expanded;
}
bool imguiBeginDropGroup( StringView name, uint32* flags, uint32 flag )
{
	assert( flags );
	auto handle       = imguiMakeHandle( flags, ImGuiControlType::DropGroup );
	handle.shortIndex = (uint8)bitScanForward( flag );
	setFlagCond( *flags, flag, imguiBeginDropGroup( handle, name, ( *flags & flag ) != 0 ) );
	return ( *flags & flag ) != 0;
}
void imguiEndDropGroup()
{
	auto style     = &ImGui->style;
	auto container = imguiCurrentContainer();
	container->rect.left -= style->innerPadding;
	container->addPosition.x = container->rect.left;
	container->horizontalCount = 0;
}

static bool imguiPaddle( ImGuiHandle handle, float* value, float min, float max, float* innerValue,
                         float innerMin, float innerMax, int32 xComponent, float origin )
{
	if( imguiHasCapture( handle ) ) {
		auto inputs      = ImGui->inputs;
		auto mouseOffset = inputs->mouse.position.elements[xComponent] - origin;
		if( mouseOffset >= innerMin && mouseOffset <= innerMax ) {
			*innerValue = mouseOffset;
			*value = ( ( mouseOffset - innerMin ) / ( innerMax - innerMin ) ) * ( max - min ) + min;
		} else if( mouseOffset < innerMin ) {
			*value      = min;
			*innerValue = innerMin;
		} else {
			*value      = max;
			*innerValue = innerMax;
		}
		auto mm = minmax( min, max );
		*value = clamp( *value, mm.first, mm.second );
		return true;
	}
	return false;
}

// behaves like a slider but does not render anything
// use sliderRect to render custom slider paddle
struct ImGuiCustomSliderState {
	uint8 flags;
	rectf sliderRect;

	enum Flags : uint8 {
		Clicked = BITFIELD( 0 ),
		Changed = BITFIELD( 1 ),
	};
	bool changed() const { return ( flags & Changed ) != 0; }
	bool clicked() const { return ( flags & Clicked ) != 0; }
};
ImGuiCustomSliderState imguiCustomSlider( ImGuiHandle handle, float* value, float min, float max,
                                          float width, float paddleWidth, float paddleHeight,
                                          bool advance = true )
{
	assert( value );

	using namespace ImGuiTexCoords;

	auto inputs = ImGui->inputs;
	auto style  = &ImGui->style;

	auto container   = imguiCurrentContainer();
	auto height      = paddleHeight + style->innerPadding * 2;
	auto addPosition = container->addPosition;
	auto rect        = imguiAddItem( width, height );
	if( !advance ) {
		container->addPosition = addPosition;
	}
	auto inner = imguiInnerRect( rect );

	auto sliderArea = inner;
	auto mm         = minmax( min, max );
	*value          = clamp( *value, mm.first, mm.second );
	sliderArea.right -= paddleWidth;
	float paddlePosition = ( ( *value - min ) / ( max - min ) ) * ::width( sliderArea );

	ImGuiCustomSliderState result = {};
	result.sliderRect =
	    RectWH( sliderArea.left + paddlePosition, sliderArea.top, paddleWidth, paddleHeight );

	if( isKeyPressed( inputs, KC_LButton ) && imguiIsHover( handle )
	    && isPointInside( result.sliderRect, inputs->mouse.position ) ) {
		imguiFocus( handle );
		imguiCapture( handle );
		ImGui->mouseOffset = inputs->mouse.position - result.sliderRect.leftTop;
		result.flags |= ImGuiCustomSliderState::Clicked;
	}

	if( imguiPaddle( handle, value, min, max, &result.sliderRect.left, sliderArea.left,
	                 sliderArea.right, VectorComponent_X, ImGui->mouseOffset.x ) ) {
		result.flags |= ImGuiCustomSliderState::Changed;
		result.sliderRect.right = result.sliderRect.left + paddleWidth;
	}
	return result;
}
bool imguiScrollbar( ImGuiHandle handle, float* pos, float min, float max, float pageSize,
                     float stepSize, rectfarg rect, bool vertical )
{
	assert( pos );
	auto renderer = ImGui->renderer;
	auto inputs   = ImGui->inputs;
	auto style    = &ImGui->style;
	handle.type   = ImGuiControlType::Scrollbar;

	float w;
	rectf r;
	rectf leftButton;
	rectf rightButton;
	rectf paddle;
	float innerMin;
	float innerMax;
	if( !vertical ) {
		w = style->scrollWidth;
		r = rect;
		leftButton  = {rect.left, rect.top, rect.left + w, rect.bottom};
		rightButton = {rect.right - w, rect.top, rect.right, rect.bottom};
		innerMin = leftButton.right;
		innerMax = rightButton.left;
	} else {
		w = style->scrollHeight;
		// swap horizontal and vertical components
		r = swizzle( rect, RectComponent_Top, RectComponent_Left, RectComponent_Bottom,
		             RectComponent_Right );
		leftButton  = {rect.left, rect.top, rect.right, rect.top + w};
		rightButton = {rect.left, rect.bottom - w, rect.right, rect.bottom};
		innerMin = leftButton.bottom;
		innerMax = rightButton.top;
	}

	auto changed = false;
	if( imguiKeypressButton( handle, leftButton ) ) {
		*pos -= stepSize;
		changed = true;
	}
	if( imguiKeypressButton( handle, rightButton ) ) {
		*pos += stepSize;
		changed = true;
	}

	max -= pageSize;
	*pos                 = clamp( *pos, min, max );
	float innerWidth     = ::width( r ) - w * 2;
	float rangeWidth     = max - min;
	float paddleWidth    = ( pageSize / ( rangeWidth + pageSize ) ) * innerWidth;
	float remainingWidth = innerWidth - paddleWidth;
	float t              = ( *pos - min ) / rangeWidth;
	float paddleOffset   = t * remainingWidth;
	innerMax -= paddleWidth;
	paddle = {innerMin + paddleOffset, r.top, innerMin + paddleOffset + paddleWidth, r.bottom};

	auto mousePos = swizzle( inputs->mouse.position, VectorComponent_X + (int)vertical,
	                         VectorComponent_Y - (int)vertical );
	if( isKeyPressed( inputs, KC_LButton ) && imguiIsHover( handle )
	    && isPointInside( paddle, mousePos ) ) {

		imguiFocus( handle );
		imguiCapture( handle );
		ImGui->mouseOffset = mousePos - paddle.leftTop;
	}

	if( imguiPaddle( handle, pos, min, max, &paddle.left, innerMin, innerMax,
	                 ( vertical ) ? ( VectorComponent_Y ) : ( VectorComponent_X ),
	                 ImGui->mouseOffset.x ) ) {
		changed      = true;
		paddle.right = paddle.left + paddleWidth;
	}
	if( vertical ) {
		paddle = swizzle( paddle, RectComponent_Top, RectComponent_Left, RectComponent_Bottom,
		                  RectComponent_Right );
	}

	setTexture( renderer, 0, null );
	MESH_STREAM_BLOCK( stream, renderer ) {
		stream->color = multiply( renderer->color, Color::White );
		pushQuad( stream, leftButton );
		pushQuad( stream, rightButton );
		stream->color = multiply( renderer->color, Color::Black );
		pushQuad( stream, paddle );
	}

	return changed;
}
bool imguiScrollbar( float* pos, float min, float max, float pageSize, float stepSize,
                     rectfarg rect, bool vertical )
{
	return imguiScrollbar( imguiMakeHandle( pos, ImGuiControlType::Scrollbar ), pos, min, max,
	                       pageSize, stepSize, rect, vertical );
}
typedef StringView ImGuiGetTextFunctionType( const void* );
bool imguiListboxSingleSelect( ImGuiHandle handle, float* scrollPos, const void* items,
                               int32 entrySize, int32 itemsCount, int32* selectedIndex,
                               rectfarg bounds, ImGuiGetTextFunctionType* getText,
                               bool hasNoneEntry )
{
	auto renderer = ImGui->renderer;
	auto font     = ImGui->font;
	auto inputs   = ImGui->inputs;
	auto style    = &ImGui->style;

	handle.type = ImGuiControlType::Listbox;

	auto rect         = bounds;
	auto height       = ::height( rect );
	auto itemHeight   = stringHeight( font );
	bool scrollActive = false;
	rectf scrollRect  = {rect.right - style->scrollWidth, rect.top, rect.right, rect.bottom};

	auto first = (const char*)items;
	auto last  = first + entrySize * itemsCount;

	if( itemsCount * itemHeight > height ) {
		rect.right -= style->scrollWidth + style->innerPadding;
		scrollActive = true;
	} else {
		*scrollPos = 0;
	}
	bool changed = false;
	if( isKeyPressed( inputs, KC_LButton ) && imguiIsHover( handle )
	    && isPointInside( rect, inputs->mouse.position ) ) {
		imguiFocus( handle );
		imguiCapture( handle );
		// handle selection
		auto index = ( int32 )( ( inputs->mouse.position.y - rect.top + *scrollPos ) / itemHeight );
		if( hasNoneEntry ) {
			--index;
		}
		if( ( index >= 0 || ( hasNoneEntry && index >= -1 ) ) && index < itemsCount ) {
			changed        = true;
			*selectedIndex = index;
		}
	}
	if( scrollActive && imguiScrollbar( handle, scrollPos, 0, itemsCount * itemHeight - height,
	                                    itemHeight, itemHeight * 0.5f, scrollRect, true ) ) {
	}

	rectf itemsBg = translate( rect, 0, -( *scrollPos ) );
	auto clip     = ClippingRect( renderer, rect );
	setTexture( renderer, 0, null );
	MESH_STREAM_BLOCK( stream, renderer ) {
		stream->color = multiply( renderer->color, 0xFF443C2D );
		pushQuad( stream, rect );

		auto entryRect   = itemsBg;
		entryRect.bottom = entryRect.top + itemHeight;
		Color colors[]   = {{multiply( renderer->color, 0xFF342B1B )},
		                  {multiply( renderer->color, 0xFF5F594C )}};
		auto index    = 0;
		auto selected = *selectedIndex;
		if( hasNoneEntry ) {
			stream->color = colors[( int32 )( selected == -1 )];
			pushQuad( stream, entryRect );
			entryRect = translate( entryRect, 0, itemHeight );
		}
		for( auto it = first; it < last; it += entrySize, ++index ) {
			stream->color = colors[( int32 )( index == selected )];
			pushQuad( stream, entryRect );
			entryRect = translate( entryRect, 0, itemHeight );
		}
	}

	auto entryRect   = itemsBg;
	entryRect.bottom = entryRect.top + itemHeight;
	reset( font );
	RENDER_COMMANDS_STATE_BLOCK( renderer ) {
		renderer->color = multiply( renderer->color, Color::White );
		if( hasNoneEntry ) {
			renderText( renderer, font, "None", entryRect );
			entryRect = translate( entryRect, 0, itemHeight );
		}
		for( auto it = first; it < last; it += entrySize ) {
			auto text = getText( it );
			renderText( renderer, font, text, entryRect );
			entryRect = translate( entryRect, 0, itemHeight );
		}
	}

	return changed;
}
bool imguiCombo( ImGuiHandle handle, const void* items, int32 entrySize, int32 itemsCount,
                 int32* selectedIndex, rectfarg rect, ImGuiGetTextFunctionType* getText,
                 bool hasNoneEntry )
{
	assert( selectedIndex );
	using namespace ImGuiTexCoords;
	auto renderer = ImGui->renderer;
	auto font     = ImGui->font;
	auto style     = &ImGui->style;
	auto inputs    = ImGui->inputs;
	auto container = imguiCurrentContainer();

	handle.type = ImGuiControlType::Combobox;
	auto inner  = imguiInnerRect( rect );
	inner.left -= style->innerPadding;

	auto buttonWidth = ::width( style->rects[ComboButton] );
	auto buttonRect  = RectSetLeft( inner, inner.right - buttonWidth - style->innerPadding * 2 );
	buttonRect = alignCenter( buttonRect, buttonWidth, ::height( style->rects[ComboButton] ) );

	auto textRect             = imguiInnerRect( RectSetRight( inner, buttonRect.left ) );
	ImGuiComboboxState* state = nullptr;

	// we will treat children being focused as the combobox being focused
	auto focus = ImGui->focus;
	focus.type = ImGuiControlType::Combobox;
	auto hasFocus = ( handle == focus );

	bool justExpanded = false;
	if( imguiKeypressButton( handle, buttonRect ) ) {
		if( !hasFocus ) {
			set_variant( ImGui->state, combobox ) = {};
		}
		state = &get_variant( ImGui->state, combobox );
		imguiFocus( handle );
		state->expanded = !state->expanded;
		justExpanded = state->expanded;
	}
	if( hasFocus ) {
		state = &get_variant( ImGui->state, combobox );
	}

	bool changed = false;

	if( state && state->expanded ) {
		imguiRenderSortableBlock( ImGuiMaxValidZ );
		auto top        = rect.top + height( rect );
		state->listRect = {rect.left, top, rect.right, top + 100};
		if( imguiListboxSingleSelect( handle, &state->scrollPos, items, entrySize, itemsCount,
		                              selectedIndex, state->listRect, getText, hasNoneEntry ) ) {
			state->expanded = false;
			changed = true;
		}
		imguiRenderSortableBlock( container->z );
		if( isPointInside( state->listRect, inputs->mouse.position ) ) {
			// purposefully set hover container to -1 so that following controls do not receive
			// inputs
			ImGui->hoverContainer = -1;
		} else if( isKeyPressed( inputs, KC_LButton ) && !justExpanded ) {
			state->expanded = false;
		}
	}

	RENDER_COMMANDS_STATE_BLOCK( renderer ) {
		auto color      = renderer->color;
		renderer->color = multiply( color, 0x800000FF );
		setTexture( renderer, 0, null );
		addRenderCommandSingleQuad( renderer, inner );
		renderer->color = multiply( color, Color::White );
		setTexture( renderer, 0, style->atlas );
		addRenderCommandSingleQuad( renderer, buttonRect, 0,
		                            makeQuadTexCoords( style->texCoords[ComboButton] ) );
		auto selected = *selectedIndex;
		if( hasNoneEntry && selected == -1 ) {
			renderTextClipped( renderer, font, "None", inner );
		} else if( selected >= 0 && selected < itemsCount ) {
			auto text = getText( (const char*)items + entrySize * selected );
			renderTextClipped( renderer, font, text, inner );
		}
	}

	return changed;
}
bool imguiCombo( ImGuiHandle handle, int32* selectedIndex, const Array< const StringView > names,
                 rectfarg rect, bool hasNoneEntry = false )
{
	auto getText = []( const void* entry ) {
		assert_alignment( entry, alignof( const StringView ) );
		return *( (const StringView*)entry );
	};
	return imguiCombo( handle, names.data(), sizeof( const StringView ), names.size(),
	                   selectedIndex, rect, getText, hasNoneEntry );
}
bool imguiCombo( ImGuiHandle handle, const void* items, int32 entrySize, int32 itemsCount,
                 int32* selectedIndex, ImGuiGetTextFunctionType* getText, bool hasNoneEntry )
{
	using namespace ImGuiTexCoords;
	auto font      = ImGui->font;
	auto container = imguiCurrentContainer();
	auto height    = max( stringHeight( font ), ::height( ImGui->style.rects[ComboButton] ) )
	              + ImGui->style.innerPadding * 2;
	auto rect = imguiAddItem( width( container->rect ), height );
	return imguiCombo( handle, items, entrySize, itemsCount, selectedIndex, rect, getText,
	                   hasNoneEntry );
}
bool imguiCombo( ImGuiHandle handle, int32* selectedIndex, const Array< const StringView > names,
                 bool hasNoneEntry = false )
{
	using namespace ImGuiTexCoords;
	auto font      = ImGui->font;
	auto container = imguiCurrentContainer();
	auto height    = max( stringHeight( font ), ::height( ImGui->style.rects[ComboButton] ) )
	              + ImGui->style.innerPadding * 2;
	auto rect = imguiAddItem( width( container->rect ), height );
	return imguiCombo( handle, selectedIndex, names, rect, hasNoneEntry );
}
bool imguiCombo( int32* selectedIndex, const Array< const StringView > names,
                 bool hasNoneEntry = false )
{
	using namespace ImGuiTexCoords;
	auto font      = ImGui->font;
	auto container = imguiCurrentContainer();
	auto handle    = imguiMakeHandle( selectedIndex, ImGuiControlType::Combobox );
	auto height    = max( stringHeight( font ), ::height( ImGui->style.rects[ComboButton] ) )
	              + ImGui->style.innerPadding * 2;
	auto rect = imguiAddItem( width( container->rect ), height );
	return imguiCombo( handle, selectedIndex, names, rect, hasNoneEntry );
}

ImGuiCustomSliderState imguiCustomSlider( float* value, float min, float max, float width,
                                          float paddleWidth, float paddleHeight,
                                          bool advance = true )
{
	auto handle = imguiMakeHandle( value, ImGuiControlType::Slider );
	return imguiCustomSlider( handle, value, min, max, width, paddleWidth, paddleHeight, advance );
}

bool imguiSlider( ImGuiHandle handle, float* value, float min, float max, rectfarg rect )
{
	using namespace ImGuiTexCoords;

	auto renderer = ImGui->renderer;
	auto inputs   = ImGui->inputs;
	auto style    = &ImGui->style;
	handle.type   = ImGuiControlType::Slider;

	auto changed     = false;
	auto mm          = minmax( min, max );
	*value           = clamp( *value, mm.first, mm.second );
	auto paddleWidth = ::width( style->rects[SliderKnob] );
	auto sliderArea  = rect;
	sliderArea.right -= paddleWidth;
	float paddlePosition = ( ( *value - min ) / ( max - min ) ) * ::width( sliderArea );
	rectf paddle =
	    translate( style->rects[SliderKnob], sliderArea.left + paddlePosition, sliderArea.top );

	if( isKeyPressed( inputs, KC_LButton ) && imguiIsHover( handle )
	    && isPointInside( paddle, inputs->mouse.position ) ) {
		imguiFocus( handle );
		imguiCapture( handle );
		ImGui->mouseOffset = inputs->mouse.position - paddle.leftTop;
	}

	if( imguiPaddle( handle, value, min, max, &paddle.left, sliderArea.left, sliderArea.right,
	                 VectorComponent_X, ImGui->mouseOffset.x ) ) {
		changed      = true;
		paddle.right = paddle.left + paddleWidth;
	}

	RENDER_COMMANDS_STATE_BLOCK( renderer ) {
		setTexture( renderer, 0, style->atlas );
		MESH_STREAM_BLOCK( stream, renderer ) {
			pushQuad( stream, paddle, 0, makeQuadTexCoords( style->texCoords[SliderKnob] ) );
		}
	}

	return changed;
}

bool imguiSlider( ImGuiHandle handle, float* value, float min, float max )
{
	assert( value );
	using namespace ImGuiTexCoords;

	auto style     = &ImGui->style;
	auto container = imguiCurrentContainer();

	auto width  = ::width( container->rect );
	auto height = ::height( style->rects[SliderKnob] ) + style->innerPadding * 2;
	auto rect   = imguiAddItem( width, height );
	auto inner  = imguiInnerRect( rect );
	return imguiSlider( handle, value, min, max, inner );
}
bool imguiSlider( float* value, float min, float max )
{
	auto handle = imguiMakeHandle( value, ImGuiControlType::Slider );
	return imguiSlider( handle, value, min, max );
}
bool imguiSlider( StringView name, float* value, float min, float max )
{
	assert( value );

	using namespace ImGuiTexCoords;

	auto renderer  = ImGui->renderer;
	auto font      = ImGui->font;
	auto style     = &ImGui->style;
	auto container = imguiCurrentContainer();

	auto handle  = imguiMakeHandle( value, ImGuiControlType::Slider );

	auto width  = ::width( container->rect );
	auto height = ::max( stringHeight( font ), ::height( style->rects[SliderKnob] ) )
	              + style->innerPadding * 2;
	auto rect       = imguiAddItem( width, height );
	auto inner      = imguiInnerRect( rect );
	auto sliderArea = inner;
	sliderArea.left += stringWidth( font, name ) + style->innerPadding;
	RENDER_COMMANDS_STATE_BLOCK( renderer ) {
		auto textArea   = inner;
		renderer->color = multiply( renderer->color, Color::White );
		renderTextClipped( renderer, font, name, textArea );
	}
	return imguiSlider( handle, value, min, max, sliderArea );
}

rectf imguiRegion( float width, float height )
{
	return imguiAddItem( width, height );
}

static rectf imguiGetPointRect( float valueX, float valueY, rectfarg valueDomain, rectfarg rect,
                                rectfarg pointBounds = {-5, -5, 5, 5} )
{
	float innerWidth  = width( rect );
	float innerHeight = height( rect );
	float rangeWidth  = width( valueDomain );
	float rangeHeight = height( valueDomain );
	float tx          = ( valueX - valueDomain.left ) / rangeWidth;
	float ty          = ( valueY - valueDomain.top ) / rangeHeight;
	vec2 offset       = {tx * innerWidth, ty * innerHeight};

	auto point   = rect.leftTop + offset;
	return translate( pointBounds, point );
}
bool imguiPoint( ImGuiHandle handle, float* valueX, float* valueY, rectfarg valueDomain,
                 rectfarg rect, bool doInput, rectf* pointRect,
                 rectfarg pointBounds = {-5, -5, 5, 5} )
{
	auto inputs  = ImGui->inputs;
	auto changed = false;
	*pointRect   = imguiGetPointRect( *valueX, *valueY, valueDomain, rect, pointBounds );
	vec2 point   = {pointRect->left - pointBounds.left, pointRect->top - pointBounds.top};

	if( doInput ) {
		if( isKeyPressed( inputs, KC_LButton ) && imguiIsHover( handle )
		    && isPointInside( *pointRect, inputs->mouse.position ) ) {

			imguiFocus( handle );
			imguiCapture( handle );
			ImGui->mouseOffset = inputs->mouse.position - point;
		}

		if( imguiPaddle( handle, valueX, valueDomain.left, valueDomain.right, &point.x, rect.left,
		                 rect.right, VectorComponent_X, ImGui->mouseOffset.x ) ) {
			changed          = true;
			pointRect->left  = point.x + pointBounds.left;
			pointRect->right = point.x + pointBounds.right;
		}
		if( imguiPaddle( handle, valueY, valueDomain.top, valueDomain.bottom, &point.y, rect.top,
		                 rect.bottom, VectorComponent_Y, ImGui->mouseOffset.y ) ) {
			changed           = true;
			pointRect->top    = point.y + pointBounds.top;
			pointRect->bottom = point.y + pointBounds.bottom;
		}
	}
	return changed;
}

bool imguiPoint( vec2* value, rectfarg valueDomain, rectfarg rect,
                 rectfarg pointBounds = {-5, -5, 5, 5}, rectf* pointRectOut = nullptr )
{
	auto handle = imguiMakeHandle( value, ImGuiControlType::Point );
	*value      = clamp( *value, correct( valueDomain ) );
	rectf pointRect;
#if 0
	auto doInput =
	    isPointInside( rect, ImGui->inputs->mouse.position ) || imguiHasCapture( handle );
#else
	auto doInput = true;
#endif
	auto result = imguiPoint( handle, &value->x, &value->y, valueDomain, rect, doInput, &pointRect,
	                          pointBounds );

	auto renderer = ImGui->renderer;
	setTexture( renderer, 0, null );
	LINE_MESH_STREAM_BLOCK( stream, renderer ) {
		// stream->color = multiply( renderer->color, Color::White );
		pushQuadOutline( stream, pointRect );
	}
	if( pointRectOut ) {
		*pointRectOut = pointRect;
	}
	return result;
}

bool imguiRect( rectf* value, rectfarg valueDomain, rectfarg rect )
{
	*value      = RectMin( *value, valueDomain );
	auto handle = imguiMakeHandle( value, ImGuiControlType::Rect );
	rectf rightBottomPoint;
	rectf leftBottomPoint;
	rectf rightTopPoint;
	rectf leftTopPoint;
	auto changed        = false;
#if 0
	auto captured       = ImGui->capture;
	captured.shortIndex = 0;
	auto doInput = isPointInside( rect, ImGui->inputs->mouse.position ) || ( captured == handle );
#else
	auto doInput = true;
#endif
	handle.shortIndex = 3;
	if( imguiPoint( handle, &value->right, &value->bottom, valueDomain, rect, doInput,
	                &rightBottomPoint ) ) {
		changed = true;
		doInput = false;
	}
	handle.shortIndex = 2;
	if( imguiPoint( handle, &value->left, &value->bottom, valueDomain, rect, doInput,
	                &leftBottomPoint ) ) {
		changed = true;
		doInput = false;
	}
	handle.shortIndex = 1;
	if( imguiPoint( handle, &value->right, &value->top, valueDomain, rect, doInput,
	                &rightTopPoint ) ) {
		changed = true;
		doInput = false;
	}
	handle.shortIndex = 0;
	if( imguiPoint( handle, &value->left, &value->top, valueDomain, rect, doInput,
	                &leftTopPoint ) ) {
		changed = true;
		doInput = false;
	}

	rightBottomPoint = imguiGetPointRect( value->left, value->top, valueDomain, rect );
	leftBottomPoint  = imguiGetPointRect( value->right, value->top, valueDomain, rect );
	rightTopPoint    = imguiGetPointRect( value->left, value->bottom, valueDomain, rect );
	leftTopPoint     = imguiGetPointRect( value->right, value->bottom, valueDomain, rect );
	auto renderer    = ImGui->renderer;
	setTexture( renderer, 0, null );
	LINE_MESH_STREAM_BLOCK( stream, renderer ) {
		// stream->color = multiply( renderer->color, Color::Black );
		pushQuadOutline( stream, rightBottomPoint );
		pushQuadOutline( stream, leftBottomPoint );
		pushQuadOutline( stream, rightTopPoint );
		pushQuadOutline( stream, leftTopPoint );
	}
	return changed;
}

RenderCommands* imguiRenderer() { return ImGui->renderer; }
GameInputs* imguiInputs() { return ImGui->inputs; }

struct ImGuiBeginColumnResult {
	rectf rect;
	float lastColumnWidth;
	float x;
	float y;
};
ImGuiBeginColumnResult imguiBeginColumn( float width )
{
	auto container                = imguiCurrentContainer();
	ImGuiBeginColumnResult result = {container->rect, width, container->addPosition.x,
	                                 container->addPosition.y};
	container->rect = RectSetWidth( container->rect, width );
	return result;
}
void imguiFitContainerToSize()
{
	auto container        = imguiCurrentContainer();
	container->rect.right = container->xMax;
}
void imguiNextColumn( ImGuiBeginColumnResult* columns, float width )
{
	auto container           = imguiCurrentContainer();
	auto lastColumnWidth     = columns->lastColumnWidth;
	columns->lastColumnWidth = width;
	container->rect          = columns->rect;
	container->rect.left     = columns->x + lastColumnWidth + ImGui->style.innerPadding;
	container->rect.right    = container->rect.left + width;
	container->addPosition.x = container->rect.left;
	container->addPosition.y = columns->y;
	container->rect          = RectSetWidth( container->rect, width );
	columns->x               = container->rect.left;
}
void imguiEndColumn( ImGuiBeginColumnResult* columns )
{
	auto container           = imguiCurrentContainer();
	container->rect          = columns->rect;
	container->rect.top      = container->addPosition.y;
	container->addPosition   = container->rect.leftTop;
	columns->lastColumnWidth = 0;
	columns->x               = container->addPosition.x;
	columns->y               = container->addPosition.y;
}

struct ImGuiListboxItem {
	StringView text;
	bool selected;
};
// scary looking class, but it allows typesafe iteration over ImGuiListboxItem members of entries of
// an array. The constructors are typesafe and convert the arguments into type erased array and
// getter. This way only the constructor of this class needs to be templated instead of making
// imguiListboxIntrusive into a templated function
struct ImGuiListboxItemView {
	char* ptr;
	uint16 entrySize;
	int32 sz;
	void* state;
	ImGuiListboxItem& (*get)( void*, void* );

	template < class T, class Func >
	ImGuiListboxItemView( Array< T > array, Func&& func )
	: ptr( (char*)array.data() ), entrySize( sizeof( T ) ), sz( array.size() ), state( &func )
	{
		get = []( void* functor, void* entry ) -> ImGuiListboxItem& {
			assert_alignment( entry, alignof( T ) );
			assert_alignment( functor, alignof( Func ) );
			return ( *(std::add_pointer_t< Func >)functor )( *(T*)entry );
		};
	}

	ImGuiListboxItemView( Array< ImGuiListboxItem > array )
	: ptr( (char*)array.data() ),
	  entrySize( sizeof( ImGuiListboxItem ) ),
	  sz( array.size() ),
	  state( nullptr )
	{
		get = []( void*, void* entry ) -> ImGuiListboxItem& {
			assert_alignment( entry, alignof( ImGuiListboxItem ) );
			return *(ImGuiListboxItem*)entry;
		};
	}

	struct iterator {
		char* ptr;
		uint16 entrySize;
		void* state;
		ImGuiListboxItem& ( *get )( void*, void* );

		iterator& operator++()
		{
			ptr += entrySize;
			return *this;
		}
		iterator operator++( int )
		{
			auto ret = *this;
			ptr += entrySize;
			return ret;
		}
		ImGuiListboxItem& operator*() { return get( state, ptr ); }
		bool operator!=( iterator other ) { return this->ptr != other.ptr; }
		bool operator==( iterator other ) { return this->ptr == other.ptr; }
	};

	iterator begin() const { return iterator{ptr, entrySize, state, get}; }
	iterator end() const { return iterator{ptr + sz * entrySize, entrySize, state, get}; }

	ImGuiListboxItem& operator[]( int32 index ) const
	{
		assert( index >= 0 && index < sz );
		auto p = ptr + entrySize * index;
		return get( state, p );
	}

	int32 size() const { return sz; }
};
int32 imguiListboxIntrusive( float* scrollPos, const ImGuiListboxItemView& items, float width,
                             float height, bool multiselect = true )
{
	auto handle   = imguiMakeHandle( scrollPos, ImGuiControlType::Listbox );
	auto renderer = ImGui->renderer;
	auto font     = ImGui->font;
	auto inputs   = ImGui->inputs;
	auto style    = &ImGui->style;

	int32 selectedIndex = -1;
	auto rect           = imguiAddItem( width, height );
	auto itemHeight     = stringHeight( font );
	bool scrollActive   = false;
	rectf scrollRect    = {rect.right - style->scrollWidth, rect.top, rect.right, rect.bottom};

	const auto itemsCount = items.size();
	if( itemsCount * itemHeight > height ) {
		rect.right -= style->scrollWidth + style->innerPadding;
		scrollActive = true;
	} else {
		*scrollPos = 0;
	}
	if( isKeyPressed( inputs, KC_LButton ) && imguiIsHover( handle )
	    && isPointInside( rect, inputs->mouse.position ) ) {
		imguiFocus( handle );
		imguiCapture( handle );
		// handle selection
		auto index = ( int32 )( ( inputs->mouse.position.y - rect.top + *scrollPos ) / itemHeight );
		if( index >= 0 && index < itemsCount ) {
			if( multiselect && isKeyDown( inputs, KC_Control ) ) {
				auto entry = &items[index];
				entry->selected = !entry->selected;
			} else {
				FOR( entry : items ) {
					entry.selected = false;
				}
				items[index].selected = true;
			}
			selectedIndex = index;
		}
	}
	if( scrollActive && imguiScrollbar( scrollPos, 0, itemsCount * itemHeight - height, height,
	                                    itemHeight, scrollRect, true ) ) {
	}

	rectf itemsBg = translate( rect, 0, -( *scrollPos ) );
	auto clip     = ClippingRect( renderer, rect );
	setTexture( renderer, 0, null );
	MESH_STREAM_BLOCK( stream, renderer ) {
		stream->color = multiply( renderer->color, 0xFF443C2D );
		pushQuad( stream, rect );

		auto entryRect   = itemsBg;
		entryRect.bottom = entryRect.top + itemHeight;
		Color colors[]   = {{multiply( renderer->color, 0xFF342B1B )},
		                  {multiply( renderer->color, 0xFF5F594C )}};
		FOR( entry : items ) {
			stream->color = colors[(int32)entry.selected];
			pushQuad( stream, entryRect );
			entryRect = translate( entryRect, 0, itemHeight );
		}
	}

	auto entryRect   = itemsBg;
	entryRect.bottom = entryRect.top + itemHeight;
	reset( font );
	RENDER_COMMANDS_STATE_BLOCK( renderer ) {
		renderer->color = multiply( renderer->color, Color::White );
		FOR( entry : items ) {
			renderText( renderer, font, entry.text, entryRect );
			entryRect = translate( entryRect, 0, itemHeight );
		}
	}

	return selectedIndex;
}
int32 imguiListbox( float* scrollPos, Array< ImGuiListboxItem > items, float width, float height,
                    bool multiselect = true )
{
	return imguiListboxIntrusive( scrollPos, items, width, height, multiselect );
}

rectf imguiScrollable( vec2* scrollPos, rectfarg scrollDomain, float width, float height,
                       float stepSize = 10 )
{
	auto style         = &ImGui->style;
	bool hscrollActive = ::width( scrollDomain ) > width;
	bool vscrollActive = ::height( scrollDomain ) > height;

	auto rect = imguiAddItem( width, height );
	if( hscrollActive ) {
		rectf hscrollRect = {rect.left, rect.bottom - style->scrollHeight, rect.right, rect.bottom};
		if( vscrollActive ) {
			hscrollRect.right -= style->scrollWidth + style->innerPadding;
		}
		rect.bottom -= style->scrollHeight + style->innerPadding;
		imguiScrollbar( &scrollPos->x, scrollDomain.left, scrollDomain.right, width, stepSize,
		                hscrollRect, false );
	} else {
		scrollPos->x = 0;
	}
	if( vscrollActive ) {
		rectf vscrollRect = {rect.right - style->scrollWidth, rect.top, rect.right, rect.bottom};
		if( hscrollActive ) {
			vscrollRect.bottom -= style->innerPadding;
		}
		rect.right -= style->scrollWidth + style->innerPadding;
		imguiScrollbar( &scrollPos->y, scrollDomain.top, scrollDomain.bottom, height, stepSize,
		                vscrollRect, true );
	} else {
		scrollPos->y = 0;
	}

	return rect;
}

struct ImGuiScrollableRegion {
	vec2 scrollPos;
	vec2 dim;
};
struct ImGuiScrollableRegionResult {
	rectf inner;
	rectf prevRect;
	char* clippingStart;
	bool8 wasProcessingInputs;
	bool8 wasGrowingHorizontal;
};
ImGuiScrollableRegionResult imguiBeginScrollableRegion( ImGuiScrollableRegion* region, float width,
                                                        float height, bool allowHorizontal = false )
{
	assert( region );
	auto handle    = imguiMakeHandle( region, ImGuiControlType::ScrollableRegion );
	auto container = imguiCurrentContainer();
	auto inputs    = ImGui->inputs;

	ImGuiScrollableRegionResult result = {};
	auto clipWidth                     = width;
	auto clipHeight                    = height;
	if( allowHorizontal && region->dim.x > width - ImGui->style.innerPadding * 2 ) {
		clipHeight -= ImGui->style.scrollHeight + ImGui->style.innerPadding;
	} else {
		region->scrollPos.x = 0;
	}
	if( region->dim.y > clipHeight - ImGui->style.innerPadding * 2 ) {
		clipWidth -= ImGui->style.scrollWidth + ImGui->style.innerPadding;
	} else {
		region->scrollPos.y = 0;
	}
	auto rect                  = imguiAddItem( clipWidth, clipHeight );
	result.inner               = imguiInnerRect( rect );
	result.prevRect            = container->rect;
	result.clippingStart       = back( ImGui->renderer );
	result.wasProcessingInputs = ImGui->processInputs;
	result.wasGrowingHorizontal =
	    isFlagSet( container->flags, ImGuiContainerStateFlags::CanGrowHorizontal );
	setFlagCond( container->flags, ImGuiContainerStateFlags::CanGrowHorizontal, allowHorizontal );

	container->rect.right  = result.inner.right;
	container->rect.left   = result.inner.left - region->scrollPos.x;
	container->rect.top    = result.inner.top - region->scrollPos.y;
	container->rect.bottom = container->rect.top;
	container->addPosition = container->rect.leftTop;

	if( imguiIsHover( handle ) && !ImGui->capture
	    && !isPointInside( rect, inputs->mouse.position ) ) {

		ImGui->processInputs = false;
	}
	return result;
}
void imguiEndScrollableRegion( ImGuiScrollableRegion* region, ImGuiScrollableRegionResult* state,
                               float stepSize = 10 )
{
	assert( region );
	assert( state );
	auto container = imguiCurrentContainer();
	auto renderer  = ImGui->renderer;

	RenderCommandsStream stream = {state->clippingStart,
	                               ( size_t )( back( renderer ) - state->clippingStart )};
	clip( stream, state->inner );

#if 0
	RENDER_COMMANDS_STATE_BLOCK( renderer ) {
	    renderer->color = Color::Black;
	    setTexture( renderer, 0, null );
	    addRenderCommandSingleQuad( renderer, container->rect );
	}
#endif

	auto containerHeight = height( container->rect );
	auto containerWidth = width( container->rect );
	auto innerHeight = height( state->inner );
	auto innerWidth = width( state->inner );
	ImGui->processInputs   = state->wasProcessingInputs;
	setFlagCond( container->flags, ImGuiContainerStateFlags::CanGrowHorizontal,
	             state->wasGrowingHorizontal );
	if( containerWidth > innerWidth ) {
		rectf scrollRect = state->inner;
		scrollRect.top   = scrollRect.bottom;
		scrollRect.bottom += ImGui->style.scrollHeight;
		imguiScrollbar( &region->scrollPos.x, 0, containerWidth, innerWidth, stepSize, scrollRect,
		                false );
	} else {
		region->scrollPos.y = 0;
	}
	if( containerHeight > innerHeight ) {
		rectf scrollRect = state->inner;
		scrollRect.left  = scrollRect.right;
		scrollRect.right += ImGui->style.scrollWidth;
		imguiScrollbar( &region->scrollPos.y, 0, containerHeight, innerHeight, stepSize, scrollRect,
		                true );
	} else {
		region->scrollPos.y = 0;
	}
	region->dim.x          = width( container->rect );
	region->dim.y          = containerHeight;
	container->rect        = state->prevRect;
	container->addPosition = container->rect.leftTop;
}

void imguiSeperator()
{
	auto renderer = imguiRenderer();
	auto container = imguiCurrentContainer();

	auto innerPadding = ImGui->style.innerPadding;
	auto rect = imguiAddItem( width( container->rect ) - innerPadding * 2, innerPadding * 2 );
	setTexture( renderer, 0, null );
	LINE_MESH_STREAM_BLOCK( stream, renderer ) {
		stream->color = multiply( stream->color, Color::White );
		pushLine( stream, {rect.left + innerPadding, rect.top + innerPadding},
		          {rect.right - innerPadding, rect.top + innerPadding} );
	}
}

void imguiFinalize()
{
	if( ImGui->renderCommandJumpFirst ) {
		auto renderer                = ImGui->renderer;
		ImGui->renderCommandJumpLast = addRenderCommandJump(
		    renderer, ImGui->renderCommandJumpLast, imguiToUserData( ImGuiMaxZ ) );
		sortRenderCommandJumps( renderer, ImGui->renderCommandJumpFirst,
		                        ImGui->renderCommandJumpLast,
		                        []( const RenderCommandJump& a, const RenderCommandJump& b ) {
			                        return imguiToZ( a.userData ) < imguiToZ( b.userData );
			                    } );
	}
}