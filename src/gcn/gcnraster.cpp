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


namespace rw{
namespace gcn{





static Raster* rasterCreateTexture(Raster *raster){
    
    if(raster->format & (Raster::PAL4 | Raster::PAL8)){
		RWERROR((ERR_NOTEXTURE));
		return nil;
	}

    gcnRaster *raster = 
}


Raster* rasterCreate(Raster *raster)
{
	gcnRaster *platras = GETGCNRASTEREXT(raster);

	Raster *rst = raster;

	switch(raster->type){
		case Raster::NORMAL:
		case Raster::TEXTURE:
			ret = rasterCreateTexture(raster);
			break;
		case Raster::CAMERATEXTURE:
			ret = rasterCreateCameraTexture(raster);
			break;
		case Raster::ZBUFFER:
			ret = rasterCreateZbuffer(raster);
			break;
		case Raster::CAMERA:
			ret = rasterCreateCamera(raster);
			break;

		default:
			RWERROR((ERR_INVRASTER));
			return nil;
	}

	
}
static void*
createNativeRaster(void *object,int32 offset){
	gcnRaster *ras = PLUGINOFFSET(gcnRaster, object, offset);
	ras->texid = GX_TEXMAP_NULL;
	return object;


}



void registerNativeRaster(void)
{
	nativeRasterOffset = Raster::registerPlugin(sizeof(gcnRaster),
												ID_RASTERGCN,
												createNativeRaster,
												destroyNativeRaster,
												copyNativeRaster);
}

}
}