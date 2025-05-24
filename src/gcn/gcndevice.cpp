#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "../rwbase.h"
#include "../rwerror.h"
#include "../rwplg.h"
#include "../rwpipeline.h"
#include "../rwobjects.h"
#include "../rwengine.h"

#include "rwgcn.h"
#include "rwgcnimpl.h"

#define GX_FIFO_SIZE	(256*1024)

#include <gccore.h>
#include <ogcsys.h>

namespace rw {
namespace gcn {

GXRModeObj *vidmode;
//external framebuffer to copy to. Yes, this is double buffering
static void *xfb[2] = {NULL,NULL};
static uint8 curr_fb = 0;

void *gcn_fifo;
int32 alphaFunc;
float32 alphaRef;

GXRModeObj *vidmodes[] = [&TVNtsc240Ds, &TVNtsc240DsAa, &TVNtsc240Int, &TVNtsc240IntAa, &TVNtsc480Int, 
&TVNtsc480IntDf, &TVNtsc480IntAa, &TVNtsc480Prog, &TVNtsc480ProgSoft, &TVNtsc480ProgAa, 
&TVMpal240Ds, &TVMpal240DsAa, &TVMpal240Int, &TVMpal240IntAa, &TVMpal480Int, 
&TVMpal480IntDf, &TVMpal480IntAa, &TVMpal480Prog, &TVMpal480ProgSoft, &TVMpal480ProgAa, 
&TVPal264Ds, &TVPal264DsAa, &TVPal264Int, &TVPal264IntAa, &TVPal528Int, 
&TVPal528IntDf, &TVPal524IntAa, &TVPal576IntDfScale, &TVPal528Prog, &TVPal528ProgSoft, 
&TVPal524ProgAa, &TVPal576ProgScale, &TVEurgb60Hz240Ds, &TVEurgb60Hz240DsAa, &TVEurgb60Hz240Int, 
&TVEurgb60Hz240IntAa, &TVEurgb60Hz480Int, &TVEurgb60Hz480IntDf, &TVEurgb60Hz480IntAa, &TVEurgb60Hz480Prog, 
&TVEurgb60Hz480ProgSoft, &TVEurgb60Hz480ProgAa, &TVRgb480Prog, &TVRgb480ProgSoft, &TVRgb480ProgAa];



struct RwRasterStateCache {
	Raster *raster;
	Texture::Addressing addressingU;
	Texture::Addressing addressingV;
	Texture::FilterMode filter;
};

struct RwStateCache {
	bool32 vertexAlpha;
	uint32 alphaTestEnable;
	uint32 alphaFunc;
	bool32 textureAlpha;
	bool32 blendEnable;
	uint32 srcblend, destblend;
	uint32 zwrite;
	uint32 ztest;
	uint32 cullmode;
	uint32 fogEnable;
	float32 fogStart;
	float32 fogEnd;

	// emulation of PS2 GS
	bool32 gsalpha;
	uint32 gsalpharef;

	RwRasterStateCache texstage[MAXNUMSTAGES];
};
static RwStateCache rwStateCache;


//This is the render state for the GX
//I named all my funcs with GCN and stuck with it.
struct GCNState{
    uint8 blendEnable;
    uint8 srcblend : 3;
    uint8 destblend : 3;

    uint8 depthTest : 1;
	uint32 depthFunc;
    bool32 depthMask;

    uint8 cullEnable : 1;
    uint32 cullFace;


    uint8 fogEnable : 1;
    RGBAf fogColor;



};

GCNState curGCNState;
GCNState oldGCNState;

static uint32 blendMap[] = {
	GX_BL_ZERO,	// actually invalid
	GX_BL_ZERO,
	GX_BL_ONE,
	GX_BL_SRCCLR,
	GX_BL_INVSRCCLR,
	GX_BL_SRCALPHA,
	GX_BL_INVSRCALPHA,
	GX_BL_DSTALPHA,
	GX_BL_INVDSTALPHA,
	GX_BL_DSTCLR,
	GX_BL_INVDSTCLR,
    //We don't have an equivalent of GL_SRC_ALPHA_SATURATE on Gamecube.
	GL_BL_ONE,
};
static int startGCN(){

    //This supposedly clears the z buffer. Might be useful to clear out random data
    GX_SetCopyClear({0,0,0,0xff},0x00ffffff);

    //Set the Viewport. The first 2 arguments are the XY of the top left corner of the screen.
	//Second 2 arguments are the width and height
	//Last 2 are values to use for near and far depth scale.
	GX_SetViewport(0,0,vidmode->fbWidth,vidmode->efbHeight,0,1);

	//Set the scale factor for the copy operation to the external framebuffer
	//This is needed because the internal framebuffer is smaller or equal to external, so a direct copy would leave a lot of empty space
	//We store it in
	xfbHeight = GX_SetDispCopyYScale(GX_GetYScaleFactor(vidmode->efbHeight,vidmode->xfbHeight));

	//We set the bounds where everything outside will be culled.
	GX_SetScissor(0,0,vidmode->fbWidth,vidmode->efbHeight);

	//We set what we're to copy from the embedded to external framebuffer
	GX_SetDispCopySrc(0,0,vidmode->fbWidth,vidmode->efbHeight);

	//Set the destination
	GX_SetDispCopyDST(vidmode->fbWidth,xfbHeight);

	//I don't know what this does, but we're supposed to do it.
	GX_SetCopyFilter(vidmode->aa,vidmode->sample_pattern,GX_TRUE,vidmode->vfilter);



    //Here, we tell the Flipper that we're using 32 bit floats for vertices, and that we're doing x y z.
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32);

	//Here, we're telling it we're using regular ass RGB with a max of 255
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8);

    //Of course, this can be changed by setting the attributes and will be if an attribute description is added
    
	
	

}
uint8 setVidMode(uint8 n){
    //if num is 0, that means we want the preferred mode
    if(n == 0){
        vidmode = VIDEO_GetPrefferedMode(NULL);
    //We don't have more than 0-45 video modes so we will return 0 if they ask for an index too high
    } else if(n >= 46){ return 0;} 
    //We set the corresponding video mode.
    else{
        vidmode = vidmodes[n-1];
    }
    //Actually tell the Flipper to set this video mode
    VIDEO_Configure(vidmode);
    //Reinitialize, as most things are set up with a specific video mode in mind.
    startGCN();
    return 1;
}


static int openGCN(EngineOpenParams *params)
{
	//Initialize the gamecube's video
	VIDEO_INIT();
	if(params->videoMode == 0){
        vidmode = VIDEO_GetPrefferedMode(NULL);
    } else if(params->videoMode >= 46){ return 0;} 
    else{
        vidmode = &vidmodes[n-1];
    }
    //create both framebuffers. Not gonna say I understand this
    xfb[0] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(vidmode));
    xfb[1] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(vidmode));
    
	//An area of memory needs to be allocated as the FIFO for the Flipper
	gcn_fifo = memalign(32,GX_FIFO_SIZE);
	//Ensure that memory is empty to prevent weird stuff
	memset(gcn_fifo,0,GX_FIFO_SIZE);

    //Configure the GC to the videomode set
    VIDEO_Configure(vidmode);
    //Set the next framebuffer
    VIDEO_SetNextFramebuffer(xfb[curr_fb]);

    //Wait for the setup to finish
    VIDEO_Flush();
    VIDEO_WaitVSync();

	//Initialize the Flipper
	GX_Init(gcn_fifo,GX_FIFO_SIZE);

	return 1;
	
	
}


static int finalizeGCN(void){
    
    return 1;
}

static int closeGCN(void){
    //Attempt to clear out the framebuffers.
    memset(xfb[0],0,sizeof(*xfb[0]));
    memset(xfb[1],0,sizeof(*xfb[1]));

    xfb[0] = NULL;
    xfb[1] = NULL;
    //obviously, after this, don't try to render stuff. It won't go well.
    return 1;
}

static int deviceSystemGCN(DeviceReq req, void *arg, int32 n){

    switch(rq){
        case DEVICEOPEN:
            openGCN((EngineOpenParams*)arg);

        case DEVICECLOSE:
            //I will close this device by clearing out the framebuffers
            closeGCN();
            return 1;

        case DEVICEINIT:
            return startGCN();

        case DEVICETERM:
            //Not Implemented
            return 1

        case DEVICEFINALIZE:
            finalizeGCN();
            return 1;

        case DEVICEGETNUMSUBSYSTEMS:
            
            return 1;
        case DEVICESETSUBSYSTEM:
            return 0
        
        case DEVICEGETSUBSSYSTEMINFO:
            ((SubSystemInfo*)arg)->name = "GameCube";
            return 1
        
        case DEVICEGETNUMVIDEOMODES:
            
            return 46;
        
        case DEVICESETVIDEOMODE:
            //If you are reading this. It is a bad idea to change the video mode.
            //mode 0 will always be the prefered mode
            return setVidMode(n);
            
        
        case DEVICEGETVIDEOMODEINFO:
            if(n >= 46){
                return 0;
            }
            VideoMode *rwmode = (VideoMode*)arg;
            rwmode->width = vidmodes[n]->fbwidth;
            rwmode->height = vidmodes[n]->efbheight;
            rwmode->depth = 1
            rwmode->flags = 0;
            return 1;

        
        case DEVICEGETMAXMULTISAMPLINGLEVELS:
        //Given the limited memory, multisampling isn't a thing here.
            return 1; 
        
        case DEVICEGETMULTISAMPLINGLEVELS:
            return 1;
        
        case DEVICESETMULTISAMPLINGLEVELS:
            return 0;
        
        default:
            assert(0 && "not implemented");
            return 0

        

        
        
    }
    return 1



}

void setFilterMode(uint32 stage, int32 filter, int32 maxAniso - 1)
{
    //Not doing textures yet

}

static void setRenderState(int32 state, void *pvalue)
{
    uint32 value = (uint32)(uintptr)pvalue;
    switch(state){
    case TEXTURERASTER:
		setRasterStage(0, (Raster*)pvalue);
		break;
	case TEXTUREADDRESS:
		setAddressU(0, value);
		setAddressV(0, value);
		break;
	case TEXTUREADDRESSU:
		setAddressU(0, value);
		break;
	case TEXTUREADDRESSV:
		setAddressV(0, value);
		break;
	case TEXTUREFILTER:
		setFilterMode(0, value);
		break;
	case VERTEXALPHA:
		if(rwStateCache.vertexAlpha != value){
            if(!rwStateCache.textureAlpha){
                //setAlphaBlend
                setAlphaBlend(value);
                //setAlphaTest
                setAlphaTest(value);
            }
            rwStateCache.vertexAlpha = value;
	    }
		break;
	case SRCBLEND:
		if(rwStateCache.srcblend != value){
			rwStateCache.srcblend = value;
			curGCNState.srcblend =  blendMap[rwStateCache.srcblend];
		}
		break;
	case DESTBLEND:
		if(rwStateCache.destblend != value){
			rwStateCache.destblend = value;
			curGCNState.destblend = blendMap[rwStateCache.destblend];
		}
		break;
	case ZTESTENABLE:
		if(rwStateCache.ztest != value){
            rwStateCache.ztest = value;
            if(rwStateCache.zwrite && !value){
                // If we still want to write, enable but set mode to always

                curGCNState.depthTest = GX_TRUE;
                curGCNState.depthFunc = GX_ALWAYS;
                
            }else{
                curGCNState.depthTest = rwStateCache.ztest ? GX_TRUE : GX_FALSE;
                curGCNState.depthFunc = GX_LEQUAL;
            }
        }
		break;
	case ZWRITEENABLE:
        if(rwStateCache.zwrite != value){
            rwStateCache.zwrite = value;
            if(enable && !rwStateCache.ztest){
                // Have to switch on ztest so writing can work on gamecube too!
                curGCNState.depthTest = GX_TRUE;
                curGCNState.depthFunc = GX_ALWAYS;
            }
            curGCNState.depthMask = rwStateCache.zwrite ? GX_ENABLE : GX_DISABLE;
        }
		break;
	case FOGENABLE:
		if(rwStateCache.fogEnable != value){
			rwStateCache.fogEnable = value;
			curGCNState.fogEnable = value ? GX_TRUE : GX_FALSE;
		}
		break;
	case FOGCOLOR:
		RGBA c;
		c.red = value;
		c.green = value>>8;
		c.blue = value>>16;
		c.alpha = value>>24;
		convColor(&curGCNSTATE.fogColor, &c);
		break;
	case CULLMODE:
		if(rwStateCache.cullmode != value){
			rwStateCache.cullmode = value;
			switch(rwStateCache.cullmode){
                case CULLNONE:
                    curGCNState.cullFace = GX_CULL_NONE;
                case CULLBACK:
                    curGCNState.cullFace = GX_CULL_BACK;
                case CULLFRONT:
                    curGCNState.cullFace = GX_CULL_FRONT;
            }
		}
		break;

	case ALPHATESTFUNC:
		if(rwStateCache.alphaFunc != value){
            rwStateCache.alphaFunc = value;
            alphaFunc = rwStateCache.alphaTestEnable ? rwStateCache.alphaFunc : ALPHAALWAYS;
        }
	}
		break;
	case ALPHATESTREF:
		if(alphaRef != value/255.0f){
			alphaRef = value/255.0f;
			
		}
		break;
	case GSALPHATEST:
		rwStateCache.gsalpha = value;
		break;
	case GSALPHATESTREF:
		rwStateCache.gsalpharef = value;


}

void flushGCNRenderState(void){
    if(oldGCNState.blendEnable != curGCNState.blendEnable ||
       oldGCNState.srcblend != curGCNState.srcblend ||
       oldGCNState.destblend != cur.destblend){

        oldGCNState.blendEnable = curGCNState.blendEnable;
        oldGCNState.srcblend = curGCNState.srcblend;
        oldGCNState.destblend = curGCNState.destblend;
        
        GX_SetBlendMode(oldGCNState.blendEnable,oldGCNState.srcblend,oldGCNState.destblend);
    }

    if(oldGCNState.depthMask != curGCNState.depthMask || 
       oldGCNState.depthTest != curGCNState.depthTest ||
       oldGCNState.depthFunc != curGCNState.depthFunc){

        oldGCNState.depthMask = curGCNState.depthMask
        oldGCNState.depthTest = curGCNState.depthTest;
        oldGCNState.depthFunc = curGCNState.depthFunc;

        if(oldGCNState.depthTest == GX_FALSE){
            GX_SetZMode(oldGCNState.depthMask,GX_ALWAYS,GX_TRUE);
        } else{
            GX_SetZMode(oldGCNState.depthMask,oldGCNState.depthFunc);
        }
        

    }

    if(oldGCNState.cullEnable != curGCNState.cullEnable || 
       oldGCNState.cullFace != curGCNState.cullFace){

        if(!oldGCNState.cullEnable){
            GX_SetCullMode(GX_CULL_NONE);
        } else{
            GX_SetCullMode(oldGCNState.cullFace)
        }

    }

}

void setAlphaBlend(bool32 enable){
    if(rwStateCache.blendEnable != enable){
        rwStateCache.blendEnable = enable;
        curGCNState.blendEnable = enable ? GX_BM_BLEND : GX_BM_NONE;
    }
}

void setAlphaTest(bool32 enable){
    if(rwStateCache.alphaTestEnable != enable){
        rwStateCache.alphaTestEnable = enable;
        alphaFunc = rwStateCache.alphaTestEnable ? rwStateCache.alphaFunc : ALPHAALWAYS;

    }
}

}


}
