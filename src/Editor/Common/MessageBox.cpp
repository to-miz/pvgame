struct AppData;
namespace EditorCommon
{
typedef void MessageBoxAction( AppData* );
struct MessageBox {
	int32 container;
	enum { OkCancel, YesNoCancel } type;
	short_string< 100 > text;
	short_string< 50 > title;

	struct {
		MessageBoxAction* onYes;
		MessageBoxAction* onNo;

		void* data;
	} action;
};

void showMessageBox( MessageBox* messageBox, StringView text, StringView title,
                     EditorCommon::MessageBoxAction* onYes,
                     EditorCommon::MessageBoxAction* onNo = nullptr )
{
	messageBox->text         = text;
	messageBox->title        = title;
	messageBox->action.onYes = onYes;
	messageBox->action.onNo  = onNo;
	messageBox->type = ( onYes && onNo ) ? ( MessageBox::YesNoCancel ) : ( MessageBox::OkCancel );
	imguiShowModal( messageBox->container );
}

void handleMessageBox( AppData* app, GameInputs* inputs, const MessageBox& messageBox )
{
	if( imguiDialog( messageBox.title, messageBox.container ) ) {
		if( isKeyPressed( inputs, KC_Return ) ) {
			if( messageBox.action.onYes ) {
				messageBox.action.onYes( app );
			}
			imguiClose( messageBox.container );
		} else if( isKeyPressed( inputs, KC_Escape ) ) {
			imguiClose( messageBox.container );
		} else {
			imguiText( messageBox.text );
			switch( messageBox.type ) {
				case MessageBox::OkCancel: {
					imguiSameLine( 2 );
					if( imguiButton( "Ok" ) ) {
						if( messageBox.action.onYes ) {
							messageBox.action.onYes( app );
						}
						imguiClose( messageBox.container );
					}
					if( imguiButton( "Cancel" ) ) {
						imguiClose( messageBox.container );
					}
					break;
				}
				case MessageBox::YesNoCancel: {
					imguiSameLine( 3 );
					if( imguiButton( "Yes" ) ) {
						if( messageBox.action.onYes ) {
							messageBox.action.onYes( app );
						}
						imguiClose( messageBox.container );
					}
					if( imguiButton( "No" ) ) {
						if( messageBox.action.onNo ) {
							messageBox.action.onNo( app );
						}
						imguiClose( messageBox.container );
					}
					if( imguiButton( "Cancel" ) ) {
						imguiClose( messageBox.container );
					}
					break;
				}
			}
		}
	}
}
}