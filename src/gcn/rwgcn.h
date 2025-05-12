namespace rw{
namespace gcn{

void registerPlatformPlugins(void);

extern Device renderdevice;
//So as to prevent the need for including gx.h,
//I will be setting these attribute types to the corresponding number
//as opposed to the defined constant
enum AttribType{

    ATTRIB_POS = 9,
    ATTRIB_NORMAL = 10,
    ATTRIB_COLOR = 11,
    ATTRIB_COLOR1 = 12,
    ATTRIB_TEX0 = 13,
    ATTRIB_TEX1 = 14,
    ATTRIB_TEX2 = 15,
    ATTRIB_TEX3 = 16,
    ATTRIB_TEX4 = 17,
    ATTRIB_TEX5 = 18,
    ATTRIB_TEX6 = 19,
    ATTRIB_TEX7 = 20,

};

enum AttribComponentType{
    ATTRIB_COMP_RGB = 0,
    ATTRIB_COMP_RGBA = 1,
    ATTRIB_COMP_NRM_XYZ = 0,
    ATTRIB_COMP_XY = 0,
    ATTRIB_COMP_XYZ = 1,
};

//Note that the name is misleading. But I don't have a better name
enum AttribComponentSize{
    ATTRIB_COMP_SIZE_F32 = 4,
    ATTRIB_COMP_SIZE_RGB565 = 0,
    ATTRIB_COMP_SIZE_RGB8 = 1,
    ATTRIB_COMP_SIZE_RGBA4 = 3,
    ATTRIB_COMP_SIZE_RGBA8 = 5,
    ATTRIB_COMP_SIZE_RGBX8 = 2,
    ATTRIB_COMP_SIZE_S16 = 3,
    ATTRIB_COMP_SIZE_S8 = 1,
    ATTRIB_COMP_SIZE_U16 = 2,
    ATTRIB_COMP_SIZE_U8 = 0,
}

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

}
}