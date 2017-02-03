constexpr const float EditorViewMinScale = 0.1f;
constexpr const float EditorViewMaxScale = 10.0f;

struct EditorView {
	vec3 rotation    = {};
	vec3 translation = {};
	float scale      = 1;
	float z          = 0;
	uint8 capture    = 0;
};