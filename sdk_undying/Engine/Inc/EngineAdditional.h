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

struct FPlace
{
    FQuat Rot;
    FVector	Pos;
};

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
