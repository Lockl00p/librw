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

static void*
driverOpen(void *o, int32, int32)
{
	
	engine->driver[PLATFORM_GCN]->defaultPipeline = makeDefaultPipeline();

	engine->driver[PLATFORM_GCN]->rasterNativeOffset = nativeRasterOffset;
	engine->driver[PLATFORM_GCN]->rasterCreate       = rasterCreate;
	engine->driver[PLATFORM_GCN]->rasterLock         = rasterLock;
	engine->driver[PLATFORM_GCN]->rasterUnlock       = rasterUnlock;
	engine->driver[PLATFORM_GCN]->rasterNumLevels    = rasterNumLevels;
	engine->driver[PLATFORM_GCN]->imageFindRasterFormat = imageFindRasterFormat;
	engine->driver[PLATFORM_GCN]->rasterFromImage    = rasterFromImage;
	engine->driver[PLATFORM_GCN]->rasterToImage      = rasterToImage;
	
	return o;
}

static void*
driverClose(void *o, int32, int32)
{
	return o;
}

void
registerPlatformPlugins(void)
{
	Driver::registerPlugin(PLATFORM_GCN, 0, PLATFORM_GCN,
	                       driverOpen, driverClose);
	
}

static InstanceDataHeader*
instanceMesh(rw::ObjPipeline *rwpipe, Geometry *geo){

}

void ObjPipeline::init(void){
	this->rw::ObjPipeline::init(PLATFORM_GCN);
	this->impl.instance = gcn::instance;
	this->impl.uninstance = gcn::uninstance;
	this->impl.render = gcn::render;
}

ObjPipeline*
ObjPipeline::create(void)
{
	ObjPipeline *pipe = rwNewT(ObjPipeline, 1, MEMDUR_GLOBAL);
	pipe->init();
	return pipe;
}

}
}
