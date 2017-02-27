mat4 getEditorViewTransform( EditorView* editor )
{
	return matrixRotationY( editor->rotation.x ) * matrixRotationX( editor->rotation.y )
	       * matrixTranslation( editor->translation )
	       * matrixScale( editor->scale, editor->scale, editor->scale )
	       * matrixTranslation( 0, 0, editor->z );
}
void processEditorViewRotation( EditorView* editor, GameInputs* inputs )
{
	auto delta = inputs->mouse.delta * 0.01f;
	editor->rotation.x -= delta.x;
	editor->rotation.y -= delta.y;
}
void processEditorViewTranslation( EditorView* editor, GameInputs* inputs )
{
	auto delta = inputs->mouse.delta / ( editor->scale * ( 400 / editor->z ) );
	editor->translation.x += delta.x;
	editor->translation.y -= delta.y;
}

mat4 processEditorViewGui( EditorView* editor, GameInputs* inputs, rectfarg rect )
{
	auto handle = imguiMakeHandle( editor, ImGuiControlType::Custom );

	auto handleMouse = [&]( uint8 button, uint8 modifier,
	                        void ( *proc )( EditorView * editor, GameInputs * inputs ) ) {
		auto pressed      = isKeyPressed( inputs, button );
		auto modifierDown = ( modifier != 0 ) ? isKeyDown( inputs, modifier ) : ( true );
		if( ( isPointInside( rect, inputs->mouse.position ) && pressed && modifierDown )
		    || imguiHasCapture( handle ) ) {

			if( pressed ) {
				imguiFocus( handle );
				imguiCapture( handle, button );
			}
			if( imguiIsHover( handle ) && isKeyDown( inputs, button ) ) {
				proc( editor, inputs );
			}
		}
	};

	handleMouse( KC_LButton, KC_Space, processEditorViewRotation );
	handleMouse( KC_MButton, 0, processEditorViewTranslation );

	if( ( isPointInside( rect, inputs->mouse.position ) || imguiHasFocus( handle ) )
	    && !floatEqZero( inputs->mouse.wheel ) ) {

		editor->scale = clamp( editor->scale + inputs->mouse.wheel * 0.1f, EditorViewMinScale,
		                       EditorViewMaxScale );
	}

	return getEditorViewTransform( editor );
}

mat4 processEditorView( EditorView* editor, GameInputs* inputs, rectfarg rect )
{
	if( inputs ) {
		auto handleMouse = [&]( EditorView* editor, uint8 button, uint8 modifier,
		                        void ( *proc )( EditorView * editor, GameInputs * inputs ) ) {
			auto pressed      = isKeyPressed( inputs, button );
			auto modifierDown = ( modifier != 0 ) ? isKeyDown( inputs, modifier ) : ( true );
			if( ( ( isPointInside( rect, inputs->mouse.position ) && pressed )
			      || ( editor->capture == button ) )
			    && modifierDown ) {

				if( pressed ) {
					editor->capture = button;
				}
				if( isKeyDown( inputs, button ) ) {
					proc( editor, inputs );
				} else {
					editor->capture = 0;
				}
			} else {
				if( editor->capture == button ) {
					editor->capture = 0;
				}
			}
		};

		handleMouse( editor, KC_LButton, 0, processEditorViewRotation );
		handleMouse( editor, KC_MButton, 0, processEditorViewTranslation );

		if( ( isPointInside( rect, inputs->mouse.position ) || editor->capture )
		    && !floatEqZero( inputs->mouse.wheel ) ) {

			editor->scale = clamp( editor->scale + inputs->mouse.wheel * 0.1f, EditorViewMinScale,
			                       EditorViewMaxScale );
		}
	}

	return getEditorViewTransform( editor );
}