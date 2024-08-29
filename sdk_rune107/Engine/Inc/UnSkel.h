// UnSkel.h
//

// USkelModel.h: interface for the USkelModel class.
//
//////////////////////////////////////////////////////////////////////


#if !defined(_UNSKEL_INCLUDED_)
#define _UNSKEL_INCLUDED_


#define LOG_ROTATION(title, r) debugf(TEXT("%s: p=%.0f, y=%.0f r=%.0f"), TEXT(title), r.Pitch*360.f/65536.f, r.Yaw*360.f/65536.f, r.Roll*360.f/65536.f);

// Scm model file header
struct SCMHeader
{
	BYTE	magic[4];
	INT		version;
	INT		modeltype;
};

// ===== DECIMATE STRUCTURES =====

#pragma pack(1)
typedef struct
{
	INT prevDec;
	_WORD from;
	_WORD to;
	_WORD connect;
	_WORD oldConnect;
	_WORD removeCount;
	_WORD remapCount;
} decimate_t;

typedef struct
{
	_WORD faceIndex;
	_WORD vertIndex;
	BYTE oldTexU;
	BYTE oldTexV;
	BYTE newTexU;
	BYTE newTexV;
} remap_t;

typedef struct
{
	SWORD faceCount;
	INT listOffset;
} connect_t;
#pragma pack()


#define NUM_POLYGROUPS			16	// Change in actor.uc also
#define MAX_JOINTS				50	// Change in actor.uc also
#define ANIMATION_CACHE_SIZE	4	// Cache can hold this many complete frames

#define MAX_CHILD_JOINTS		4

// Joint property flags
#define JOINT_FLAG_BLENDJOINT	0x01	// Apply animation blending to this joint
#define JOINT_FLAG_ACCELERATIVE	0x02	// Can apply forces to this joint (accelerates towards animation state)
#define JOINT_FLAG_SPRINGPOINT	0x04	// Can apply forces to this joint (springs back towards animation state)
#define JOINT_FLAG_IKCHAIN		0x08	// Joint is part of an IK chain
#define JOINT_FLAG_COLLISION	0x10	// Joint has collision property
#define JOINT_FLAG_ABSPOSITION	0x20	//** Not done, being used for SetJointPosition(),
#define JOINT_FLAG_ABSROTATION	0x40	//** Not done, being used for SetJointRotation(),
#define JOINT_FLAG_GRAVJOINT	0x80	// Gravity joint, accelerates according to zone gravity
#define NUM_JOINT_FLAGS			8

#define JOINT_FLAGS_NOANIM		(JOINT_FLAG_ACCELERATIVE|JOINT_FLAG_SPRINGPOINT|JOINT_FLAG_IKCHAIN|JOINT_FLAG_GRAVJOINT|JOINT_FLAG_ABSPOSITION|JOINT_FLAG_ABSROTATION)


#define ANIMMASK_POSX		0x01
#define ANIMMASK_POSY		0x02
#define ANIMMASK_POSZ		0x04
#define ANIMMASK_SCALEXZ	0x08
#define ANIMMASK_SCALEY		0x10
#define ANIMMASK_PITCH		0x20
#define ANIMMASK_YAW		0x40
#define ANIMMASK_ROLL		0x80

// Joint groups
typedef enum
{
	JOINTGROUP_LEGS		= 0,
	JOINTGROUP_TORSO	= 1,
	NUM_JOINTGROUPS		= 2
}jointgroup_e;

// prototypes
struct DynJoint;
class DynSkel;

// Temporary structure used while loading a SCM file
typedef struct
{
	char name[64];
}eventlist_t;

// ----------------------------------------

struct TriUV_t
{
	BYTE u;
	BYTE v;

	friend FArchive &operator<<( FArchive& Ar, TriUV_t& T )
	{
		return Ar << T.u << T.v;
	}
};

struct Triangle
{
	SWORD	vIndex[3];		// Vertex indices
	TriUV_t	tex[3];			// Texture UV coordinates
	BYTE	polygroup;		// polygroup this tri belongs to

	friend FArchive &operator<<( FArchive& Ar, Triangle& T )
	{
		Ar << T.vIndex[0] << T.vIndex[1] << T.vIndex[2];
		Ar << T.tex[0] << T.tex[1] << T.tex[2];
		Ar << T.polygroup;
		return Ar;
	}
};

struct ShadowTri
{
	SWORD vIndex[3];
	FVector Vertex[3];

	friend FArchive &operator<<( FArchive& Ar, ShadowTri& S)
	{
//		Ar << S.vIndex[0] << S.vIndex[1] << S.vIndex[2];
		return Ar;
	}
};

struct Vertex
{
	FVector point;
	FVector point2;		// position of vertex relative to joint2
	INT joint1;
	INT joint2;
	FLOAT weight1;

	friend FArchive &operator<<( FArchive& Ar, Vertex& V )
	{
		Ar << V.point << V.point2 << V.joint1 << V.joint2 << V.weight1;
		return Ar;
	}
};


struct JointState
{
	FVector pos;
	FRotator rot;
	FScale scale;

	friend FArchive &operator<<( FArchive& Ar, JointState& J )
	{
		Ar << J.pos << J.rot << J.scale;
		return Ar;
	}
};


struct AnimFrame
{
	AnimFrame()		// This constructor is called (ReadSCM() uses 'new' operator)
	{
		sequenceID = 0;
		event = NAME_None;
		bounds = FBox(FVector(0,0,0),FVector(0,0,0));
		jointanim.Empty();
	};

	SWORD				sequenceID;			// Sequence this frame belongs to
	FName				event;				// Event to call during this frame
	FBox				bounds;				// Bounding box of frame
	TArray<JointState>	jointanim;			// O(numjoints) only used for preprocess (transient)

	friend FArchive &operator<<( FArchive& Ar, AnimFrame& F )
	{
		Ar << F.sequenceID << F.event << F.bounds;
		Ar << F.jointanim;
		return Ar;
	}
};



struct FSkelAnimSeq : public FMeshAnimSeq
{
	TArray<BYTE> animdata;

	friend FArchive &operator<<( FArchive& Ar, FSkelAnimSeq& S )
	{
		return Ar << (FMeshAnimSeq&)S << S.animdata;
	}
};

struct Mesh
{
	Mesh()	// This constructor is called (ReadSCM() uses 'new' operator)
	{
		guard(Mesh::Mesh);
		numverts = numtris = 0;
		tris.Empty();
		verts.Empty();
		dectris.Empty();
		connect.Empty();
		curDecIndex = 0;
		oldDecCount = 0;
		decCount = 0;
		decdata = NULL;
		unguard;
	};

	// Data
	INT numverts;
	INT numtris;

	TArray<Triangle>	tris;
	TArray<Vertex>		verts;
	INT					GroupFlags[NUM_POLYGROUPS];
	FName				PolyGroupSkinNames[NUM_POLYGROUPS];

	// Chunk of LOD data here
	TArray<Triangle>	dectris;		// Transient
	TArray<INT>			connect;		// Transient
	INT					curDecIndex;	// Transient
	INT					oldDecCount;	// Transient
	TArray<UBOOL>		vertDecimated;	// Transient -- true if the vertex was decimated
	TArray<UBOOL>		triDecimated;	// Transient -- true if the triangle was decimated
	INT					decCount;
	TArray<BYTE>		decdata;

	// Shadow triangles
	TArray<ShadowTri>	ShadowTris; // Shadow triangles

	friend FArchive &operator<<( FArchive& Ar, Mesh& M )
	{
		Ar << M.numverts << M.numtris;
		Ar << M.tris << M.verts;
		Ar << M.decCount;
		Ar << M.decdata;
		for (int ix=0; ix<NUM_POLYGROUPS; ix++)
		{
			Ar << M.GroupFlags[ix];
			Ar << M.PolyGroupSkinNames[ix];
		}

		if(Ar.IsLoading())
		{
			M.curDecIndex = 0;
			M.oldDecCount = 0;
			M.connect.Empty();
			M.connect.Add(M.numverts);
			M.vertDecimated.Empty();
			M.vertDecimated.AddZeroed(M.numverts);
			for(int i = 0; i < M.numverts; i++)
			{
				M.connect(i) = i;
			}

			M.dectris.Empty();
			M.dectris.Add(M.numtris);
			M.triDecimated.Empty();
			M.triDecimated.AddZeroed(M.numtris);
			for(int i = 0; i < M.numtris; i++)
			{
				M.dectris(i) = M.tris(i);
			}

			M.ShadowTris.Empty();
		}
	
		return Ar;
	}
};


struct Joint
{
	// Skeletal structure implimentation
	INT			parent;
	INT			children[MAX_CHILD_JOINTS];
	FName		name;						// Name of joint
	INT			jointgroup;					// Body group this belongs (allows us to animate groups seperately)
	INT			flags;						// Default joint flags
	FRotator	baserot;					// Rotational delta to base orientation
	FPlane		planes[6];					// Local joint coordinate planes defining a collision box

	// Serializer.
	friend FArchive &operator<<( FArchive& Ar, Joint& J )
	{
		Ar << J.parent << J.children[0] << J.children[1] << J.children[2] << J.children[3];
		Ar << J.name << J.jointgroup << J.flags;
		Ar << J.baserot;
		Ar << J.planes[0] << J.planes[1] << J.planes[2] << J.planes[3] << J.planes[4] << J.planes[5];

		return Ar;
	}
};


struct AnimationCache
{
	TArray<JointState>		states;		// O(numjoints) used to speed up anim decoding
	FName					SeqName;	// Sequence stored in AnimCache1
	INT						Frame;		// Frame stored in AnimCache1
	DWORD					Time;		// Time cache was last accessed
};

class ENGINE_API USkelModel : public UPrimitive
{
	DECLARE_CLASS(USkelModel, UPrimitive, 0, Engine);

public:
	INT nummeshes;
	INT numjoints;
	INT numframes;
	INT numsequences;
	INT numskins;
	INT rootjoint;

	FVector					PosOffset;		// Offset of creature relative to base
	FRotator				RotOffset;		// Offset of creatures rotation

	TArray<Mesh>			meshes;
	TArray<Joint>			joints;
	TArray<FSkelAnimSeq>	AnimSeqs;		// Compressed animation data for sequence
	TArray<AnimFrame>		frames;

	// Data below here are not in serializer (transient)
	// -----------------------------------------------------------------------------------
	AnimationCache			AnimCache[ANIMATION_CACHE_SIZE];
	DWORD					Time;

	// UObject Interface
	USkelModel();
	void PostLoad();
	void Serialize( FArchive& Ar );

	// UPrimitive Interface
	FBox GetRenderBoundingBox(const AActor* Owner, UBOOL Exact );
	FSphere GetRenderBoundingSphere( const AActor* Owner, UBOOL Exact );
	FBox GetCollisionBoundingBox( const AActor *Owner ) const;
	UBOOL LineCheck
	(
		FCheckResult&	Result,
		AActor*			Owner,
		FVector			End,
		FVector			Start,
		FVector			Size,
		DWORD           ExtraNodeFlags
	);

	UBOOL LineCheckJointBox
	(
		FCheckResult&	Result,
		AActor*			Owner,
		FVector			End,
		FVector			Start,
		FVector			Size,
		DWORD           ExtraNodeFlags,
		FPlane			Planes[6]
	);
	// END TEST

	// USkelModel Interface
	DynSkel *LockDSkel(const AActor *Owner, FCacheItem *&Item) const;
	void UnlockDSkel(FCacheItem *Item) const;
	bool DSkelLocked();
	void EnsureLatestCoords(AActor *Owner);
	FVector GetJointPos(AActor *Owner, int j);
	INT JointNamed(const FName &name);
	void TurnJointTo(INT j, FRotator worldrot, AActor *Owner);
	void TickSkeleton(AActor *Owner, FLOAT DeltaSeconds);
	void GetFrame(AActor* Owner, const FCoords &coords, FLOAT LODCurve, FVector *Deformed);
	FLOAT Decimate(AActor *Owner, FSceneNode *Frame, UBOOL bWire, INT SkelApprox);
	void CalculateLODCurve(AActor *Owner, FSceneNode *Frame, FLOAT &curve, FLOAT &percent);
	void SetDefaultJointFlags(AActor *Owner);
	void InitializeForActor(AActor *Owner);
	void UpdateSkeletonToAbsoluteFrame(FName SeqName, int frame, AActor *Owner);
	void GetSkeleton(AActor *Owner, const FCoords &Coords, FName Sequence, int Frame, FName ProxySequence, int ProxyFrame);

	int ReadSCM(const char dfilename[], const TCHAR* modelName, UObject *pkg, UBOOL Unmirror, UBOOL NoCenter);
	FSkelAnimSeq* GetAnimSeq(AActor *Owner, FName seqname);
	inline FLOAT DecodeShortToFloat(short s)
	{
		return ((float)s) / 256.0f;
	}
	inline INT DecodeShortToInt(short s)
	{
		return (INT)s;
	}
	inline short EncodeFloatToShort(FLOAT f)
	{
		return (short)(f * 256.0f);
	}
	inline short EncodeIntToShort(INT i)
	{
		return (short)i;
	}
	void SetAnimFrame(const FCoords &coords, AActor *Owner);
	void DecodeFrame(FSkelAnimSeq &seq, int localframe, JointState *js);
	JointState GetJointState(FSkelAnimSeq &seq, int joint, int localframe);
	void CompressAnimationSequence(BYTE *outptr, int &outlength, short seqid);
	FVector		WorldToModel	(AActor *Owner, FVector&	world);
	FVector		WorldToJoint	(AActor *Owner, FVector&	world, int j);
	FVector		WorldPosToJoint	(AActor *Owner, FVector&	world, int j);
	FRotator	WorldRotToJoint	(AActor *Owner, FRotator&	world, int j);
	FRotator	ModelRotToJoint	(AActor *Owner, FRotator&	model, int j);


private:
	void CalculateChildBones(INT jx, FCoords coords);
	void GetAbsoluteFrame(INT jointgroup, const AActor *Owner, int &lowframe, int &highframe);
	void TransformVertex(AActor *Owner, Vertex *vert, FVector *out);
};


// Per instance skeletal model information (transient)
struct DynJoint
{
public:
	FVector		curpos;			// Joint position in world coordinates
	FRotator	currot;			// Joint rotation in world coordinates
	FCoords		coords;			// Transformation coords

	// Dynamic Joint support parameters (only needed for accelerative, springpoint joints)
	// NOTE: All vectors/rotators are in local joint coordinates

	//									SPRINGPOINT			ACCELERATIVE		
	//									-----------			------------
	FVector		relpos;				//	joint pos			joint pos
	FVector		relvel;				//	joint vel			joint vel
	FVector		relaccel;			//	Rest position		joint accel

	FLOAT		AccelBoneLength;	//						Length of bone

	FRotator	relrot;				//	Joint rotation		Joint rotation
	FRotator	radialVel;			//	Radial velocity		Radial velocity
	FRotator	radialAccel;		//	Radial accel		Radial accel
	FRotator	deltarot;			//	Extra rotation added on for LookAt() TurnTo()

	void MangleJointTranslation(FVector &pos, AActor *Owner, INT jointindex);
	void MangleJointRotation(FRotator &rot, AActor *Owner, INT jointindex);
	void MangleJointScale(FScale &scale, AActor *Owner, INT jointindex);
	void Tick(FLOAT DeltaSeconds, AActor *Owner, INT jointindex);
};


class DynSkel
{
public:
	DynJoint	*joint;				// Array of DynJoints allocated when cache is created

	void Initialize(AActor *Owner);
	void Dynamics(AActor *Owner, TArray<JointState>& js);
};


#endif


