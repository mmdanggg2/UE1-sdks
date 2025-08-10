// Credit to https://github.com/l0m-dev/undying-headers

struct FDamageInfo
{
    float 	Damage;				// Damage Amount (Hitpoints)
    int 	ManaCost;			// Damage Amount (Hitpoints)
    FName	DamageType;			// Damage Type
    FString	DamageString;		// Damage String - used to construc messages
    bool	bMagical;			// Damage is magical
    AActor* Deliverer;			// Actor Delivering this damage
    bool 	bBounceProjectile;	// A projectile delivering this damage needs to be destroyed
    float	EffectStrength;		// Strength of the secondary effects of being damage
    FName	JointName;			// Name of the joint to apply damage to
    FVector	ImpactForce;		// direction and magnatude of the damage impact on the skeleton
    float	DamageMultiplier;	// damage multiplier - for head shots.
    float	DamageRadius;		// radius of area effect (explosive) damage.
    FVector	DamageLocation;		// location of area effect (explosive) damage.
};
struct FSoundProps {};
struct FStatInfo {};
struct FSubtitleInfo {};

struct FFloatParams {
    float Base;
    float Rand;
};
struct FColorParams {
    FColor Base;
    FColor Rand;
};

struct FFootSoundEntry {};

struct FActData
{
    float timer;
    float stamp;
    int directdata;
    int func;
    int subfunc;
    int directdatasize;
    int	effect;
};

#pragma comment(lib, "DWI")
namespace DWI {
class VRotate3{
public:
    float X, Y, Z;
};
class Rotate3 {
public:
    float X, Y, Z, W;
};

class Vector3 {
public:
    float X, Y, Z;
};

class Dir3 : public Vector3 {};
class Matrix3 {
public:
    Vector3 Row0;
    Vector3 Row1;
    Vector3 Row2;
};
class Transform3 {
public:
    Matrix3 Matrix;
    Vector3 Origin;
};

class Placement3 {
public:
    Rotate3 Rot;
    FVector	Pos;
};

template <class T>
class array {
    T* data;
    int arrayNum;
};
class Cylinder
{
public:
    float Bottom;
    float Top;
    float Radius;
    Dir3 Dir;
};

class SkelProp {
public:
    int ID; // gets set to FName JointName
    bool MoveNode;
    float MaxVel;
    Cylinder CylinderBound;
    float BoundingRadius;
    float CumulativeRadius;
    VRotate3 RotLimits[2];
};

class SkelNode {
public:
    bool Visible;
    bool Tangible;
    SkelProp* const Prop;
    SkelNode* Parent;
    short NodeIndex;
    short NodeDepth;
    array<SkelNode*> Elements;
    Placement3 RelPlace;
};

class AnimEnviron {
public:
    Vector3 gravity;
    Vector3 wind;
    void* WorldGeom; // array<DWI::Plane>? or list
};

class AnimState {
public:
    const TCHAR* MeshName;
    SkelNode* pSkeleton; // used in GetFrame, set in ApplyAnim
    SkelNode* JointSkel; // offset from pSkeleton (by 1)
    SkelNode* pMeshSkeleton; // set in costructor, from Mesh->SkelProto (AnimData->SkelProto)
    array<Vector3> AnimVerts;
    array<struct JointVert> JointVerts;
    class AnimCommandList* pCommands; // CPP_98: undying used std::list*
    bool IsAnimating; // is animating? bNeedsUpdate?
    bool bNeedsJointWorldTransUpdate; // set in ApplyAnim
    float DrawScale;
    AnimEnviron AnimEnv;
    array<Transform3> JointWorldTrans;
    UMesh* Mesh;
    bool bNeedsSkelPlacementUpdate;
    float AnimTimeSeconds;

    Transform3 GetRootTrans() const;
};
}; // DWI::

typedef DWI::Placement3 FPlace;

struct FLighting				// Lighting properties for a material.
{
    FColor	Constant;			// Constant (self-illumination) color.
    FColor	Diffuse;			// Omni-directional reflectance (modulates texture).
    FColor	SpecularShade;		// Directional reflectance, shading texture.
    FColor	SpecularHilite;		// Directional reflectance, highlight texture.
    BYTE	SpecularWidth;		// Sharpness: 0 = perfectly sharp, max = perfectly diffuse.
    INT     TextureMask;		// Bitmask of textures this material applies to. -1 = all.
};

struct FImpactSoundParams
{
    float MaxVolume;
    float MinVolume;
    float Radius;
    float MaxPitch;
    float MinPitch;
    USound* Sound_1;
    USound* Sound_2;
    USound* Sound_3;
};

struct FImpactSoundEntry {};
