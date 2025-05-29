#include <gx.h>

namespace rw{
namespace gcn{

void registerPlatformPlugins(void);

extern Device renderdevice;
enum AttribType{

    ATTRIB_POS = GX_VA_POS,
    ATTRIB_NORMAL = GX_VA_NRM,
    ATTRIB_COLOR = GX_VA_CLR0,
    ATTRIB_COLOR1 = GX_VA_CLR1,
    ATTRIB_TEX0 = GX_VA_TEX0,
    ATTRIB_TEX1 = GX_VA_TEX1,
    ATTRIB_TEX2 = GX_VA_TEX2,
    ATTRIB_TEX3 = GX_VA_TEX3,
    ATTRIB_TEX4 = GX_VA_TEX4,
    ATTRIB_TEX5 = GX_VA_TEX5,
    ATTRIB_TEX6 = GX_VA_TEX6,
    ATTRIB_TEX7 = GX_VA_TEX7,

};

enum AttribComponentType{
    ATTRIB_COMP_RGB = GX_CLR_RGB,
    ATTRIB_COMP_RGBA = GX_CLI_RGBA,
    ATTRIB_COMP_NRM_XYZ = GX_NRM_XYZ,
    ATTRIB_COMP_XY = GX_POS_XY,
    ATTRIB_COMP_XYZ = GX_POS_XYZ,
};

//Note that the name is misleading. But I don't have a better name
enum AttribComponentSize{
    ATTRIB_COMP_SIZE_F32 = GX_F32,
    ATTRIB_COMP_SIZE_RGB565 = GX_RGB565,
    ATTRIB_COMP_SIZE_RGB8 = GX_RGB8,
    ATTRIB_COMP_SIZE_RGBA4 = GX_RGBA4,
    ATTRIB_COMP_SIZE_RGBA8 = GX_RGBA8,
    ATTRIB_COMP_SIZE_RGBX8 = GX_RGBX8,
    ATTRIB_COMP_SIZE_S16 = GX_S16,
    ATTRIB_COMP_SIZE_S8 = GX_S8,
    ATTRIB_COMP_SIZE_U16 = GX_U16,
    ATTRIB_COMP_SIZE_U8 = GX_U8,
}
struct EngineOpenParams{
	uint8 videoMode = 0;
};

//These are the args to GX_SetVtxAttrFmt.
//Minus the first one, as we're going to be mainly relying on the first VAT
struct AttribDesc{
    uint8 AttribType : 5;
    uint8 AttribComponentType : 2;
    uint8 AttribComponentSize : 3
};

struct InstanceData
{
    uint32 numIndices;
    Material *material;
    void *indexBuffer;

};

//If numAttribs is set to 0, we use the last AttribDesc set
struct InstanceDataHeader : rw::InstanceDataHeader
{
    uint32 numVertices;
    uint16 serialNumber;
    uint32 primType;
    int32 stride;
    void *vertexBuffer;
    InstanceData *inst;
    int32 numAttribs;
    AttribDesc *AttribDesc;
    uint32 totalNumVertex;

};

struct Im3DVertex
{
	V3d     position;
	V3d     normal;		// librw extension
	uint8   r, g, b, a;
	float32 u, v;

	void setX(float32 x) { this->position.x = x; }
	void setY(float32 y) { this->position.y = y; }
	void setZ(float32 z) { this->position.z = z; }
	void setNormalX(float32 x) { this->normal.x = x; }
	void setNormalY(float32 y) { this->normal.y = y; }
	void setNormalZ(float32 z) { this->normal.z = z; }
	void setColor(uint8 r, uint8 g, uint8 b, uint8 a) {
		this->r = r; this->g = g; this->b = b; this->a = a; }
	void setU(float32 u) { this->u = u; }
	void setV(float32 v) { this->v = v; }

	float getX(void) { return this->position.x; }
	float getY(void) { return this->position.y; }
	float getZ(void) { return this->position.z; }
	float getNormalX(void) { return this->normal.x; }
	float getNormalY(void) { return this->normal.y; }
	float getNormalZ(void) { return this->normal.z; }
	RGBA getColor(void) { return makeRGBA(this->r, this->g, this->b, this->a); }
	float getU(void) { return this->u; }
	float getV(void) { return this->v; }
};
extern RGBA im3dMaterialColor;
extern SurfaceProperties im3dSurfaceProps;

struct Im2DVertex
{
	float32 x, y, z, w;
	uint8   r, g, b, a;
	float32 u, v;

	void setScreenX(float32 x) { this->x = x; }
	void setScreenY(float32 y) { this->y = y; }
	void setScreenZ(float32 z) { this->z = z; }
	// This is a bit unefficient but we have to counteract GL's divide, so multiply
	void setCameraZ(float32 z) { this->w = z; }
	void setRecipCameraZ(float32 recipz) { this->w = 1.0f/recipz; }
	void setColor(uint8 r, uint8 g, uint8 b, uint8 a) {
		this->r = r; this->g = g; this->b = b; this->a = a; }
	void setU(float32 u, float recipz) { this->u = u; }
	void setV(float32 v, float recipz) { this->v = v; }

	float getScreenX(void) { return this->x; }
	float getScreenY(void) { return this->y; }
	float getScreenZ(void) { return this->z; }
	float getCameraZ(void) { return this->w; }
	float getRecipCameraZ(void) { return 1.0f/this->w; }
	RGBA getColor(void) { return makeRGBA(this->r, this->g, this->b, this->a); }
	float getU(void) { return this->u; }
	float getV(void) { return this->v; }
};
void setAttribs(AttribDesc *attribDescs, int32 numAttribs);

//Normally, this would just be GXTexObj, but for some reason
//the program cannot access the attributes of the texture object
//so I have to esssentially put all these attributes here.
struct gcnRaster
{

	//And my previous setup ended up useless cause I need gx.h to get a GXTexObj. Fuck me.
	GXTexObj* textureObject;
	//This holds a pointer to the 32-bit aligned image data. 
	void* texture;
	//The maximum value is 1024. 11 bits is all we need.
	uint16 width : 11;
	uint16 height : 11;

	
	uint8 format;
	//texture coord wrapping strategy in the S direction... whatever that means
	uint8 s_wrap;
	//THERES A T DIRECTION?
	uint8 t_wrap;
	//This is whether trilinear filtering will be used
	uint8 mipmap;

	//The texmap number. Essentially the texture id. You can create more than 8 textures
	//But only 8 textures can be applied to a surface at once, and many altercations done rely on this ID.
	uint8 texid;

};

//A lot of stuff I don't understand. like why do this?
extern int32 nativeRasterOffset;
#define GETGCNRASTEREXT(raster) PLUGINOFFSET(rw::gcn::gcnRaster, raster rw::gcn::nativeRasterOffset)

}
}