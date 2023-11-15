#include <lwmeshedt.h>
#include <lwserver.h>
#include <lwxpanel.h>
#include <lwdyna.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <lwmeshes.h>
#include <lwtxtr.h>
#include <lwmodtool.h>
#include <lwtxtred.h>
#include <lwmath.h>
#include <tbb/tbb.h>

typedef struct st_pointCache
{
    LWDVector oldPos;
    LWDVector newPos;
    LWPntID id;
    
    int pointContained;
    double pointDist;
    int pointHandleIndex;
    
    int linearContained;
    double linearDist;
    double linearT;
    double linearRadius;
    int linearHandleIndex;
    LWDVector linearClosestPoint;
    
    int splineContained;
    double splineDist;
    double splineT;
    double splineRadius;
    int splineHandleIndex;
    LWDVector splineClosestPoint;
    
    int symmContained;
    int symmIndex;
} PointCache;


typedef struct st_SplineTool
{
	int numHand;
	int selected;
	int currHand;
	double r0;
	int active,update, dirty;
	double radius[20];
	LWDVector handPos[20];
	double strength, handleSize, EaseIn, EaseOut;
	int inter, child_comp, recorded, fofftype;
    
    int justRecorded;
    int justSymmetry;
    
    PointCache *cache;
    
} SplineTool;

typedef struct st_LWPoint
{
   float x;
   float y;
   float z;
} Point3D;


class VDeformerData
{
public:
    
	struct ThreadData
	{
        SplineTool *tool;
        int numcase;
	};
    
    VDeformerData(){};
    
	VDeformerData(SplineTool *tool, int numcase)
	{
        m_data.tool = tool;
        m_data.numcase = numcase;
	}
    
	void operator()( const tbb::blocked_range<size_t>& r ) const;
    
private:
    
    struct ThreadData m_data;
};

class CheckSymmetryData
{
public:
    
	struct ThreadData
	{
        SplineTool *tool;
	};
    
    CheckSymmetryData(){};
    
	CheckSymmetryData(SplineTool *tool)
	{
        m_data.tool = tool;
	}
    
	void operator()( const tbb::blocked_range<size_t>& r ) const;
    
private:
    
    struct ThreadData m_data;
};

class ApplySymmetryData
{
public:
    
	struct ThreadData
	{
        SplineTool *tool;
	};
    
    ApplySymmetryData(){};
    
	ApplySymmetryData(SplineTool *tool)
	{
        m_data.tool = tool;
	}
    
	void operator()( const tbb::blocked_range<size_t>& r ) const;
    
private:
    
    struct ThreadData m_data;
};

extern FILE *file;

extern LWStateQueryFuncs *query;

int get_xpanf( GlobalFunc *global );

extern int Spline(MeshEditOp *op, SplineTool *, FILE *);

LWXPanelID Spline_Panel( SplineTool *tool );

void DisableControl(void);

void RecordPosition(SplineTool *tool);

void backToRestSelected(SplineTool *tool);

void backToRestAll(SplineTool *tool);

/* casting typedef function */
typedef void*(*get_function)(void*, unsigned int);
typedef LWXPRefreshCode(*set_function)(void*, unsigned int, void*);

typedef void(*done_function)(void*);
typedef int(*count_function)(void*, LWToolEvent*);
typedef void (*draw_function)(void*, LWWireDrawAccess*);
typedef int(*dirty_function)(void*);
typedef void(*up_function)(void*, LWToolEvent*);
typedef const char* (*help_function)(void*, LWToolEvent*);
typedef void (*move_function)(void*, LWToolEvent*);
typedef int (*down_function)(void*, LWToolEvent*);
typedef void* (*panel_function)(void*);
typedef void (*event_function)(void*, int);
typedef const char*(*build_function)(void*, MeshEditOp*);
typedef int(*test_function)(void*);
typedef void(*end_function)(void*, int);
typedef int (*activate_function)(int, void* (*)(const char*, int), void*, void*);
typedef EDError (*pntScan_function)(void*, const EDPointInfo*);
typedef EDError (*fastPntScan_function)(void*, LWPntID);
