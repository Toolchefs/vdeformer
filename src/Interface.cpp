


#include "spline.h"
#include <stdlib.h>
#include <string.h>
#include <lwmodtool.h>
#include <lwvparm.h>

#define GRAD_PARM_NUM_PARAMDESC 2

static int StEnable;

int enable;

LWXPanelFuncs *xpanf;
LWXPanelID panel;
SplineTool *tl;
static SplineTool staticTool = {
	0,
	0,
	0,
	0.0,
	0, 0, 0,
	0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 
	0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
	1.0,
	0.2,
	0.0,
	1.0,
	0,
	1,
	0,
	1
};

int EnableRcpos, DisRcpos, EnableEIEO;

enum { ID_SSEL = 0x8001, ID_NRADIUS, ID_DELH, ID_HANDLE_SIZE, ID_BRADIUS, ID_POS, ID_FALLOFF, ID_MIRROR, ID_STORE, ID_LOAD, ID_REST, ID_APPLY, ID_FORGET, ID_INTER, ID_CHILD_COMP, ID_STARTENABLE, ID_ENDIST, ID_RCPOS, ID_NRCPOS, ID_FOFFTYPE, ID_EI, ID_EO, ID_EIEO, ID_BTOREST, ID_THUMB, ID_THUMGROUP};

int get_xpanf( GlobalFunc *global )
{
    xpanf = static_cast<LWXPanelFuncs*>(global(LWXPANELFUNCS_GLOBAL, GFUSE_TRANSIENT));
    return (xpanf != NULL);
}

void DisableControl(void)
{
	enable = 0;
	EnableRcpos = 1;
	DisRcpos = 0;	
}

void MakeHintMAX(int num)
{
	LWXPanelHint hints[] = {XpMAX(ID_SSEL, num),XpEND};
	xpanf->hint( panel, 0, hints );
}

static void *Get(SplineTool *tool, unsigned long vid)
{

	switch (vid)
	{
		case ID_SSEL:		
			MakeHintMAX(tool->numHand - 1);
			return &tool->currHand;

		case ID_NRADIUS:	return &tool->radius[tool->currHand];
		case ID_POS:		return &tool->handPos[tool->currHand];
		case ID_FALLOFF:	return &tool->strength;
		case ID_HANDLE_SIZE: return &tool->handleSize;
		case ID_STARTENABLE:	return &tool->active;
		case ID_CHILD_COMP:		return &tool->child_comp;
		case ID_INTER:		return &tool->inter;
		case ID_RCPOS:		return &EnableRcpos;
		case ID_NRCPOS:		return &DisRcpos;
		case ID_EIEO:		return &EnableEIEO;
		case ID_FOFFTYPE:	return &tool->fofftype;
		case ID_EI:			return &tool->EaseIn;
		case ID_EO:			return &tool->EaseOut;
		default:			return NULL;			
	}
}

static int Set( SplineTool *tool, unsigned long vid, void *value )
{
//   const char *a;
   double *d, temp;
   int i;

   switch ( vid )
   {
      case ID_SSEL:
         i = *(( int * ) value);
		 if (i < 0) i = 0;
		 if (i > tool->numHand - 1) i = tool->numHand - 1;
		 tool->currHand = i;
		 break;

	  case ID_NRADIUS:
         d = ( double * ) value;
		 tool->radius[tool->currHand] = fabs(*d);
		 break;

      case ID_POS:
         d = ( double * ) value;
		 tool->handPos[tool->currHand][ 0 ] = d[ 0 ];
		 tool->handPos[tool->currHand][ 1 ] = d[ 1 ];
		 tool->handPos[tool->currHand][ 2 ] = d[ 2 ];
         break;
		 
	 case ID_FALLOFF:
		d = ( double * ) value;
		tool->strength = fabs(*d);
		break;

	case ID_HANDLE_SIZE:
		d = ( double * ) value;
		tool->handleSize = fabs(*d);
		break;
	
	case ID_CHILD_COMP:
		i = *(( int * ) value);
		tool->child_comp = i;
		break;

	case ID_INTER:
		i = *(( int * ) value);
		tool->inter = i;

		break;

	case ID_FOFFTYPE:	
		i = *(( int * ) value);
		tool->fofftype = i;

        EnableEIEO = i == 1;
		break;

	case ID_EI:	
		temp = *(( double * ) value);
		
		if (temp > 1)
			tool->EaseIn = 1;
		else
		if (temp < 0)
			tool->EaseIn = 0;
		else
			tool->EaseIn = temp;
		break;

	case ID_EO:	
		temp = *(( double * ) value);
		
		if (temp > 1)
			tool->EaseOut = 1;
		else
		if (temp < 0)
			tool->EaseOut = 0.001;
		else
			tool->EaseOut = temp;
		break;

	 default:
         return LWXPRC_NONE;
   }

	tool->update = LWT_TEST_UPDATE;
	tool->dirty = 1;

	xpanf->viewRefresh(panel);
	return LWXPRC_DRAW;
}

void DeleteHandle (LWXPanelID pan, int cid)
{
	int i,start;

	if (tl->currHand != 0)
		start = tl->currHand;
	else
		start = 0;
		
	for (i = start; i < tl->numHand ;i++)
	{
		tl->handPos[i][0] = tl->handPos[i + 1][0];
		tl->handPos[i][1] = tl->handPos[i + 1][1];
		tl->handPos[i][2] = tl->handPos[i + 1][2];

		tl->radius[i] = tl->radius[i + 1];
	}

	tl->numHand--;

	if (tl->currHand != 0)
		tl->currHand--;
	else
		tl->currHand = 0;
	
	tl->dirty = 1;
	tl->update = LWT_TEST_UPDATE;
	xpanf->viewRefresh(panel);
}

void SetRadius(LWXPanelID pan, int cid)
{
	int i;
	for (i = 0; i < tl->numHand; i++)
		tl->radius[i] = tl->radius[tl->currHand];

	tl->dirty = 1;
	tl->update = LWT_TEST_UPDATE;
}

void MirrorClick(LWXPanelID pan, int cid)
{
	int i = 0;
	for (i = 0; i < tl->numHand; i++)
	{
		tl->handPos[i][0] = -tl->handPos[i][0];
	}
	tl->dirty = 1;
	tl->update = LWT_TEST_UPDATE;
}

void StoreClick(LWXPanelID pan, int cid)
{
	memcpy(&staticTool, tl, sizeof(SplineTool));
	memcpy(&StEnable, &enable, sizeof(int));
}

void LoadClick(LWXPanelID pan, int cid)
{
	if (&staticTool)
	{
		memcpy(tl, &staticTool, sizeof(SplineTool));
		memcpy(&enable, &StEnable, sizeof(int));
		
		tl->dirty = 1;
		tl->update = LWT_TEST_UPDATE;
	}
}

void RecPosition(LWXPanelID pan, int cid)
{
	RecordPosition(tl);
	EnableRcpos = 0;
	DisRcpos = 1;

	tl->recorded = 1;
    tl->justRecorded = 1;
    
    if (tl->cache)
        free(tl->cache);
    tl->cache = NULL;

	xpanf->viewRefresh(panel);
}

void Apply(LWXPanelID pan, int cid)
{
	EnableRcpos = 1;
	DisRcpos = 0;
	
	tl->update = LWT_TEST_CLONE;

	backToRestAll(tl);
    
    if (tl->cache)
        free(tl->cache);
    tl->cache = NULL;


	tl->recorded = 0;
    tl->justRecorded = 0;
	tl->dirty = 1;

	xpanf->viewRefresh(panel);
}

void Forget(LWXPanelID pan, int cid)
{
	EnableRcpos = 1;
	DisRcpos = 0;
	
	tl->update = LWT_TEST_REJECT;

	backToRestAll(tl);
    
    if (tl->cache)
        free(tl->cache);
    tl->cache = NULL;
	
	tl->recorded = 0;
    tl->justRecorded = 0;
	tl->dirty = 1;

	xpanf->viewRefresh(panel);
}

void backToRest(LWXPanelID pan, int cid)
{
	backToRestSelected(tl);
	tl->dirty = 1;
	tl->update = LWT_TEST_UPDATE;
}

void draw (LWXPanelID pan, unsigned long cid, LWXPDrAreaID *reg, int w, int h)
{
	float t, k1, k2, f, factor;
	int x, y, oldx, oldy;
	xpanf->drawf->drawRGBBox(reg, 0, 0, 0, 0, 0, w, h);
	switch (tl->fofftype) 
	{
	case 0:
		xpanf->drawf->drawLine(reg, 2, 0, h, w, 0);
		break;

	case 1:

		oldx = 0;
		oldy = h;
		for (t = 0.05; t <= 1.0; t += 0.05)
		{		
			k1 = tl->EaseIn / 2;
			
			if (tl->EaseOut > 0)
				k2 = 1 - tl->EaseOut / 2;
			else
				k2 = -1 + fabs(tl->EaseOut / 2);

			f = k1 * (2 / PI) + k2 - k1 + (1 - k2) * 2 / PI;
			
			if ( t < k1)
				factor = (k1 * (2 / PI) * (sin((t / k1) * (PI / 2) - PI / 2) + 1)) / f;
			else
			if (t > k2)
				factor = (k1 / (PI / 2) + k2 - k1 + ( (1 - k2) * (2 / PI)) * sin(( t - k2 ) / (1 - k2) * (PI / 2))) / f;
			else
				factor = (k1 / (PI / 2) + t - k1) / f;	
		
			x = t * w;
			y = h - factor * h;

			xpanf->drawf->drawLine(reg, 2, x, y, oldx, oldy);
			oldx = x;
			oldy = y;
		}

		xpanf->drawf->drawLine(reg, 2, x, y, w, 0);

		break;
	}	
}

LWXPanelID Spline_Panel( SplineTool *tool )
{
	static char *hint[] = {"None", "Linear", "Spline", NULL};
	static char *tfoff[] = {"Linear", "EaseInEaseOut", NULL};
		
	static LWXPanelControl ctl[] = {
      { ID_SSEL,	"Selected",  "iSliderText"  },
      { ID_NRADIUS,	"Radius",    "float"  },
	  { ID_DELH,	"Delete Selected Handle", "vButton"},
	  { ID_BRADIUS,	"All Radius as Selected", "vButton"},
	  {	ID_MIRROR,	"Mirror", "vButton"},
	  { ID_HANDLE_SIZE,	"Handle radius",    "float"  },
	  { ID_POS,		"Position ",	"float3"},
	  { ID_CHILD_COMP, "Child Compensation", "iBoolean"},
	  { ID_REST,	"Record Position", "vButton"},
	  { ID_APPLY,	"Apply", "vButton"},
	  { ID_FORGET,	"Forget", "vButton"},
	  { ID_BTOREST,	"Back To Rest Position", "vButton"},
	  { ID_FALLOFF, "Strength", "percent"},
	  { ID_FOFFTYPE, "Falloff Type", "iPopChoice"},
      { ID_EI, "Ease In", "percent"},
	  { ID_EO, "Ease Out", "percent"},
	  { ID_THUMB,	"Drawing", "dThumbnail"},
	  { ID_INTER,	"Interpolation", "iChoice"},
	  {	ID_STORE,	"Store settings", "vButton"},
	  { ID_LOAD,	"Restore settings", "vButton"},
	  { 0 }
	};

	static LWXPanelDataDesc cdata[] = {
      { ID_SSEL,	"Selected",  "integer"  },
      { ID_NRADIUS,	"Radius",    "float"  },
	  { ID_POS,		"Position",	 "float3"},
	  { ID_FALLOFF, "Strength", "float"},
	  { ID_FOFFTYPE, "Falloff Type", "integer"},
	  { ID_EI, "Ease In", "float"},
	  { ID_EO, "Ease Out", "float"},
	  { ID_EIEO, "Falloff Type", "integer"},
	  { ID_HANDLE_SIZE,	"Handle radius",    "float"  },
	  { ID_STARTENABLE, "StartEnable", "integer"},
	  { ID_INTER,	"Interpolation", "integer"},
	  { ID_CHILD_COMP, "Child Compensation", "integer"},
	  { ID_ENDIST, "EnDist", "integer"},
	  { ID_RCPOS, "EnRec", "integer"},
	  { ID_NRCPOS, "EnRec", "integer"},
	  { 0 }
	};

	LWXPanelHint hints[] = {
      XpLABEL( 0, "tcVDeformer" ),
      XpMIN( ID_NRADIUS, 0),
	  XpMIN( ID_SSEL, 0),
	  XpDIVADD( ID_HANDLE_SIZE),
	  XpDIVADD( ID_CHILD_COMP),
	  XpDIVADD( ID_POS ),
	  XpDIVADD( ID_BTOREST ),
	  XpDIVADD(ID_THUMB),
	  XpDIVADD( ID_INTER ),
	  XpSTRLIST( ID_INTER, hint),
	  XpSTRLIST( ID_FOFFTYPE, tfoff),
	  XpHARDMINMAX( ID_FALLOFF, 0, 1),
	  XpCTRLCFG( ID_THUMB, THUM_SML | THUM_FULL),
	  XpENABLE_ (ID_NRCPOS), XpH(ID_APPLY), XpH(ID_FORGET), XpH(ID_BTOREST), XpEND,
	  XpENABLE_ (ID_EIEO) , XpH(ID_EI), XpH(ID_EO), XpEND,
	  XpENABLE_ (ID_RCPOS), XpH(ID_REST), XpH(ID_STORE), XpH(ID_LOAD), XpH(ID_MIRROR), XpH(ID_DELH), XpEND,
	  XpENABLEMSG_(ID_STARTENABLE, "Click on a viewport to activate \"VDeformer\"!"), XpH(ID_SSEL), XpH(ID_NRADIUS),XpH(ID_DELH), XpH(ID_BRADIUS), XpH(ID_POS), XpH(ID_FALLOFF), XpH(ID_MIRROR), XpH(ID_STORE), XpH(ID_LOAD), XpH(ID_HANDLE_SIZE), XpH(ID_CHILD_COMP), XpH(ID_REST), XpH(ID_APPLY), XpH(ID_FORGET), XpH(ID_INTER), XpH(ID_FOFFTYPE), XpEND,
	  XpBUTNOTIFY( ID_DELH, DeleteHandle ),
	  XpBUTNOTIFY( ID_BRADIUS, SetRadius ),																																																																									
	  XpBUTNOTIFY( ID_REST, RecPosition ),
	  XpBUTNOTIFY( ID_APPLY, Apply ),
	  XpBUTNOTIFY( ID_FORGET, Forget ),
	  XpBUTNOTIFY( ID_MIRROR, MirrorClick),
	  XpBUTNOTIFY( ID_STORE, StoreClick),
	  XpBUTNOTIFY( ID_LOAD, LoadClick),
	  XpBUTNOTIFY( ID_BTOREST, backToRest),
	  XpDRAWCBFUNC( ID_THUMB, draw ) ,
	  XpEND
	};

	EnableRcpos = 1;
	EnableEIEO = DisRcpos = 0;
	tl = tool;
	
	panel = xpanf->create( LWXP_VIEW, ctl );
 	if ( !panel ) return NULL;

 	xpanf->describe( panel, cdata, (get_function)Get, (set_function)Set );
 	xpanf->hint( panel, 0, hints );

 	return panel;
}