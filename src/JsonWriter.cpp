struct JsonWriter {
	string_builder builder;
	bool8 isNotFirst;
	bool8 isNotProperty;
	bool8 minimal;
	int32 indentLevel;

	char* data() const { return builder.data(); }
	int32 size() const { return builder.size(); }
};

JsonWriter makeJsonWriter( char* buffer, int32 len )
{
	JsonWriter result               = {string_builder{buffer, len}};
	result.builder.format.precision = 14;  // set floating point precision to highest value, so that
	                                       // floating point values survive the roundtrip exactly
	return result;
}

static void writeIndentation( JsonWriter* writer )
{
	if( writer->minimal ) {
		return;
	}
	for( auto i = 0; i < writer->indentLevel; ++i ) {
		writer->builder << "  ";
	}
}
static void writeNewLine( JsonWriter* writer )
{
	if( !writer->minimal ) {
		writer->builder << '\n';
	}
}
static void writeComma( JsonWriter* writer )
{
	if( writer->isNotFirst && writer->isNotProperty ) {
		writer->builder << ',';
		writeNewLine( writer );
		writeIndentation( writer );
	} else if( writer->isNotProperty ) {
		writeNewLine( writer );
		writeIndentation( writer );
	}
}
void writeStartObject( JsonWriter* writer )
{
	writeComma( writer );
	writer->builder << '{';
	writer->isNotFirst    = false;
	writer->isNotProperty = true;
	++writer->indentLevel;
}
void writeEndObject( JsonWriter* writer )
{
	--writer->indentLevel;
	if( writer->isNotFirst ) {
		writeNewLine( writer );
		writeIndentation( writer );
	}
	writer->builder << '}';
	writer->isNotFirst    = true;
	writer->isNotProperty = true;
}
void writeStartArray( JsonWriter* writer )
{
	writeComma( writer );
	writer->builder << '[';
	writer->isNotFirst    = false;
	writer->isNotProperty = true;
	++writer->indentLevel;
}
void writeEndArray( JsonWriter* writer )
{
	--writer->indentLevel;
	if( writer->isNotFirst ) {
		writeNewLine( writer );
		writeIndentation( writer );
	}
	writer->builder << ']';
	writer->isNotFirst    = true;
	writer->isNotProperty = true;
}
void writePropertyName( JsonWriter* writer, StringView name )
{
	if( writer->isNotFirst ) {
		writer->builder << ',';
		writeNewLine( writer );
		writeIndentation( writer );
	} else {
		writeNewLine( writer );
		writeIndentation( writer );
	}
	writer->isNotFirst    = true;
	writer->isNotProperty = false;
	writer->builder.print( "\"{}\":", name );
	if( !writer->minimal ) {
		writer->builder << ' ';
	}
}
template < class T >
void writeValue( JsonWriter* writer, const T& value )
{
	writeComma( writer );
	writer->isNotFirst    = true;
	writer->isNotProperty = true;
	writer->builder << value;
}
void writeValue( JsonWriter* writer, StringView value )
{
	// TODO: when writing string values we need to escape some characters
	writeComma( writer );
	writer->isNotFirst    = true;
	writer->isNotProperty = true;
	writer->builder << '"';
	FOR( c : value ) {
		switch( c ) {
			case '\n': {
				writer->builder << '\n';
				break;
			}
			case '\\': {
				writer->builder << "\\\\";
				break;
			}
			default: {
				writer->builder << c;
				break;
			}
		}
	}
	writer->builder << '"';
}
void writeValue( JsonWriter* writer, const string& value )
{
	writeValue( writer, StringView( value ) );
}
void writeNull( JsonWriter* writer )
{
	writeComma( writer );
	writer->isNotFirst    = true;
	writer->isNotProperty = true;
	writer->builder << "null";
}
void writeValue( JsonWriter* writer, null_t )
{
	writeNull( writer );
}

template < class T >
void writeProperty( JsonWriter* writer, StringView name, const T& value )
{
	writePropertyName( writer, name );
	writeValue( writer, value );
}

void writeValue( JsonWriter* writer, NullableInt32 value )
{
	if( value ) {
		writeValue( writer, value.value );
	} else {
		writeNull( writer );
	}
}
void writeValue( JsonWriter* writer, vec2arg v )
{
	writeStartObject( writer );
	auto prev       = writer->minimal;
	writer->minimal = true;
	writeProperty( writer, "x", v.x );
	writeProperty( writer, "y", v.y );
	writeEndObject( writer );
	writer->minimal = prev;
}
void writeValue( JsonWriter* writer, vec3arg v )
{
#if 0
	writeStartObject( writer );
	auto prev       = writer->minimal;
	writer->minimal = true;
	writeProperty( writer, "x", v.x );
	writeProperty( writer, "y", v.y );
	writeProperty( writer, "z", v.z );
	writeEndObject( writer );
	writer->minimal = prev;
#else
	writeStartArray( writer );
	auto prev       = writer->minimal;
	writer->minimal = true;
	writeValue( writer, v.x );
	writeValue( writer, v.y );
	writeValue( writer, v.z );
	writeEndArray( writer );
	writer->minimal = prev;
#endif
}

template< class T >
void writeValue( JsonWriter* writer, trectarg< T > rect )
{
	writeStartObject( writer );
	auto prev       = writer->minimal;
	writer->minimal = true;
	writeProperty( writer, "left", rect.left );
	writeProperty( writer, "top", rect.top );
	writeProperty( writer, "right", rect.right );
	writeProperty( writer, "bottom", rect.bottom );
	writeEndObject( writer );
	writer->minimal = prev;
}
