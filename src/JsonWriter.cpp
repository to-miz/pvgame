struct JsonWriter {
	string_builder builder;
	bool isNotFirst;
	bool isNotProperty;
	int32 indentLevel;
};

JsonWriter makeJsonWriter( char* buffer, int32 len )
{
	JsonWriter result = {string_builder{buffer, len}};
	return result;
}

static void writeIndentation( JsonWriter* writer )
{
	for( auto i = 0; i < writer->indentLevel; ++i ) {
		writer->builder << "  ";
	}
}
static void writeComma( JsonWriter* writer )
{
	if( writer->isNotFirst && writer->isNotProperty ) {
		writer->builder << ",\n";
		writeIndentation( writer );
	} else if( writer->isNotProperty ) {
		writer->builder << '\n';
		writeIndentation( writer );
	}
}
void writeStartObject( JsonWriter* writer )
{
	writeComma( writer );
	writer->builder << '{';
	writer->isNotFirst = false;
	writer->isNotProperty = true;
	++writer->indentLevel;
}
void writeEndObject( JsonWriter* writer )
{
	--writer->indentLevel;
	if( writer->isNotFirst ) {
		writer->builder << '\n';
		writeIndentation( writer );
	}
	writer->builder << '}';
	writer->isNotFirst = true;
	writer->isNotProperty = true;
}
void writeStartArray( JsonWriter* writer )
{
	writeComma( writer );
	writer->builder << '[';
	writer->isNotFirst = false;
	writer->isNotProperty = true;
	++writer->indentLevel;
}
void writeEndArray( JsonWriter* writer )
{
	--writer->indentLevel;
	if( writer->isNotFirst ) {
		writer->builder << '\n';
		writeIndentation( writer );
	}
	writer->builder << ']';
	writer->isNotFirst = true;
	writer->isNotProperty = true;
}
void writePropertyName( JsonWriter* writer, StringView name )
{
	if( writer->isNotFirst ) {
		writer->builder << ",\n";
		writeIndentation( writer );
	} else {
		writer->builder << '\n';
		writeIndentation( writer );
	}
	writer->isNotFirst = true;
	writer->isNotProperty = false;
	writer->builder.print( "\"{}\": ", name );
}
template < class T >
void writeValue( JsonWriter* writer, const T& value )
{
	writeComma( writer );
	writer->isNotFirst = true;
	writer->isNotProperty = true;
	writer->builder << value;
}
void writeValue( JsonWriter* writer, StringView value )
{
	// TODO: when writing string values we need to escape some characters
	writeComma( writer );
	writer->isNotFirst = true;
	writer->isNotProperty = true;
	writer->builder.print( "\"{}\"", value );
}

template < class T >
void writeProperty( JsonWriter* writer, StringView name, const T& value )
{
	writePropertyName( writer, name );
	writeValue( writer, value );
}

void writeValue( JsonWriter* writer, vec2arg v )
{
	writeStartObject( writer );
	writeProperty( writer, "x", v.x );
	writeProperty( writer, "y", v.y );
	writeEndObject( writer );
}