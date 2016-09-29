struct QuadTexCoords {
	vec2 elements[4];
};

constexpr inline QuadTexCoords makeQuadTexCoordsDef()
{
	return {0, 0, 1, 0, 0, 1, 1, 1};
}

#if QUADTEXCOORDS_ARGS_AS_CONST_REF == 1
	typedef const QuadTexCoords& QuadTexCoordsArg;
#else
	typedef QuadTexCoords QuadTexCoordsArg;
#endif

QuadTexCoords makeQuadTexCoords( rectf texCoords )
{
	QuadTexCoords result;
	result.elements[0] = {texCoords.left, texCoords.top};
	result.elements[1] = {texCoords.right, texCoords.top};
	result.elements[2] = {texCoords.left, texCoords.bottom};
	result.elements[3] = {texCoords.right, texCoords.bottom};
	return result;
}
QuadTexCoords makeQuadTexCoordsCcw90( rectf texCoords )
{
	QuadTexCoords result;
	result.elements[0] = {texCoords.left, texCoords.bottom};
	result.elements[1] = {texCoords.left, texCoords.top};
	result.elements[2] = {texCoords.right, texCoords.bottom};
	result.elements[3] = {texCoords.right, texCoords.top};
	return result;
}
QuadTexCoords makeQuadTexCoordsCw90( rectf texCoords )
{
	QuadTexCoords result;
	result.elements[0] = {texCoords.right, texCoords.top};
	result.elements[1] = {texCoords.right, texCoords.bottom};
	result.elements[2] = {texCoords.left, texCoords.top};
	result.elements[3] = {texCoords.left, texCoords.bottom};
	return result;
}
QuadTexCoords makeQuadTexCoordsCcw90( QuadTexCoordsArg texCoords )
{
	QuadTexCoords result;
	result.elements[0] = texCoords.elements[1];
	result.elements[1] = texCoords.elements[3];
	result.elements[2] = texCoords.elements[0];
	result.elements[3] = texCoords.elements[2];
	return result;
}
QuadTexCoords makeQuadTexCoordsCw90( QuadTexCoordsArg texCoords )
{
	QuadTexCoords result;
	result.elements[0] = texCoords.elements[2];
	result.elements[1] = texCoords.elements[0];
	result.elements[2] = texCoords.elements[3];
	result.elements[3] = texCoords.elements[1];
	return result;
}

QuadTexCoords makeQuadTexCoordsMirroredHorizontal( rectf texCoords )
{
	QuadTexCoords result;
	result.elements[0] = {texCoords.right, texCoords.top};
	result.elements[1] = {texCoords.left, texCoords.top};
	result.elements[2] = {texCoords.right, texCoords.bottom};
	result.elements[3] = {texCoords.left, texCoords.bottom};
	return result;
}
QuadTexCoords makeQuadTexCoordsMirroredVertical( rectf texCoords )
{
	QuadTexCoords result;
	result.elements[0] = {texCoords.left, texCoords.bottom};
	result.elements[1] = {texCoords.right, texCoords.bottom};
	result.elements[2] = {texCoords.left, texCoords.top};
	result.elements[3] = {texCoords.right, texCoords.top};
	return result;
}
QuadTexCoords makeQuadTexCoordsMirroredDiagonal( rectf texCoords )
{
	QuadTexCoords result;
	result.elements[0] = {texCoords.right, texCoords.bottom};
	result.elements[1] = {texCoords.left, texCoords.bottom};
	result.elements[2] = {texCoords.right, texCoords.top};
	result.elements[3] = {texCoords.left, texCoords.top};
	return result;
}
QuadTexCoords makeQuadTexCoordsMirroredHorizontal( QuadTexCoordsArg texCoords )
{
	QuadTexCoords result;
	result.elements[0] = texCoords.elements[1];
	result.elements[1] = texCoords.elements[0];
	result.elements[2] = texCoords.elements[3];
	result.elements[3] = texCoords.elements[2];
	return result;
}
QuadTexCoords makeQuadTexCoordsMirroredVertical( QuadTexCoordsArg texCoords )
{
	QuadTexCoords result;
	result.elements[0] = texCoords.elements[2];
	result.elements[1] = texCoords.elements[3];
	result.elements[2] = texCoords.elements[0];
	result.elements[3] = texCoords.elements[1];
	return result;
}
QuadTexCoords makeQuadTexCoordsMirroredDiagonal( QuadTexCoordsArg texCoords )
{
	QuadTexCoords result;
	result.elements[0] = texCoords.elements[3];
	result.elements[1] = texCoords.elements[2];
	result.elements[2] = texCoords.elements[1];
	result.elements[3] = texCoords.elements[0];
	return result;
}

float getAxisAlignedWidth( QuadTexCoordsArg texCoords )
{
	return texCoords.elements[1].u - texCoords.elements[0].u;
}
float getAxisAlignedHeight( QuadTexCoordsArg texCoords )
{
	return texCoords.elements[2].v - texCoords.elements[0].v;
}
