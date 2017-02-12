struct SkeletonTransform {
	vec3 translation;
	vec3 rotation;
	vec3 scale;
	float length;
	int16 parent;
	int16 id;
};
enum class SkeletonVisualType : int8 { None, Voxel };
struct SkeletonVoxelVisuals {
	int16 voxelIndex;
	int16 index;
	int8 animation;
	int8 frame;
	int16 id;
};
struct SkeletonKeyframesState {
	int16 translation;
	int16 rotation;
	int16 scale;
	int16 custom;
};
struct SkeletonAnimationState {
	int16 animationIndex;
	float currentFrame;
	float blendFactor;
	Array< SkeletonKeyframesState > keyframeStates;
};
struct SkeletonDefinition;
struct Skeleton {
	Array< SkeletonTransform > transforms;
	Array< mat4 > worldTransforms;
	Array< SkeletonVoxelVisuals > visuals;
	Array< const VoxelCollection* > voxels;
	UArray< SkeletonAnimationState > animations;
	Array< Array< SkeletonKeyframesState > > keyframeStatesPool;
	int16 definitionId;
	bool8 dirty;
	const SkeletonDefinition* definition;
};
enum class SkeletonEaseType : int8 {
	Lerp,
	Step,
	Smoothstep,
	EaseOutBounce,
	EaseOutElastic,
	Curve,
};
template< class T >
struct SkeletonKeyframe {
	typedef T value_type;

	float t;
	T data;
	SkeletonEaseType easeType;
	int16 curveIndex;
};
struct SkeleonKeyframes {
	Array< SkeletonKeyframe< vec3 > > translation;
	Array< SkeletonKeyframe< vec3 > > rotation;
	Array< SkeletonKeyframe< vec3 > > scale;
	Array< SkeletonKeyframe< int16 > > frame;
	Array< SkeletonKeyframe< bool8 > > active;
	int16 id;
	bool8 hasKeyframes;
};
struct SkeletonAnimation {
	Array< SkeletonTransform > transforms;
	Array< SkeletonVoxelVisuals > visuals;
	Array< SkeleonKeyframes > keyframes;
	Array< BezierForwardDifferencerData > curves;
	float duration;
};
struct SkeletonId {
	int16 id;
	int16 nameLength;
	char name[10];
};
struct SkeletonAssetId {
	int16 id;
	int16 transformIndex;
	int16 assetIndex;
	int16 nameLength;
	AnimatorAsset::Type type;
	char name[10];
};
struct SkeletonDefinition {
	Array< SkeletonAnimation > animations;
	Array< SkeletonTransform > baseNodes;
	Array< SkeletonVoxelVisuals > baseVisuals;
	Array< SkeletonId > animationIds;
	Array< SkeletonId > nodeIds;
	Array< SkeletonAssetId > assetIds;

	Array< VoxelCollection > voxels;
	int16 id;

	inline explicit operator bool() const { return id >= 0; };
};
struct SkeletonSystem {
	Array< Skeleton > skeletons;
	UArray< SkeletonDefinition > definitions;
};

struct SkeletonTest {
	Skeleton skeleton;
	SkeletonDefinition definition;
	bool initialized;
};