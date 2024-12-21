/*=============================================================================
	D3D8.h: Unreal D3D8 support header.
	Portions copyright 1999 Epic Games, Inc. All Rights Reserved.

	Revision history:

=============================================================================*/

#define UTGLR_UNREAL_227_BUILD 1

#include <math.h>
#include <stdio.h>

#include <map>
#pragma warning(disable : 4663)
#pragma warning(disable : 4018)
#include <vector>

#pragma warning(disable : 4244)
#pragma warning(disable : 4245)

#pragma warning(disable : 4146)



typedef IDirect3D8 * (WINAPI * LPDIRECT3DCREATE8)(UINT SDKVersion);

//Use debug D3D8 DLL
//#define UTD3D8R_USE_DEBUG_D3D8_DLL

//Vertex program assembly code
//#define UTD3D8R_INCLUDE_VERTEX_PROGRAM_ASM

#if ASM
#ifdef UTD3D8R_INCLUDE_VERTEX_PROGRAM_ASM
#include <d3dx8.h>
#endif

//Optional ASM code
#define UTGLR_USE_ASM_CODE

//Optional SSE code
#define UTGLR_INCLUDE_SSE_CODE

#ifdef UTGLR_INCLUDE_SSE_CODE
#include <xmmintrin.h>
#include <emmintrin.h>
#endif
#endif

//Optional opcode patch code
//#define UTGLR_INCLUDE_OPCODE_PATCH_CODE

#ifdef UTGLR_INCLUDE_OPCODE_PATCH_CODE
#include "OpcodePatch.h"
#endif

//Optional fastcall calling convention usage
#define UTGLR_USE_FASTCALL

#ifdef UTGLR_USE_FASTCALL
	#ifdef WIN32
	#define FASTCALL	__fastcall
	#else
	#define FASTCALL
	#endif
#else
#define FASTCALL
#endif


//Debug defines
//#define UTGLR_DEBUG_SHOW_TEX_CONVERT_COUNTS
//#define UTGLR_DEBUG_SHOW_CALL_COUNTS
//#define UTGLR_DEBUG_WORLD_WIREFRAME
//#define UTGLR_DEBUG_ACTOR_WIREFRAME
//#define UTGLR_DEBUG_Z_RANGE_HACK_WIREFRAME


/*-----------------------------------------------------------------------------
	Globals.
-----------------------------------------------------------------------------*/

//If exceeds 7, various things will break
#define MAX_TMUNITS			4		// vogel: maximum number of texture mapping units supported

//Must be at least 2000
#define VERTEX_ARRAY_SIZE	4000	// vogel: better safe than sorry


/*-----------------------------------------------------------------------------
	D3D8Drv.
-----------------------------------------------------------------------------*/

class UD3D8RenderDevice;


template <class ClassT> class rbtree_allocator {
public:
	typedef typename ClassT::node_t node_t;

public:
	rbtree_allocator() {
	}
	~rbtree_allocator() {
	}

	node_t *alloc_node(void) {
		return new node_t;
	}

	void free_node(node_t *pNode) {
		delete pNode;
	}
};

template <class KeyT, class DataT> class rbtree {
private:
	enum { RED_NODE, BLACK_NODE };
	enum { Left = 0, Right = 1 };

public:
	struct node_t {
		struct node_t *pC[2], *pParent;
		unsigned char color;
		KeyT key;
		DataT data;
	};

private:
	rbtree(const rbtree &);
	rbtree &operator=(const rbtree &);

public:
	rbtree() {
		m_pRoot = &m_NIL;
		m_NIL.color = BLACK_NODE;
		m_NIL.pParent = 0;
		m_NIL.pC[Left] = 0;
		m_NIL.pC[Right] = 0;
	}
	~rbtree() {
	}

	node_t *find(KeyT key) {
		node_t *px;

		px = m_pRoot;
		while (px != &m_NIL) {
			if (key == px->key) {
				return px;
			}
			px = px->pC[(key < px->key) ? Left : Right];
		}

		return 0;
	}

	bool insert(node_t *pNode) {
		//Attempt to insert the new node
		if (m_pRoot == &m_NIL) {
			m_pRoot = pNode;
			pNode->pParent = &m_NIL;
			pNode->pC[Left] = &m_NIL;
			pNode->pC[Right] = &m_NIL;
			pNode->color = BLACK_NODE;

			return true;
		}

		node_t *px, *py;
		unsigned int compLR = Left;

		px = m_pRoot;
		py = 0;

		while (px != &m_NIL) {
			py = px;

			if (pNode->key == px->key) {
				return false;
			}

			compLR = (pNode->key < px->key) ? Left : Right;
			px = px->pC[compLR];
		}

		py->pC[compLR] = pNode;
		pNode->pParent = py;
		pNode->pC[Left] = &m_NIL;
		pNode->pC[Right] = &m_NIL;
		pNode->color = RED_NODE;

		//Rebalance the tree
		px = pNode;
		node_t *pxParent;
		while (((pxParent = px->pParent) != &m_NIL) && (pxParent->color == RED_NODE)) {
			node_t *pxParentParent;

			pxParentParent = pxParent->pParent;
			pxParentParent->color = RED_NODE;
			if (pxParent == pxParentParent->pC[Left]) {
				py = pxParentParent->pC[Right];
				if (py->color == RED_NODE) {
					py->color = BLACK_NODE;
					pxParent->color = BLACK_NODE;
					px = pxParentParent;
				}
				else {
					if (px == pxParent->pC[Right]) {
						px = pxParent;
						left_rotate(px);
						pxParent = px->pParent;
					}
					pxParent->color = BLACK_NODE;
					right_rotate(pxParentParent);
				}
			}
			else {
				py = pxParentParent->pC[Left];
				if (py->color == RED_NODE) {
					py->color = BLACK_NODE;
					pxParent->color = BLACK_NODE;
					px = pxParentParent;
				}
				else {
					if (px == pxParent->pC[Left]) {
						px = pxParent;
						right_rotate(px);
						pxParent = px->pParent;
					}
					pxParent->color = BLACK_NODE;
					left_rotate(pxParentParent);
				}
			}
		}

		m_pRoot->color = BLACK_NODE;

		return true;
	}

	void remove(node_t *pNode) {
		node_t *px, *py;

		py = pNode;
		if (py->pC[Left] == &m_NIL) {
			px = py->pC[Right];
		}
		else if (py->pC[Right] == &m_NIL) {
			px = py->pC[Left];
		}
		else {
			py = py->pC[Right];
			while (py->pC[Left] != &m_NIL) {
				py = py->pC[Left];
			}
			px = py->pC[Right];
		}

		if (py != pNode) {
			pNode->pC[Left]->pParent = py;
			py->pC[Left] = pNode->pC[Left];
			if (py != pNode->pC[Right]) {
				px->pParent = py->pParent;
				py->pParent->pC[Left] = px;
				py->pC[Right] = pNode->pC[Right];
				pNode->pC[Right]->pParent = py;
			}
			else {
				px->pParent = py;
			}

			if (m_pRoot == pNode) {
				m_pRoot = py;
			}
			else {
				pNode->pParent->pC[(pNode->pParent->pC[Left] == pNode) ? Left : Right] = py;
			}
			py->pParent = pNode->pParent;

			unsigned char tempColor = pNode->color;
			pNode->color = py->color;
			py->color = tempColor;

			py = pNode;
		}
		else {
			px->pParent = py->pParent;

			if (m_pRoot == pNode) {
				m_pRoot = px;
			}
			else {
				py->pParent->pC[(py->pParent->pC[Left] == py) ? Left : Right] = px;
			}
		}

		if (py->color == BLACK_NODE) {
			node_t *pxp;

			while (((pxp = px->pParent) != &m_NIL) && (px->color == BLACK_NODE)) {
				if (px == pxp->pC[Left]) {
					node_t *pw;

					pw = pxp->pC[Right];
					if (pw->color == RED_NODE) {
						pw->color = BLACK_NODE;
						pxp->color = RED_NODE;
						left_rotate(pxp);
						pw = pxp->pC[Right];
					}
					//if ((pw->pC[Left]->color == BLACK_NODE) & (pw->pC[Right]->color == BLACK_NODE))
					if (((pw->pC[Left]->color ^ BLACK_NODE) | (pw->pC[Right]->color ^ BLACK_NODE)) == 0) {
						pw->color = RED_NODE;
						px = pxp;
					}
					else {
						if (pw->pC[Right]->color == BLACK_NODE) {
							pw->pC[Left]->color = BLACK_NODE;
							pw->color = RED_NODE;
							right_rotate(pw);
							pw = pxp->pC[Right];
						}
						pw->color = pxp->color;
						pxp->color = BLACK_NODE;
						pw->pC[Right]->color = BLACK_NODE;
						left_rotate(pxp);
						break;
					}
				}
				else {
					node_t *pw;

					pw = pxp->pC[Left];
					if (pw->color == RED_NODE) {
						pw->color = BLACK_NODE;
						pxp->color = RED_NODE;
						right_rotate(pxp);
						pw = pxp->pC[Left];
					}
					//if ((pw->pC[Right]->color == BLACK_NODE) & (pw->pC[Left]->color == BLACK_NODE))
					if (((pw->pC[Right]->color ^ BLACK_NODE) | (pw->pC[Left]->color ^ BLACK_NODE)) == 0) {
						pw->color = RED_NODE;
						px = pxp;
					}
					else {
						if (pw->pC[Left]->color == BLACK_NODE) {
							pw->pC[Right]->color = BLACK_NODE;
							pw->color = RED_NODE;
							left_rotate(pw);
							pw = pxp->pC[Left];
						}
						pw->color = pxp->color;
						pxp->color = BLACK_NODE;
						pw->pC[Left]->color = BLACK_NODE;
						right_rotate(pxp);
						break;
					}
				}
			}
			px->color = BLACK_NODE;
		}

		return;
	}

	node_t * FASTCALL next_node(node_t *pNode) {
		node_t *px;

		if (pNode->pC[Right] != &m_NIL) {
			pNode = pNode->pC[Right];
			while (pNode->pC[Left] != &m_NIL) {
				pNode = pNode->pC[Left];
			}
			return pNode;
		}

		if (pNode->pParent == &m_NIL) {
			return &m_NIL;
		}

		px = pNode->pParent;
		while (pNode == px->pC[Right]) {
			pNode = px;
			if (pNode->pParent == &m_NIL) {
				return &m_NIL;
			}
			px = pNode->pParent;
		}

		return px;
	}

	node_t *begin(void) {
		node_t *px;

		if (m_pRoot == &m_NIL) {
			return &m_NIL;
		}

		for (px = m_pRoot; px->pC[Left] != &m_NIL; px = px->pC[Left]);

		return px;
	}

	node_t *end(void) {
		return &m_NIL;
	}

	void FASTCALL clear(rbtree_allocator<rbtree<KeyT, DataT> > *pAllocator) {
		node_t *px;

		px = begin();
		while (px != end()) {
			node_t *py = next_node(px);
			remove(px);
			pAllocator->free_node(px);
			px = py;
		}

		return;
	}

	unsigned int calc_size(void) {
		node_t *px;
		unsigned int size;

		px = begin();
		size = 0;
		while (px != end()) {
			px = next_node(px);
			size++;
		}

		return size;
	}

private:
	void FASTCALL left_rotate(node_t *px) {
		node_t *py;

		py = px->pC[Right];

		node_t *pyLeft = py->pC[Left];
		px->pC[Right] = pyLeft;
		if (pyLeft != &m_NIL) {
			pyLeft->pParent = px;
		}

		node_t *pxParent = px->pParent;
		py->pParent = pxParent;

		if (pxParent == &m_NIL) {
			m_pRoot = py;
		}
		else {
			pxParent->pC[(px == pxParent->pC[Left]) ? Left : Right] = py;
		}

		py->pC[Left] = px;
		px->pParent = py;

		return;
	}

	void FASTCALL right_rotate(node_t *px) {
		node_t *py;

		py = px->pC[Left];

		node_t *pyRight = py->pC[Right];
		px->pC[Left] = pyRight;
		if (pyRight != &m_NIL) {
			pyRight->pParent = px;
		}

		node_t *pxParent = px->pParent;
		py->pParent = pxParent;

		if (pxParent == &m_NIL) {
			m_pRoot = py;
		}
		else {
			pxParent->pC[(px == pxParent->pC[Right]) ? Right : Left] = py;
		}

		py->pC[Right] = px;
		px->pParent = py;

		return;
	}

private:
	node_t m_NIL;
	node_t *m_pRoot;
};


enum bind_type_t {
	BIND_TYPE_ZERO_PREFIX,
	BIND_TYPE_NON_ZERO_PREFIX,
	BIND_TYPE_NON_ZERO_PREFIX_LRU_LIST
};

#define CT_MIN_FILTER_POINT					0x00
#define CT_MIN_FILTER_LINEAR				0x01
#define CT_MIN_FILTER_ANISOTROPIC			0x02
#define CT_MIN_FILTER_MASK					0x03

#define CT_MIP_FILTER_NONE					0x00
#define CT_MIP_FILTER_POINT					0x04
#define CT_MIP_FILTER_LINEAR				0x08
#define CT_MIP_FILTER_MASK					0x0C

#define CT_MAG_FILTER_LINEAR_NOT_POINT_BIT	0x10

#define CT_HAS_MIPMAPS_BIT					0x20

#define CT_ADDRESS_U_CLAMP					0x40
#define CT_ADDRESS_V_CLAMP					0x80

//Default texture parameters for new D3D texture
const BYTE CT_DEFAULT_TEX_FILTER_PARAMS = CT_MIN_FILTER_POINT | CT_MIP_FILTER_NONE;

struct tex_params_t {
	BYTE filter;
	BYTE reserved1;
	BYTE reserved2;
	BYTE reserved3;
};

//Default texture stage parameters for D3D
const tex_params_t CT_DEFAULT_TEX_PARAMS = { CT_DEFAULT_TEX_FILTER_PARAMS, 0, 0, 0 };

#define DT_NO_SMOOTH_BIT	0x01

struct FCachedTexture {
	IDirect3DTexture8 *pTexObj;
	DWORD LastUsedFrameCount;
	BYTE BaseMip;
	BYTE MaxLevel;
	BYTE UBits, VBits;
	DWORD UClampVal, VClampVal;
	FLOAT UMult, VMult;
	BYTE texType;
	BYTE bindType;
	BYTE treeIndex;
	BYTE dynamicTexBits;
	tex_params_t texParams;
	D3DFORMAT texFormat;
	union {
		void (FASTCALL UD3D8RenderDevice::*pConvertBGRA7777)(const FMipmapBase *, INT);
	};
	FCachedTexture *pPrev;
	FCachedTexture *pNext;
};

class CCachedTextureChain {
public:
	CCachedTextureChain() {
		mark_as_clear();
	}
	~CCachedTextureChain() {
	}

	inline void mark_as_clear(void) {
		m_head.pNext = &m_tail;
		m_tail.pPrev = &m_head;
	}

	inline void unlink(FCachedTexture *pCT) {
		pCT->pPrev->pNext = pCT->pNext;
		pCT->pNext->pPrev = pCT->pPrev;
	}
	inline void link_to_tail(FCachedTexture *pCT) {
		pCT->pPrev = m_tail.pPrev;
		pCT->pNext = &m_tail;
		m_tail.pPrev->pNext = pCT;
		m_tail.pPrev = pCT;
	}

	inline FCachedTexture *begin(void) {
		return m_head.pNext;
	}
	inline FCachedTexture *end(void) {
		return &m_tail;
	}

private:
	FCachedTexture m_head;
	FCachedTexture m_tail;
};

template <class ClassT> class rbtree_node_pool {
public:
	typedef typename ClassT::node_t node_t;

public:
	rbtree_node_pool() {
		m_pTail = 0;
	}
	~rbtree_node_pool() {
	}

	inline void add(node_t *pNode) {
		pNode->pParent = m_pTail;

		m_pTail = pNode;

		return;
	}

	inline node_t *try_remove(void) {
		node_t *pNode;

		if (m_pTail == 0) {
			return 0;
		}

		pNode = m_pTail;

		m_pTail = pNode->pParent;

		return pNode;
	}

	unsigned int calc_size(void) {
		node_t *pNode;
		unsigned int size;

		pNode = m_pTail;
		size = 0;
		while (pNode != 0) {
			pNode = pNode->pParent;
			size++;
		}

		return size;
	}

private:
	node_t *m_pTail;
};

struct FTexInfo {
	QWORD CurrentCacheID;
	DWORD CurrentDynamicPolyFlags;
	FCachedTexture *pBind;
	FLOAT UMult;
	FLOAT VMult;
	FLOAT UPan;
	FLOAT VPan;
};

//Vertex only (for intermediate buffering)
struct FGLVertex {
	FLOAT x;
	FLOAT y;
	FLOAT z;
};

//Vertex and primary color
struct FGLVertexColor {
	FLOAT x;
	FLOAT y;
	FLOAT z;
	DWORD color;
};

//Tex coords
struct FGLTexCoord {
	FLOAT u;
	FLOAT v;
};

//Secondary color
struct FGLSecondaryColor {
	DWORD specular;
};

struct FGLMapDot {
	FLOAT u;
	FLOAT v;
};


//
// A D3D8 rendering device attached to a viewport.
//
class UD3D8RenderDevice : public URenderDevice {
#if defined UTGLR_DX_BUILD || defined UTGLR_UNREAL_BUILD
	DECLARE_CLASS(UD3D8RenderDevice, URenderDevice, CLASS_Config)
#else
	DECLARE_CLASS(UD3D8RenderDevice, URenderDevice, CLASS_Config, D3D8Drv)
#endif

#ifdef UTD3D8R_INCLUDE_VERTEX_PROGRAM_ASM
	void AssembleVertexProgram(void) {
		HRESULT hResult;
		LPD3DXBUFFER pBufConstants;
		LPD3DXBUFFER pBufCompiled;
		LPD3DXBUFFER pBufErrors;

		dout << L"Enter vertex program assembly" << std::endl;

		hResult = D3DXAssembleShader(g_tempShaderString, strlen(g_tempShaderString), D3DXASM_DEBUG, &pBufConstants, &pBufCompiled, &pBufErrors);
		if (FAILED(hResult)) {
			DWORD bufSize;
			std::string sMsg;
			DWORD u;
			VOID *bufPtr;

			dout << L"ERROR ASSEMBLING SHADER" << std::endl;


			bufSize = pBufErrors->GetBufferSize();
			bufPtr = pBufErrors->GetBufferPointer();
			dout << L"Size = " << bufSize << std::endl;
			for (u = 0; u < bufSize; u++) {
				sMsg += *((const char *)bufPtr + u);
			}
			dout << L"Data = " << appFromAnsi(sMsg.c_str()) << std::endl;

			return;
		}
		dout << L"SHADER ASSEMBLED OKAY" << std::endl;

		{
			DWORD compBufSize = pBufCompiled->GetBufferSize();
			VOID *compBufPtr = pBufCompiled->GetBufferPointer();
			DWORD u;
			std::basic_string<TCHAR> sMsg;

			dout << L"Compiled Size = " << compBufSize << std::endl;
			dout << L"Data = " << std::endl;
			for (u = 0; u < compBufSize; u += 4) {
				if ((u % 32) == 0) {
					dout << sMsg << std::endl;
					sMsg.resize(0);
				}
				sMsg += L"0x" + HexString(32, *(DWORD *)((BYTE *)compBufPtr + u)) + L", ";
			}
			dout << sMsg << std::endl;
		}

		dout << L"Leave vertex program assembly" << std::endl;

		return;
	}
#endif

	class ods_buf : public std::basic_stringbuf<TCHAR, std::char_traits<TCHAR> > {
	public:
		virtual ~ods_buf() {
			sync();
		}

	protected:
		int sync() {
#ifdef WIN32
			//Output the string
			TCHAR_CALL_OS(OutputDebugStringW(str().c_str()), OutputDebugStringA(appToAnsi(str().c_str())));
#else
			//Add non-win32 debug output code here
#endif

			//Clear the buffer
			str(std::basic_string<TCHAR>());
			
			return 0;
		}
	};
	class ods_stream : public std::basic_ostream<TCHAR> {
	public:
		ods_stream() : std::basic_ostream<TCHAR, std::char_traits<TCHAR> >(new ods_buf()) {
		}
		~ods_stream() {
			delete rdbuf();
		}
	};

	std::basic_string<TCHAR> HexString(DWORD numBits, DWORD data) {
		std::basic_ostringstream<TCHAR> strHexNum;

		strHexNum << std::hex;
		strHexNum.fill('0');
		strHexNum << std::uppercase;
		strHexNum << std::setw(((numBits + 3) & -4) / 4);
		strHexNum << data;

		return strHexNum.str();
	}

	//Debug stream
	ods_stream dout;

	//Debug bits
	DWORD m_debugBits;
	inline bool DebugBit(DWORD debugBit) {
		return ((m_debugBits & debugBit) != 0);
	}
	enum {
		DEBUG_BIT_BASIC	= 0x00000001,
		DEBUG_BIT_ANY	= 0xFFFFFFFF
	};

	//Fixed texture cache ids
	#define TEX_CACHE_ID_UNUSED		0xFFFFFFFFFFFFFFFF
	#define TEX_CACHE_ID_NO_TEX		0xFFFFFFFF00000010
	#define TEX_CACHE_ID_ALPHA_TEX	0xFFFFFFFF00000020

	//Texture cache id flags
	enum {
		TEX_CACHE_ID_FLAG_MASKED	= 0x1,
		TEX_CACHE_ID_FLAG_16BIT		= 0x2
	};

	//Mask for poly flags that impact texture object state
	#define TEX_DYNAMIC_POLY_FLAGS_MASK		(PF_NoSmooth)

	// Information about a cached texture.
	enum tex_type_t {
		TEX_TYPE_NONE,
		TEX_TYPE_COMPRESSED_DXT1,
		TEX_TYPE_COMPRESSED_DXT1_TO_DXT3,
		TEX_TYPE_COMPRESSED_DXT3,
		TEX_TYPE_COMPRESSED_DXT5,
		TEX_TYPE_PALETTED,
		TEX_TYPE_HAS_PALETTE,
		TEX_TYPE_NORMAL
	};
	#define TEX_FLAG_NO_CLAMP	0x00000001

	struct FTexConvertCtx {
		INT stepBits;
		DWORD texWidthPow2;
		DWORD texHeightPow2;
		const FCachedTexture *pBind;
		D3DLOCKED_RECT lockRect;
	} m_texConvertCtx;


	enum { VERTEX_ARRAY_ALIGN = 64 };	//Must be even multiple of 16B for SSE
	enum { VERTEX_ARRAY_TAIL_PADDING = 72 };	//Must include 8B for half SSE tail

	//Vertex shaders for stream definitions
	DWORD m_standardNTextureStreamShader[MAX_TMUNITS];
	DWORD m_twoColorSingleTextureStreamShader;
	bool m_standardNTextureStreamShaderCreated[MAX_TMUNITS];
	bool m_twoColorSingleTextureStreamShaderCreated;

	//Current vertex shader state tracking
	DWORD m_curVertexShader;

	//Vertex and primary color
	FGLVertex m_csVertexArray[VERTEX_ARRAY_SIZE];
	IDirect3DVertexBuffer8 *m_d3dVertexColorBuffer;
	FGLVertexColor *m_pVertexColorArray;

	//Secondary color
	IDirect3DVertexBuffer8 *m_d3dSecondaryColorBuffer;
	FGLSecondaryColor *m_pSecondaryColorArray;

	//Tex coords
	IDirect3DVertexBuffer8 *m_d3dTexCoordBuffer[MAX_TMUNITS];
	FGLTexCoord *m_pTexCoordArray[MAX_TMUNITS];

	FGLMapDot *MapDotArray;
	BYTE m_MapDotArrayMem[(sizeof(FGLMapDot) * VERTEX_ARRAY_SIZE) + VERTEX_ARRAY_ALIGN + VERTEX_ARRAY_TAIL_PADDING];

	//Vertex buffer state flags
	INT m_curVertexBufferPos;
	bool m_vertexColorBufferNeedsDiscard;
	bool m_secondaryColorBufferNeedsDiscard;
	bool m_texCoordBufferNeedsDiscard[MAX_TMUNITS];

	inline void FlushVertexBuffers(void) {
		unsigned int u;
		m_curVertexBufferPos = 0;
		m_vertexColorBufferNeedsDiscard = true;
		m_secondaryColorBufferNeedsDiscard = true;
		for (u = 0; u < MAX_TMUNITS; u++) {
			m_texCoordBufferNeedsDiscard[u] = true;
		}
#if 0
{
	static int si;
	dout << L"utd3d8r: VB Flush = " << si++ << std::endl;
}
#endif
	}

	inline void LockVertexColorBuffer(void) {
		BYTE *pData;
		DWORD lockFlags;

		lockFlags = D3DLOCK_NOSYSLOCK;
		if (m_vertexColorBufferNeedsDiscard) {
			m_vertexColorBufferNeedsDiscard = false;
			lockFlags |= D3DLOCK_DISCARD;
		}
		else {
			lockFlags |= D3DLOCK_NOOVERWRITE;
		}
		if (FAILED(m_d3dVertexColorBuffer->Lock(0, 0, &pData, lockFlags))) {
			appErrorf(TEXT("Vertex buffer lock failed"));
		}

		m_pVertexColorArray = (FGLVertexColor *)(pData + (m_curVertexBufferPos * sizeof(FGLVertexColor)));
	}
	inline void UnlockVertexColorBuffer(void) {
		if (FAILED(m_d3dVertexColorBuffer->Unlock())) {
			appErrorf(TEXT("Vertex buffer unlock failed"));
		}
	}

	inline void LockSecondaryColorBuffer(void) {
		BYTE *pData;
		DWORD lockFlags;

		lockFlags = D3DLOCK_NOSYSLOCK;
		if (m_secondaryColorBufferNeedsDiscard) {
			m_secondaryColorBufferNeedsDiscard = false;
			lockFlags |= D3DLOCK_DISCARD;
		}
		else {
			lockFlags |= D3DLOCK_NOOVERWRITE;
		}
		if (FAILED(m_d3dSecondaryColorBuffer->Lock(0, 0, &pData, lockFlags))) {
			appErrorf(TEXT("Vertex buffer lock failed"));
		}

		m_pSecondaryColorArray = (FGLSecondaryColor *)(pData + (m_curVertexBufferPos * sizeof(FGLSecondaryColor)));
	}
	inline void UnlockSecondaryColorBuffer(void) {
		if (FAILED(m_d3dSecondaryColorBuffer->Unlock())) {
			appErrorf(TEXT("Vertex buffer unlock failed"));
		}
	}

	inline void LockTexCoordBuffer(DWORD texUnit) {
		BYTE *pData;
		DWORD lockFlags;

		lockFlags = D3DLOCK_NOSYSLOCK;
		if (m_texCoordBufferNeedsDiscard[texUnit]) {
			m_texCoordBufferNeedsDiscard[texUnit] = false;
			lockFlags |= D3DLOCK_DISCARD;
		}
		else {
			lockFlags |= D3DLOCK_NOOVERWRITE;
		}
		if (FAILED(m_d3dTexCoordBuffer[texUnit]->Lock(0, 0, &pData, lockFlags))) {
			appErrorf(TEXT("Vertex buffer lock failed"));
		}

		m_pTexCoordArray[texUnit] = (FGLTexCoord *)(pData + (m_curVertexBufferPos * sizeof(FGLTexCoord)));
	}
	inline void UnlockTexCoordBuffer(DWORD texUnit) {
		if (FAILED(m_d3dTexCoordBuffer[texUnit]->Unlock())) {
			appErrorf(TEXT("Vertex buffer unlock failed"));
		}
	}

	//IsNear bits for detail texturing
	DWORD DetailTextureIsNearArray[VERTEX_ARRAY_SIZE / 3];

	//First and count arrays for glMultiDrawArrays
	INT MultiDrawFirstArray[VERTEX_ARRAY_SIZE / 3];
	INT MultiDrawCountArray[VERTEX_ARRAY_SIZE / 3];

	DWORD m_csPolyCount;
	INT m_csPtCount;

	FLOAT m_csUDot;
	FLOAT m_csVDot;


	// MultiPass rendering information
	struct FGLRenderPass {
		struct FGLSinglePass {
			FTextureInfo* Info;
			DWORD PolyFlags;
			FLOAT PanBias;
		} TMU[MAX_TMUNITS];
	} MultiPass;				// vogel: MULTIPASS!!! ;)

	//Texture state cache information
	BYTE m_texEnableBits;

	//Vertex program ids
	DWORD m_csNTextureVertexShader[MAX_TMUNITS];
	DWORD m_csDetailAlphaVertexShader;
	DWORD m_csDualTextureAndDetailTextureVertexShader;
	DWORD m_standardSingleTextureVertexShader;
	bool m_csNTextureVertexShaderCreated[MAX_TMUNITS];
	bool m_csDetailAlphaVertexShaderCreated;
	bool m_csDualTextureAndDetailTextureVertexShaderCreated;
	bool m_standardSingleTextureVertexShaderCreated;


//	struct FGammaRamp {
//		_WORD red[256];
//		_WORD green[256];
//		_WORD blue[256];
//	};
	struct FByteGammaRamp {
		BYTE red[256];
		BYTE green[256];
		BYTE blue[256];
	};

	// Permanent variables.
	HWND m_hWnd;
	HDC m_hDC;

	UBOOL WasFullscreen;

	bool m_prevSwapBuffersStatus;

	typedef rbtree<DWORD, FCachedTexture> DWORD_CTTree_t;
	typedef rbtree_allocator<DWORD_CTTree_t> DWORD_CTTree_Allocator_t;
	typedef rbtree<QWORD, FCachedTexture> QWORD_CTTree_t;
	typedef rbtree_allocator<QWORD_CTTree_t> QWORD_CTTree_Allocator_t;
	typedef rbtree_node_pool<QWORD_CTTree_t> QWORD_CTTree_NodePool_t;
	typedef DWORD TexPoolMapKey_t;
	typedef rbtree<TexPoolMapKey_t, QWORD_CTTree_NodePool_t> TexPoolMap_t;
	typedef rbtree_allocator<TexPoolMap_t> TexPoolMap_Allocator_t;

	enum { NUM_CTTree_TREES = 16 }; //Must be a power of 2
	inline DWORD CTZeroPrefixCacheIDSuffixToTreeIndex(DWORD CacheIDSuffix) {
		return ((CacheIDSuffix >> 12) & (NUM_CTTree_TREES - 1));
	}
	inline DWORD CTNonZeroPrefixCacheIDSuffixToTreeIndex(DWORD CacheIDSuffix) {
		return ((CacheIDSuffix >> 20) & (NUM_CTTree_TREES - 1));
	}
	inline DWORD MakeTexPoolMapKey(DWORD UBits, DWORD VBits) {
		return ((UBits << 16) | VBits);
	}

	DWORD_CTTree_t m_localZeroPrefixBindTrees[NUM_CTTree_TREES], *m_zeroPrefixBindTrees;
	QWORD_CTTree_t m_localNonZeroPrefixBindTrees[NUM_CTTree_TREES], *m_nonZeroPrefixBindTrees;
	CCachedTextureChain m_localNonZeroPrefixBindChain, *m_nonZeroPrefixBindChain;
	TexPoolMap_t m_localRGBA8TexPool, *m_RGBA8TexPool;

	DWORD_CTTree_Allocator_t m_DWORD_CTTree_Allocator;
	QWORD_CTTree_Allocator_t m_QWORD_CTTree_Allocator;
	TexPoolMap_Allocator_t m_TexPoolMap_Allocator;

	QWORD_CTTree_NodePool_t m_nonZeroPrefixNodePool;

	TArray<FPlane> Modes;

	//Use UViewport* in URenderDevice
	//UViewport* Viewport;


	// Timing.
	DWORD BindCycles, ImageCycles, ComplexCycles, GouraudCycles, TileCycles;

	DWORD m_AASwitchCount;
	DWORD m_stat0Count;
	DWORD m_stat1Count;


	// Hardware constraints.
	struct {
		UBOOL SinglePassDetail;
		UBOOL UseVertexProgram;
	} DCV;

	//UBOOL DetailTextures;
	FLOAT LODBias;
	FLOAT GammaOffset;
	FLOAT GammaOffsetRed;
	FLOAT GammaOffsetGreen;
	FLOAT GammaOffsetBlue;
	INT Brightness;
	UBOOL GammaCorrectScreenshots;
	UBOOL OneXBlending;
	INT MaxLogUOverV;
	INT MaxLogVOverU;
	INT MinLogTextureSize;
	INT MaxLogTextureSize;
	INT MaxAnisotropy;
	INT TMUnits;
	INT MaxTMUnits;
	INT RefreshRate;
	UBOOL UseMultiTexture;
	UBOOL UsePalette;
	UBOOL UsePrecache;
	UBOOL UseTrilinear;
	UBOOL UseVertexSpecular;
	UBOOL UseAlphaPalette;
	UBOOL UseS3TC;
	UBOOL Use16BitTextures;
	UBOOL Use565Textures;
	UBOOL UseCVA;
	UBOOL NoFiltering;
	INT DetailMax;
	UBOOL UseDetailAlpha;
	UBOOL DetailClipping;
	UBOOL ColorizeDetailTextures;
	UBOOL SinglePassFog;
	UBOOL SinglePassDetail;
	UBOOL UseSSE;
	UBOOL UseSSE2;
	UBOOL UseTexIdPool;
	UBOOL UseTexPool;
	UBOOL CacheStaticMaps;
	INT DynamicTexIdRecycleLevel;
	UBOOL TexDXT1ToDXT3;
	UBOOL UseVertexProgram;
	BYTE UseVSync;
	INT FrameRateLimit;
#if defined UTGLR_DX_BUILD || defined UTGLR_UNREAL_BUILD || defined UTGLR_RUNE_BUILD
	FLOAT m_prevFrameTimestamp;
#else
	FTime m_prevFrameTimestamp;
#endif
	UBOOL SceneNodeHack;
	UBOOL SmoothMaskedTextures;
	UBOOL MaskedTextureHack;

	UBOOL UseTripleBuffering;
	UBOOL UsePureDevice;
	UBOOL UseSoftwareVertexProcessing;
	UBOOL RequestHighResolutionZ;
	UBOOL UseAA;
	INT NumAASamples;
	UBOOL NoAATiles;

	UBOOL ZRangeHack;
	UBOOL UseHardwareClipping;
	bool m_useZRangeHack;
	bool m_nearZRangeHackProjectionActive;
	bool m_requestNearZRangeHackProjection;
	bool m_lastRequestNearZRangeHackProjection;

	UBOOL BufferActorTris;
	UBOOL BufferClippedActorTris;
	INT BufferedVerts;

	UBOOL BufferTileQuads;
	INT BufferedTileVerts;

	//Previous lock variables
	//Used to detect changes in settings
	BITFIELD PL_DetailTextures;
	UBOOL PL_OneXBlending;
	INT PL_MaxLogUOverV;
	INT PL_MaxLogVOverU;
	INT PL_MinLogTextureSize;
	INT PL_MaxLogTextureSize;
	UBOOL PL_NoFiltering;
	UBOOL PL_UseTrilinear;
	UBOOL PL_Use16BitTextures;
	UBOOL PL_Use565Textures;
	UBOOL PL_TexDXT1ToDXT3;
	INT PL_MaxAnisotropy;
	UBOOL PL_SmoothMaskedTextures;
	UBOOL PL_MaskedTextureHack;
	FLOAT PL_LODBias;
	UBOOL PL_UsePalette;
	UBOOL PL_UseAlphaPalette;
	UBOOL PL_UseDetailAlpha;
	UBOOL PL_SinglePassDetail;
	UBOOL PL_UseVertexProgram;
	UBOOL PL_UseSSE;
	UBOOL PL_UseSSE2;

	bool m_setGammaRampSucceeded;
	FLOAT SavedGammaCorrection;

	DWORD m_numDepthBits;

	INT AllocatedTextures;

	INT m_rpPassCount;
	INT m_rpTMUnits;
	bool m_rpForceSingle;
	bool m_rpMasked;
	bool m_rpSetDepthEqual;
	DWORD m_rpColor;

	void (UD3D8RenderDevice::*m_pRenderPassesNoCheckSetupProc)(void);
	void (FASTCALL UD3D8RenderDevice::*m_pRenderPassesNoCheckSetup_DualTextureAndDetailTextureProc)(FTextureInfo &);

	DWORD (FASTCALL UD3D8RenderDevice::*m_pBufferDetailTextureDataProc)(FLOAT);

	// Hit info.
	BYTE* HitData;
	INT* HitSize;

	DWORD m_currentFrameCount;

	// Lock variables.
	FPlane FlashScale, FlashFog;
	FLOAT m_RProjZ, m_Aspect;
	FLOAT m_RFX2, m_RFY2;
	INT m_sceneNodeX, m_sceneNodeY;
	bool m_usingAA;
	bool m_curAAEnable;
	bool m_defAAEnable;

	DWORD m_curBlendFlags;
	DWORD m_smoothMaskedTexturesBit;
	DWORD m_lastPolyFlags;
	INT m_bufferActorTrisCutoff;

	DWORD m_detailTextureColor4ub;

	DWORD m_maskedTextureHackMask;

	BYTE m_requestedColorFlags;
	#define CF_COLOR_ARRAY	0x01
	#define CF_FOG_MODE		0x02
#ifdef UTGLR_RUNE_BUILD
	BYTE m_gpAlpha;
#endif

	DWORD m_curTexEnvFlags[MAX_TMUNITS];
	tex_params_t m_curTexStageParams[MAX_TMUNITS];
	FTexInfo TexInfo[MAX_TMUNITS];

	void (FASTCALL *m_pBuffer3BasicVertsProc)(UD3D8RenderDevice *, FTransTexture **);
	void (FASTCALL *m_pBuffer3ColoredVertsProc)(UD3D8RenderDevice *, FTransTexture **);
	void (FASTCALL *m_pBuffer3FoggedVertsProc)(UD3D8RenderDevice *, FTransTexture **);

	void (FASTCALL *m_pBuffer3VertsProc)(UD3D8RenderDevice *, FTransTexture **);

	IDirect3DTexture8 *m_pNoTexObj;
	IDirect3DTexture8 *m_pAlphaTexObj;

	// Static variables.
	static INT NumDevices;
	static INT LockCount;

	static HMODULE hModuleD3d8;
	static LPDIRECT3DCREATE8 pDirect3DCreate8;

	static bool g_gammaFirstTime;
	static bool g_haveOriginalGammaRamp;
//	static FGammaRamp g_originalGammaRamp;


	IDirect3D8 *m_d3d8;
	IDirect3DDevice8 *m_d3dDevice;

	INT m_SetRes_NewX;
	INT m_SetRes_NewY;
	INT m_SetRes_NewColorBytes;
	UBOOL m_SetRes_Fullscreen;
	bool m_SetRes_isDeviceReset;

	D3DCAPS8 m_d3dCaps;
	bool m_palettedTextureCap;
	bool m_dxt1TextureCap;
	bool m_dxt3TextureCap;
	bool m_dxt5TextureCap;
	bool m_16BitTextureCap;
	bool m_alphaTextureCap;

	D3DPRESENT_PARAMETERS m_d3dpp;
	bool m_doSoftwareVertexInit;
	DWORD m_vertexShaderUsage;
	BYTE m_LastZMode;

#ifdef BGRA_MAKE
#undef BGRA_MAKE
#endif
	static inline DWORD BGRA_MAKE(BYTE b, BYTE g, BYTE r, BYTE a) {
		return (a << 24) | (r << 16) | (g << 8) | b;
	}


#if defined WIN32 && defined UTGLR_USE_ASM_CODE
	static inline DWORD FPlaneTo_BGR_A255(const FPlane *pPlane) {
		static FLOAT f255 = 255.0f;
		INT iR, iG, iB;
		__asm {
			mov eax, pPlane
			fld [eax]FVector.X
			fmul [f255]
			fistp [iR]
			fld [eax]FVector.Y
			fmul [f255]
			fistp [iG]
			fld [eax]FVector.Z
			fmul [f255]
			fistp [iB]
		}
		return BGRA_MAKE(iB, iG, iR, 255);
	}
#else
	static inline DWORD FPlaneTo_BGR_A255(const FPlane *pPlane) {
		return BGRA_MAKE(
					appRound(pPlane->Z * 255.0f),
					appRound(pPlane->Y * 255.0f),
					appRound(pPlane->X * 255.0f),
					255);
	}
#endif

#if defined WIN32 && defined UTGLR_USE_ASM_CODE
	static inline DWORD FPlaneTo_BGRClamped_A255(const FPlane *pPlane) {
		static FLOAT f255 = 255.0f;
		INT iR, iG, iB;
		__asm {
			mov eax, pPlane
			fld [eax]FVector.X
			fmul [f255]
			fistp [iR]
			fld [eax]FVector.Y
			fmul [f255]
			fistp [iG]
			fld [eax]FVector.Z
			fmul [f255]
			fistp [iB]
		}
		return BGRA_MAKE(Clamp(iB, 0, 255), Clamp(iG, 0, 255), Clamp(iR, 0, 255), 255);
	}
#else
	static inline DWORD FPlaneTo_BGRClamped_A255(const FPlane *pPlane) {
		return BGRA_MAKE(
					Clamp(appRound(pPlane->Z * 255.0f), 0, 255),
					Clamp(appRound(pPlane->Y * 255.0f), 0, 255),
					Clamp(appRound(pPlane->X * 255.0f), 0, 255),
					255);
	}
#endif

#if defined WIN32 && defined UTGLR_USE_ASM_CODE
	static inline DWORD FPlaneTo_BGR_A0(const FPlane *pPlane) {
		static FLOAT f255 = 255.0f;
		INT iR, iG, iB;
		__asm {
			mov eax, pPlane
			fld [eax]FVector.X
			fmul [f255]
			fistp [iR]
			fld [eax]FVector.Y
			fmul [f255]
			fistp [iG]
			fld [eax]FVector.Z
			fmul [f255]
			fistp [iB]
		}
		return BGRA_MAKE(iB, iG, iR, 0);
	}
#else
	static inline DWORD FPlaneTo_BGR_A0(const FPlane *pPlane) {
		return BGRA_MAKE(
					appRound(pPlane->Z * 255.0f),
					appRound(pPlane->Y * 255.0f),
					appRound(pPlane->X * 255.0f),
					0);
	}
#endif

#if defined WIN32 && defined UTGLR_USE_ASM_CODE
	static inline DWORD FPlaneTo_BGR_Aub(const FPlane *pPlane, BYTE alpha) {
		static FLOAT f255 = 255.0f;
		INT iR, iG, iB;
		__asm {
			mov eax, pPlane
			fld [eax]FVector.X
			fmul [f255]
			fistp [iR]
			fld [eax]FVector.Y
			fmul [f255]
			fistp [iG]
			fld [eax]FVector.Z
			fmul [f255]
			fistp [iB]
		}
		return BGRA_MAKE(iB, iG, iR, alpha);
	}
#else
	static inline DWORD FPlaneTo_RGB_Aub(const FPlane *pPlane, BYTE alpha) {
		return BGRA_MAKE(
					appRound(pPlane->Z * 255.0f),
					appRound(pPlane->Y * 255.0f),
					appRound(pPlane->X * 255.0f),
					alpha);
	}
#endif

#if defined WIN32 && defined UTGLR_USE_ASM_CODE
	static inline DWORD FPlaneTo_BGRA(const FPlane *pPlane) {
		static FLOAT f255 = 255.0f;
		INT iR, iG, iB, iA;
		__asm {
			mov eax, pPlane
			fld [eax]FVector.X
			fmul [f255]
			fistp [iR]
			fld [eax]FVector.Y
			fmul [f255]
			fistp [iG]
			fld [eax]FVector.Z
			fmul [f255]
			fistp [iB]
			fld [eax]FPlane.W
			fmul [f255]
			fistp [iA]
		}
		return BGRA_MAKE(iB, iG, iR, iA);
	}
#else
	static inline DWORD FPlaneTo_BGRA(const FPlane *pPlane) {
		return BGRA_MAKE(
					appRound(pPlane->Z * 255.0f),
					appRound(pPlane->Y * 255.0f),
					appRound(pPlane->X * 255.0f),
					appRound(pPlane->W * 255.0f));
	}
#endif

#if defined WIN32 && defined UTGLR_USE_ASM_CODE
	static inline DWORD FPlaneTo_BGRAClamped(const FPlane *pPlane) {
		static FLOAT f255 = 255.0f;
		INT iR, iG, iB, iA;
		__asm {
			mov eax, pPlane
			fld [eax]FVector.X
			fmul [f255]
			fistp [iR]
			fld [eax]FVector.Y
			fmul [f255]
			fistp [iG]
			fld [eax]FVector.Z
			fmul [f255]
			fistp [iB]
			fld [eax]FPlane.W
			fmul [f255]
			fistp [iA]
		}
		return BGRA_MAKE(Clamp(iB, 0, 255), Clamp(iG, 0, 255), Clamp(iR, 0, 255), Clamp(iA, 0, 255));
	}
#else
	static inline DWORD FPlaneTo_BGRAClamped(const FPlane *pPlane) {
		return BGRA_MAKE(
					Clamp(appRound(pPlane->Z * 255.0f), 0, 255),
					Clamp(appRound(pPlane->Y * 255.0f), 0, 255),
					Clamp(appRound(pPlane->X * 255.0f), 0, 255),
					Clamp(appRound(pPlane->W * 255.0f), 0, 255));
	}
#endif

#if defined WIN32 && defined UTGLR_USE_ASM_CODE
	static inline DWORD FPlaneTo_BGRScaled_A255(const FPlane *pPlane, FLOAT rgbScale) {
		INT iR, iG, iB;
		__asm {
			mov eax, pPlane
			fld [eax]FVector.X
			fmul [rgbScale]
			fistp [iR]
			fld [eax]FVector.Y
			fmul [rgbScale]
			fistp [iG]
			fld [eax]FVector.Z
			fmul [rgbScale]
			fistp [iB]
		}
		return BGRA_MAKE(iB, iG, iR, 255);
	}
#else
	static inline DWORD FPlaneTo_BGRScaled_A255(const FPlane *pPlane, FLOAT rgbScale) {
		return BGRA_MAKE(
					appRound(pPlane->Z * rgbScale),
					appRound(pPlane->Y * rgbScale),
					appRound(pPlane->X * rgbScale),
					255);
	}
#endif


	// UObject interface.
	void StaticConstructor();


	// Implementation.
	void FASTCALL SC_AddBoolConfigParam(DWORD BitMaskOffset, const TCHAR *pName, UBOOL &param, ECppProperty EC_CppProperty, INT InOffset, UBOOL defaultValue);
	void FASTCALL SC_AddIntConfigParam(const TCHAR *pName, INT &param, ECppProperty EC_CppProperty, INT InOffset, INT defaultValue);
	void FASTCALL SC_AddFloatConfigParam(const TCHAR *pName, FLOAT &param, ECppProperty EC_CppProperty, INT InOffset, FLOAT defaultValue);
	void FASTCALL SC_AddByteConfigParam(const TCHAR *pName, BYTE &param, ECppProperty EC_CppProperty, INT InOffset, BYTE defaultValue);
	void FASTCALL SC_AddEnumConfigParam(const TCHAR *pName, BYTE &param, ECppProperty EC_CppProperty, INT InOffset, UEnum* Enum, BYTE defaultValue);

	void FASTCALL DbgPrintInitParam(const TCHAR *pName, INT value);
	void FASTCALL DbgPrintInitParam(const TCHAR *pName, FLOAT value);

#ifdef UTGLR_INCLUDE_SSE_CODE
	static bool CPU_DetectCPUID(void);
	static bool CPU_DetectSSE(void);
	static bool CPU_DetectSSE2(void);
#endif //UTGLR_INCLUDE_SSE_CODE

	void BuildGammaRamp(float redGamma, float greenGamma, float blueGamma, int brightness, D3DGAMMARAMP &ramp);
	void BuildGammaRamp(float redGamma, float greenGamma, float blueGamma, int brightness, FByteGammaRamp &ramp);
	void SetGamma(FLOAT GammaCorrection);
	void ResetGamma(void);
	BYTE SetZTestMode(BYTE Mode);

	UBOOL FailedInitf(const TCHAR* Fmt, ...);
	void Exit();
	void ShutdownAfterError();

	UBOOL SetRes(INT NewX, INT NewY, INT NewColorBytes, UBOOL Fullscreen);
	void UnsetRes();

	bool FASTCALL CheckDepthFormat(D3DFORMAT adapterFormat, D3DFORMAT backBufferFormat, D3DFORMAT depthBufferFormat);

	void ConfigValidate_RefreshDCV(void);
	void ConfigValidate_RequiredExtensions(void);
	void ConfigValidate_Main(void);

	void InitPermanentResourcesAndRenderingState(void);
	void FreePermanentResources(void);


	UBOOL Init(UViewport* InViewport, INT NewX, INT NewY, INT NewColorBytes, UBOOL Fullscreen);

	static QSORT_RETURN CDECL CompareRes(const FPlane* A, const FPlane* B) {
		return (QSORT_RETURN) (((A->X - B->X) != 0.0) ? (A->X - B->X) : (A->Y - B->Y));
	}

	UBOOL Exec(const TCHAR* Cmd, FOutputDevice& Ar);
	void Lock(FPlane InFlashScale, FPlane InFlashFog, FPlane ScreenClear, DWORD RenderLockFlags, BYTE* InHitData, INT* InHitSize);
	void SetSceneNode(FSceneNode* Frame);
	void Unlock(UBOOL Blit);
	void Flush(UBOOL AllowPrecache);

	void DrawComplexSurface(FSceneNode* Frame, FSurfaceInfo& Surface, FSurfaceFacet& Facet);
#ifdef UTGLR_RUNE_BUILD
	void PreDrawFogSurface();
	void PostDrawFogSurface();
	void DrawFogSurface(FSceneNode* Frame, FFogSurf &FogSurf);
	void PreDrawGouraud(FSceneNode* Frame, FLOAT FogDistance, FPlane FogColor);
	void PostDrawGouraud(FLOAT FogDistance);
#elif UTGLR_UNREAL_227_BUILD
	void PreDrawFogSurface();
	void PostDrawFogSurface();
	void DrawFogSurface(FSceneNode* Frame, FFogSurf &FogSurf);
	void PreDrawGouraud(FSceneNode* Frame, FFogSurf &FogSurf);
	void PostDrawGouraud(FLOAT FogDistance);
#endif
	void DrawGouraudPolygonOld(FSceneNode* Frame, FTextureInfo& Info, FTransTexture** Pts, INT NumPts, DWORD PolyFlags, FSpanBuffer* Span);
	void DrawGouraudPolygon(FSceneNode* Frame, FTextureInfo& Info, FTransTexture** Pts, INT NumPts, DWORD PolyFlags, FSpanBuffer* Span);
	void DrawTile(FSceneNode* Frame, FTextureInfo& Info, FLOAT X, FLOAT Y, FLOAT XL, FLOAT YL, FLOAT U, FLOAT V, FLOAT UL, FLOAT VL, class FSpanBuffer* Span, FLOAT Z, FPlane Color, FPlane Fog, DWORD PolyFlags);
	void Draw2DLine(FSceneNode* Frame, FPlane Color, DWORD LineFlags, FVector P1, FVector P2);
	void Draw3DLine(FSceneNode* Frame, FPlane Color, DWORD LineFlags, FVector P1, FVector P2);
	void Draw2DPoint(FSceneNode* Frame, FPlane Color, DWORD LineFlags, FLOAT X1, FLOAT Y1, FLOAT X2, FLOAT Y2, FLOAT Z);

	void ClearZ(FSceneNode* Frame);
	void PushHit(const BYTE* Data, INT Count);
	void PopHit(INT Count, UBOOL bForce);
	void GetStats(TCHAR* Result);
	void ReadPixels(FColor* Pixels, UBOOL bGammaCorrectOutput);
	void EndFlash();
	void PrecacheTexture(FTextureInfo& Info, DWORD PolyFlags);


	void InitNoTextureSafe(void);
	void InitAlphaTextureSafe(void);

	void ScanForOldTextures(void);

	inline void SetNoTexture(INT Multi) {
		if (TexInfo[Multi].CurrentCacheID != TEX_CACHE_ID_NO_TEX) {
			SetNoTextureNoCheck(Multi);
		}
	}
	inline void SetAlphaTexture(INT Multi) {
		if (TexInfo[Multi].CurrentCacheID != TEX_CACHE_ID_ALPHA_TEX) {
			SetAlphaTextureNoCheck(Multi);
		}
	}

	void FASTCALL SetNoTextureNoCheck(INT Multi);
	void FASTCALL SetAlphaTextureNoCheck(INT Multi);

	inline void SetTexture(INT Multi, FTextureInfo& Info, DWORD PolyFlags, FLOAT PanBias) {
		FTexInfo& Tex = TexInfo[Multi];
		QWORD CacheID;
		DWORD DynamicPolyFlags;

		// Set panning.
		Tex.UPan = Info.Pan.X + PanBias * Info.UScale;
		Tex.VPan = Info.Pan.Y + PanBias * Info.VScale;

		//PF_Memorized used internally to indicate 16-bit texture
		PolyFlags &= ~PF_Memorized;

		//Load texture cache id
		CacheID = Info.CacheID;

		//Only attempt to alter texture cache id on certain textures
		if ((CacheID & 0xFF) == 0xE0) {
			//Alter texture cache id if masked texture hack is enabled and texture is masked
			CacheID |= ((PolyFlags & PF_Masked) ? TEX_CACHE_ID_FLAG_MASKED : 0) & m_maskedTextureHackMask;

			//Check for 16 bit texture option
			if (Use16BitTextures) {
				if (Info.Palette && (Info.Palette[128].A == 255)) {
					CacheID |= TEX_CACHE_ID_FLAG_16BIT;
					PolyFlags |= PF_Memorized;
				}
			}
		}

		//Get dynamic poly flags
		DynamicPolyFlags = PolyFlags & TEX_DYNAMIC_POLY_FLAGS_MASK;

		// Find in cache.
		if ((CacheID == Tex.CurrentCacheID) && (DynamicPolyFlags == Tex.CurrentDynamicPolyFlags) && !Info.bRealtimeChanged) {
			return;
		}

		//Update soon to be current texture cache id
		Tex.CurrentCacheID = CacheID;
		Tex.CurrentDynamicPolyFlags = DynamicPolyFlags;

		SetTextureNoCheck(Multi, Tex, Info, PolyFlags);

		return;
	}

	inline void SetTextureNoPanBias(INT Multi, FTextureInfo& Info, DWORD PolyFlags) {
		FTexInfo& Tex = TexInfo[Multi];
		QWORD CacheID;
		DWORD DynamicPolyFlags;

		// Set panning.
		Tex.UPan = Info.Pan.X;
		Tex.VPan = Info.Pan.Y;

		//PF_Memorized used internally to indicate 16-bit texture
		PolyFlags &= ~PF_Memorized;

		//Load texture cache id
		CacheID = Info.CacheID;

		//Only attempt to alter texture cache id on certain textures
		if ((CacheID & 0xFF) == 0xE0) {
			//Alter texture cache id if masked texture hack is enabled and texture is masked
			CacheID |= ((PolyFlags & PF_Masked) ? TEX_CACHE_ID_FLAG_MASKED : 0) & m_maskedTextureHackMask;

			//Check for 16 bit texture option
			if (Use16BitTextures) {
				if (Info.Palette && (Info.Palette[128].A == 255)) {
					CacheID |= TEX_CACHE_ID_FLAG_16BIT;
					PolyFlags |= PF_Memorized;
				}
			}
		}

		//Get dynamic poly flags
		DynamicPolyFlags = PolyFlags & TEX_DYNAMIC_POLY_FLAGS_MASK;

		// Find in cache.
		if ((CacheID == Tex.CurrentCacheID) && (DynamicPolyFlags == Tex.CurrentDynamicPolyFlags) && !Info.bRealtimeChanged) {
			return;
		}

		//Update soon to be current texture cache id
		Tex.CurrentCacheID = CacheID;
		Tex.CurrentDynamicPolyFlags = DynamicPolyFlags;

		SetTextureNoCheck(Multi, Tex, Info, PolyFlags);

		return;
	}

	void FASTCALL SetTextureNoCheck(DWORD texNum, FTexInfo& Tex, FTextureInfo& Info, DWORD PolyFlags);
	void FASTCALL CacheTextureInfo(FCachedTexture *pBind, const FTextureInfo &Info, DWORD PolyFlags);

	void FASTCALL ConvertDXT1_DXT1(const FMipmapBase *Mip, INT Level);
	void FASTCALL ConvertDXT1_DXT3(const FMipmapBase *Mip, INT Level);
	void FASTCALL ConvertDXT35_DXT35(const FMipmapBase *Mip, INT Level);
//	void FASTCALL ConvertP8_P8(const FMipmapBase *Mip, INT Level);
//	void FASTCALL ConvertP8_P8_NoStep(const FMipmapBase *Mip, INT Level);
	void FASTCALL ConvertP8_RGBA8888(const FMipmapBase *Mip, const FColor *Palette, INT Level);
	void FASTCALL ConvertP8_RGBA8888_NoStep(const FMipmapBase *Mip, const FColor *Palette, INT Level);
	void FASTCALL ConvertP8_RGB565(const FMipmapBase *Mip, const FColor *Palette, INT Level);
	void FASTCALL ConvertP8_RGB565_NoStep(const FMipmapBase *Mip, const FColor *Palette, INT Level);
	void FASTCALL ConvertP8_RGBA5551(const FMipmapBase *Mip, const FColor *Palette, INT Level);
	void FASTCALL ConvertP8_RGBA5551_NoStep(const FMipmapBase *Mip, const FColor *Palette, INT Level);
	void FASTCALL ConvertBGRA7777_BGRA8888(const FMipmapBase *Mip, INT Level);
	void FASTCALL ConvertBGRA7777_BGRA8888_NoClamp(const FMipmapBase *Mip, INT Level);
	void FASTCALL ConvertBGRA8888(const FMipmapBase* Mip, INT Level);
	void FASTCALL ConvertBGRA8888_NoClamp(const FMipmapBase* Mip, INT Level);

	inline void SetBlend(DWORD PolyFlags) {
#ifdef UTGLR_RUNE_BUILD
		if (PolyFlags & PF_AlphaBlend) {
			if (!(PolyFlags & PF_Masked)) {
				PolyFlags |= PF_Occlude;
			}
			else {
				PolyFlags &= ~PF_Masked;
			}
		}
		else
#endif
		if (!(PolyFlags & (PF_Translucent | PF_Modulated | PF_AlphaBlend | PF_Highlighted))) {
			PolyFlags |= PF_Occlude;
		}
		else if (PolyFlags & PF_Translucent) {
			PolyFlags &= ~PF_Masked;
		}

		//Only check relevant blend flags
#ifdef UTGLR_RUNE_BUILD
		DWORD blendFlags = PolyFlags & (PF_Translucent | PF_Modulated | PF_Invisible | PF_Occlude | PF_Masked | PF_Highlighted | PF_RenderFog | PF_AlphaBlend);
#elif  UTGLR_UNREAL_227_BUILD
		DWORD blendFlags = PolyFlags & (PF_Translucent | PF_Modulated | PF_Invisible | PF_Occlude | PF_Masked | PF_Highlighted | PF_RenderFog | PF_AlphaBlend);
#else
		DWORD blendFlags = PolyFlags & (PF_Translucent | PF_Modulated | PF_Invisible | PF_Occlude | PF_Masked | PF_Highlighted | PF_RenderFog);
#endif
		if (m_curBlendFlags != blendFlags) {
			SetBlendNoCheck(blendFlags);
		}
	}
	void FASTCALL SetBlendNoCheck(DWORD blendFlags);

	inline void SetTexEnv(INT texUnit, DWORD PolyFlags) {
		//Only check relevant tex env flags
		DWORD texEnvFlags = PolyFlags & (PF_Modulated | PF_Highlighted | PF_Memorized | PF_FlatShaded);
		//Modulated by default
		if ((texEnvFlags & (PF_Modulated | PF_Highlighted | PF_Memorized)) == 0) {
			texEnvFlags |= PF_Modulated;
		}
		if (m_curTexEnvFlags[texUnit] != texEnvFlags) {
			SetTexEnvNoCheck(texUnit, texEnvFlags);
		}
	}
	void InitOrInvalidateTexEnvState(void);
	void FASTCALL SetTexLODBiasState(INT TMUnits);
	void FASTCALL SetTexMaxAnisotropyState(INT TMUnits);
	void FASTCALL SetTexEnvNoCheck(DWORD texUnit, DWORD texEnvFlags);

	inline void SetTexFilter(DWORD texNum, BYTE texFilterParams) {
		if (m_curTexStageParams[texNum].filter != texFilterParams) {
			SetTexFilterNoCheck(texNum, texFilterParams);
		}
	}
	void FASTCALL SetTexFilterNoCheck(DWORD texNum, BYTE texFilterParams);


	inline void SetDefaultTextureState(void) {
		//Check if only texture unit zero is enabled
		if (m_texEnableBits != 0x1) {
			DWORD texUnit;
			DWORD texBit;

			//Disable all texture units except texture unit zero
			for (texUnit = 1, texBit = 0x2; m_texEnableBits != 0x1; texUnit++, texBit <<= 1) {
				//See if the texture unit is enabled
				if (texBit & m_texEnableBits) {
					//Update tex enable bits (sub to clear known set bit)
					m_texEnableBits -= texBit;

					//Mark the texture unit as disabled
					m_curTexEnvFlags[texUnit] = 0;

					//Disable the texture unit
					m_d3dDevice->SetTextureStageState(texUnit, D3DTSS_COLOROP, D3DTOP_DISABLE);
					m_d3dDevice->SetTextureStageState(texUnit, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
				}
			}
		}

		return;
	}

	inline void DisableSubsequentTextures(DWORD firstTexUnit) {
		DWORD texUnit;
		BYTE texBit = 1U << firstTexUnit;

		//Disable subsequent texture units
		for (texUnit = firstTexUnit; texBit <= m_texEnableBits; texUnit++, texBit <<= 1) {
			//See if the texture unit is enabled
			if (texBit & m_texEnableBits) {
				//Update tex enable bits (sub to clear known set bit)
				m_texEnableBits -= texBit;

				//Mark the texture unit as disabled
				m_curTexEnvFlags[texUnit] = 0;

				//Disable the texture unit
				m_d3dDevice->SetTextureStageState(texUnit, D3DTSS_COLOROP, D3DTOP_DISABLE);
				m_d3dDevice->SetTextureStageState(texUnit, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
			}
		}

		return;
	}

	inline void SetDefaultStreamState(void) {
		if (m_curVertexShader != m_standardNTextureStreamShader[0]) {
			SetStreamStateNoCheck(m_standardNTextureStreamShader[0]);
		}
	}
	inline void SetStreamState(DWORD streamShader) {
		if (m_curVertexShader != streamShader) {
			SetStreamStateNoCheck(streamShader);
		}
	}
	void FASTCALL SetStreamStateNoCheck(DWORD streamShader);

	inline void SetDefaultProjectionState(void) {
		//See if non-default projection is active
		if (m_nearZRangeHackProjectionActive) {
			SetProjectionStateNoCheck(false);
		}
	}

	inline void SetProjectionState(bool requestNearZRangeHackProjection) {
		if (requestNearZRangeHackProjection != m_nearZRangeHackProjectionActive) {
			//Set requested projection state
			SetProjectionStateNoCheck(requestNearZRangeHackProjection);
		}
	}

	inline void SetDefaultAAState(void) {
		if (m_defAAEnable != m_curAAEnable) {
			SetAAStateNoCheck(m_defAAEnable);
		}
	}
	inline void SetDisabledAAState(void) {
		if (m_curAAEnable && m_usingAA) {
			SetAAStateNoCheck(false);
		}
	}
	void FASTCALL SetAAStateNoCheck(bool AAEnable);

	bool InitializeVertexPrograms(void);
	bool FASTCALL LoadVertexProgram(const DWORD *, const DWORD *, DWORD *, const TCHAR *);
	void TryInitializeVertexProgramMode(void);
	void ShutdownVertexProgramMode(void);

	void FASTCALL SetProjectionStateNoCheck(bool);
	void SetOrthoProjection(void);

	inline void RenderPasses(void) {
		if (m_rpPassCount != 0) {
			RenderPassesExec();
		}
	}
	inline void RenderPasses_DualTextureAndDetailTexture(FTextureInfo &DetailTextureInfo) {
		RenderPassesExec_DualTextureAndDetailTexture(DetailTextureInfo);
	}

	inline void AddRenderPass(FTextureInfo* Info, DWORD PolyFlags, FLOAT PanBias) {
		INT rpPassCount = m_rpPassCount;

		MultiPass.TMU[rpPassCount].Info      = Info;
		MultiPass.TMU[rpPassCount].PolyFlags = PolyFlags;
		MultiPass.TMU[rpPassCount].PanBias   = PanBias;

		//Single pass rendering forced here by setting m_rpTMUnits equal to 1
		rpPassCount++;
		m_rpPassCount = rpPassCount;
		if (rpPassCount >= m_rpTMUnits) {
			//m_rpPassCount will never be equal to 0 here
			RenderPassesExec();
		}
	}

	void RenderPassesExec(void);
	void FASTCALL RenderPassesExec_DualTextureAndDetailTexture(FTextureInfo &DetailTextureInfo);

	void RenderPassesNoCheckSetup(void);
	void RenderPassesNoCheckSetup_VP(void);
	void FASTCALL RenderPassesNoCheckSetup_DualTextureAndDetailTexture(FTextureInfo &);
	void FASTCALL RenderPassesNoCheckSetup_DualTextureAndDetailTexture_VP(FTextureInfo &);

	INT FASTCALL BufferStaticComplexSurfaceGeometry(const FSurfaceFacet&);
	INT FASTCALL BufferStaticComplexSurfaceGeometry_VP(const FSurfaceFacet&);
	DWORD FASTCALL BufferDetailTextureData(FLOAT);
#ifdef UTGLR_INCLUDE_SSE_CODE
	DWORD FASTCALL BufferDetailTextureData_SSE2(FLOAT);
#endif //UTGLR_INCLUDE_SSE_CODE

	void FASTCALL DrawDetailTexture(FTextureInfo &, bool);
	void FASTCALL DrawDetailTexture_VP(FTextureInfo &);

	inline void EndGouraudPolygonBuffering(void) {
		if (BufferedVerts > 0) {
			EndBufferingNoCheck();
		}
	}
	inline void EndTileBuffering(void) {
		if (BufferedTileVerts > 0) {
			EndTileBufferingNoCheck();
		}
	}
	inline void EndBuffering(void) {
		if (BufferedVerts > 0) {
			EndBufferingNoCheck();
		}
		if (BufferedTileVerts > 0) {
			EndTileBufferingNoCheck();
		}
	}
	void EndBufferingNoCheck(void);
	void EndTileBufferingNoCheck(void);

	void FASTCALL BufferAdditionalClippedVerts(FTransTexture** Pts, INT NumPts);
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
