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
	static const AnimatorAsset::Type AssetType = AnimatorAsset::type_collection;

	int16 assetIndex;
	int16 index;
	int8 animation;
	int8 frame;
	int16 id;
};
struct SkeletonEmitterState {
	static const AnimatorAsset::Type AssetType = AnimatorAsset::type_emitter;

	int16 assetIndex;
	int16 index;
	int16 id;
	bool8 active;
	float time;
};
struct SkeletonHitboxState {
	rectf relative;
	vec3 origin;
	int16 assetIndex;
	int16 index;
	int16 id;
	bool8 active;
	enum Type : int8 { Collision, Hitbox, Hurtbox } type;
};
struct SkeletonKeyframesState {
	int16 translation;
	int16 rotation;
	int16 scale;
	int16 custom;
};
struct SkeletonAnimationState {
	enum : uint16 {
		Repeating = BITFIELD( 0 ),
		Playing   = BITFIELD( 1 ),
	};
	int16 animationIndex;
	uint16 flags;
	float currentFrame;
	float blendFactor;
	Array< SkeletonKeyframesState > keyframeStates;
};
struct SkeletonDefinition;
struct Skeleton {
	Array< SkeletonTransform > transforms;
	Array< mat4 > worldTransforms;
	Array< SkeletonVoxelVisuals > visuals;
	Array< SkeletonEmitterState > emitters;
	Array< const VoxelCollection* > voxels;
	Array< SkeletonHitboxState > hitboxes;
	UArray< SkeletonAnimationState > animations;
	Array< Array< SkeletonKeyframesState > > keyframeStatesPool;
	int16 definitionId;
	bool8 dirty;
	bool8 mirrored;
	const SkeletonDefinition* definition;

	mat4 rootTransform;
};
void setTransform( Skeleton* skeleton, mat4arg transform );
void setMirrored( Skeleton* skeleton, bool mirrored );

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
	Array< SkeletonKeyframe< int8 > > frame;
	Array< SkeletonKeyframe< bool8 > > active;
	int16 id;
	bool8 hasKeyframes;
};
struct SkeletonAnimation {
	Array< SkeletonTransform > transforms;
	Array< SkeletonVoxelVisuals > visuals;
	Array< SkeletonEmitterState > emitters;
	Array< SkeletonHitboxState > hitboxes;
	Array< SkeleonKeyframes > keyframes;
	Array< BezierForwardDifferencerData > curves;
	float duration;
};
struct SkeletonId {
	int16 id;
	int16 nameLength;
	char name[25];

	StringView getName() const { return {name, nameLength}; }
};
struct SkeletonAssetId {
	int16 id;
	int16 assetIndex;
	int16 nameLength;
	AnimatorAsset::Type type;
	char name[10];

	StringView getName() const { return {name, nameLength}; }
};
struct SkeletonDefinition {
	Array< SkeletonAnimation > animations;
	Array< SkeletonTransform > baseNodes;
	Array< SkeletonVoxelVisuals > baseVisuals;
	Array< SkeletonEmitterState > baseEmitters;
	Array< SkeletonHitboxState > baseHitboxes;
	Array< SkeletonId > animationIds;
	Array< SkeletonId > nodeIds;
	Array< SkeletonAssetId > assetIds;

	Array< VoxelCollection > voxels;
	Array< AnimatorParticleEmitter > emitters;
	Array< rectf > hitboxes;
	int16 id;

	inline explicit operator bool() const { return id >= 0; };
};

struct HeroSkeletonDefinition {
	SkeletonDefinition* definition; // if this is null, definition is not loaded

	// these ids need to be contiguous
	// must be same order as HeroAnimationNames
	struct {
		int8 turn;
		int8 landing;

		int8 idle;
		int8 walk;
		int8 jumpRising;
		int8 jumpFalling;
		int8 wallslide;

		int8 idleShoot;
		int8 walkShoot;
		int8 jumpRisingShoot;
		int8 jumpFallingShoot;
		int8 wallslideShoot;
	} ids;
};
const StringView HeroAnimationNames[] = {
    "Turn",
    "Landing",

    "Idle",
    "Walk",
    "JumpRising",
    "JumpFalling",
    "Wallslide",

    "IdleShoot",
    "WalkShoot",
    "JumpRisingShoot",
    "JumpFallingShoot",
    "WallslideShoot",
};

struct SkeletonSystem {
	UArray< Skeleton > skeletons;
	UArray< SkeletonDefinition > definitions;

	HeroSkeletonDefinition hero;
};