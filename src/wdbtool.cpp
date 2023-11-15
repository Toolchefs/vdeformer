
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <lwserver.h>
#include <lwmodtool.h>
#include "spline.h"

/* this is where the trace is written */

#define TR_FILENAME  "D:\\Scripting\\CPlugin\\DF_SplineDeformer\\trace.txt"

//OldHandlePosition
LWDVector oldHpos, oldAllPos[19];
LWStateQueryFuncs *query;

FILE *fp;

static SplineTool SSpline = {
	0,
	0,
	0,
	0.0,
	0, 0, 0,
	0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 
	0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
	1.0,
	0.2,
	0.0,
	1.0,
	0,
	1,
	0,
	0,
    0,
    0,
    0
};

#define TR_DONE      0
#define TR_HELP      1
#define TR_COUNT     2
#define TR_HANDLE    3
#define TR_ADJUST    4
#define TR_START     5
#define TR_DRAW      6
#define TR_DIRTY     7
#define TR_EVENT     8
#define TR_PANEL     9
#define TR_BUILD     10
#define TR_TEST      11
#define TR_END       12
#define TR_MDOWN     13
#define TR_MMOVE     14
#define TR_MUP       15
#define TR_PANSET    16
#define TR_PANGET    17


SplineTool *new_Spline(void)
{
	SplineTool *tool;

	tool = new SplineTool;
	if (tool)
		memcpy(tool, &SSpline, sizeof(SplineTool));
    
    tool->cache = NULL;

	return tool;
}

static int Spline_Count(SplineTool *tool,LWToolEvent *event)
{
	return tool->active ? tool->numHand: 0;
}


static LWError Spline_Build (SplineTool	*tool,MeshEditOp *op)
{
	EDError	err;
	//trace( TR_BUILD, tool, NULL );
	err = Spline(op, tool, fp);
	tool->update = LWT_TEST_NOTHING;

	return (err ? "Failed" : NULL);
}

static int Spline_Test (SplineTool *tool)
{
	//trace( TR_TEST, tool, NULL );
	return tool->update;
}


static void Spline_End (SplineTool *tool,int keep)
{
	tool->update = LWT_TEST_NOTHING;
}


static const char *Spline_Help (SplineTool *tool,LWToolEvent *event)
{
	if (tool->numHand == 0)
		return "Click on a viewport to activate the tool.";
    
	return "Left mouse Button: add/adjust handle - Right mouse Button: change handle radius";
}

static int Spline_Down (SplineTool *tool,LWToolEvent *event)
{
	double tempX, tempY, tempZ, x, y, z, b, c, discr;
	int i, j;
	LWDVector vec;

	if (tool->numHand == 0 || !tool->active)
	{
		tool->active = 1;	
	}
	//trace( TR_MDOWN, tool, NULL );
	if (event->flags & LWTOOLF_ALT_BUTTON) 
	{
		tool->r0 = tool->radius[tool->currHand];
		tool->dirty = 1;
	} 
	else
	{
		tempX = event->posRaw[0];
		tempY = event->posRaw[1];
		tempZ = event->posRaw[2];
		
		for (i = 0; i < tool->numHand; i++)
		{
			x = tempX - tool->handPos[i][0];
			y = tempY - tool->handPos[i][1];
			z = tempZ - tool->handPos[i][2];

			if (abs(event->axis[1]) == 1.0)
			{
				if (x * x + z * z <= pow(tool->handleSize,2))
				{
					tool->selected = 1;
					tool->currHand = i;
					break;
				}
				tempY = 0.0;
			}
			else
			if (abs(event->axis[0]) == 1.0)
			{
				if (y * y + z * z <= pow(tool->handleSize,2))
				{
					tool->selected = 1;
					tool->currHand = i;
					break;
				}
				tempX = 0.0;
			}
			else
			if (abs(event->axis[2]) == 1.0)
			{
				if (y * y + x * x <= pow(tool->handleSize,2))
				{
					tool->selected = 1;
					tool->currHand = i;
					break;
				}
				tempZ = 0.0;
			}
			else
			{
			vec[0] = x;
			vec[1] = y;
			vec[2] = z;
			b = vec[ 0 ] * event->axis[ 0 ] + vec[ 1 ] * event->axis[ 1 ] + vec[ 2 ] * event->axis[ 2 ];
			c = vec[0] * vec[0] + vec[1] * vec[1] + vec[2] * vec[2] - pow(tool->handleSize,2); 

			discr = b * b -c;
			if (discr >= 0)
			{
				tool->selected = 1;
				tool->currHand = i;
				break;
			}
			}
		}
		
		if (tool->selected == 0 && !tool->recorded)
		{
			tool->numHand++;
			if (tool->numHand > 20)
			{
			tool->numHand = 20;
			}
			else
			{		
			tool->currHand = tool->numHand - 1;
			tool->handPos[tool->currHand][0] = tempX;
			tool->handPos[tool->currHand][1] = tempY;
			tool->handPos[tool->currHand][2] = tempZ;
			if (tool->currHand > 0)
				tool->radius[tool->currHand] = tool->radius[tool->currHand - 1];
			else
                tool->radius[tool->currHand] = 1.0;
			tool->dirty = 1;
			tool->selected = 1;
			}
		}
		
		if(!tool->child_comp)
		{
			oldHpos[0] = tool->handPos[tool->currHand][0];
			oldHpos[1] = tool->handPos[tool->currHand][1];
			oldHpos[2] = tool->handPos[tool->currHand][2];
			for (j = tool->currHand + 1; j < tool->numHand; j++)
			{
				oldAllPos[j][0] = tool->handPos[j][0];
				oldAllPos[j][1] = tool->handPos[j][1];
				oldAllPos[j][2] = tool->handPos[j][2];
			}
		}

	}
	tool->update = LWT_TEST_UPDATE;
	return 1;
}

static void Spline_Move (SplineTool	*tool,LWToolEvent *event)
{
	double den, t, d;
	LWDVector vec, offset;
	int i;
	//trace( TR_MMOVE, tool, NULL );
	if (event->flags & LWTOOLF_ALT_BUTTON) 
	{
		tool->radius[tool->currHand] = fabs(tool->r0 + event->dx * 0.01);
        //we need to recalculate to evaluate the new radius
        tool->justRecorded = 1;
	}
	else
	{
		if (tool->selected == 1)
		{
			if (abs(event->axis[1]) == 1.0)
			{
				tool->handPos[tool->currHand][0] = event->posRaw[0];
				tool->handPos[tool->currHand][2] = event->posRaw[2];
			}
			else
			if (abs(event->axis[0]) == 1.0)
			{
				tool->handPos[tool->currHand][1] = event->posRaw[1];
				tool->handPos[tool->currHand][2] = event->posRaw[2];
			}
			else
			if (abs(event->axis[2]) == 1.0)
			{
				tool->handPos[tool->currHand][1] = event->posRaw[1];
				tool->handPos[tool->currHand][0] = event->posRaw[0];
			}
			else
			{
				vec[0] = event->axis[0];
				vec[1] = event->axis[1];
				vec[2] = event->axis[2];
				den = vec[0] * vec[0] + vec[1] * vec[1] + vec[2] * vec[2];

				d = -(vec[0] * tool->handPos[tool->currHand][0] + vec[1] * tool->handPos[tool->currHand][1] + vec[2] * tool->handPos[tool->currHand][2]);
				
				if (den < 0.1)
				{
					if (abs(event->posRaw[0] * vec[0] + event->posRaw[1] * vec[1] + event->posRaw[2] * vec[2] + d) < 0.1)
					{
						tool->handPos[tool->currHand][0] = event->posRaw[0];
						tool->handPos[tool->currHand][1] = event->posRaw[1];
						tool->handPos[tool->currHand][2] = event->posRaw[2];
					}
				}
				else
				{
					t = -(event->posRaw[0] * vec[0] + event->posRaw[1] * vec[1] + event->posRaw[2] * vec[2] + d);
					t = t / den;
					tool->handPos[tool->currHand][0] = event->posRaw[0] + t * vec[0];
					tool->handPos[tool->currHand][1] = event->posRaw[1] + t * vec[1];
					tool->handPos[tool->currHand][2] = event->posRaw[2] + t * vec[2];
				}

			}

			if (!tool->child_comp)
			{
				offset[0] = tool->handPos[tool->currHand][0] - oldHpos[0];
				offset[1] = tool->handPos[tool->currHand][1] - oldHpos[1];
				offset[2] = tool->handPos[tool->currHand][2] - oldHpos[2];
				for (i = tool->currHand + 1; i < tool->numHand; i++)
				{
					tool->handPos[i][0] = oldAllPos[i][0] + offset[0];
					tool->handPos[i][1] = oldAllPos[i][1] + offset[1];
					tool->handPos[i][2] = oldAllPos[i][2] + offset[2];
				}
			}
		}
	}

	tool->dirty = 1;

	tool->update = LWT_TEST_UPDATE;
}


static void Spline_Up (SplineTool	*tool,LWToolEvent *event)
{
	tool->selected = 0;
	tool->update = LWT_TEST_UPDATE;
}


static void Spline_Event (SplineTool *tool,int code)
{
	switch (code) {
	    case LWT_EVENT_DROP:		
			memcpy(tool, &SSpline, sizeof(SplineTool));
			//tool->numHand = 0;
			//tool->active = 0;
			tool->dirty = 1;
			tool->update = LWT_TEST_REJECT;
			DisableControl();
			break;

	    case LWT_EVENT_RESET:
			memcpy(tool, &SSpline, sizeof(SplineTool));
			//tool->numHand = 0;
			//tool->active = 0;
			tool->dirty = 1;
			tool->update = LWT_TEST_REJECT;
			DisableControl();
		break;

	    case LWT_EVENT_ACTIVATE:
			tool->active = 0;
			tool->dirty = 0;
		break;
	}
}


static void Spline_Draw(SplineTool *tool, LWWireDrawAccess *draw)
{
	int i;
	double t;
	LWFVector vec, pt0, pt1, pt2, pt3;
	if (!tool->active) return;
	//trace(TR_DRAW, tool, NULL);
	for (i=0; i < tool->numHand; i++)
	{
	vec[0] = (float)tool->handPos[i][0];
	vec[1] = (float)tool->handPos[i][1];
	vec[2] = (float)tool->handPos[i][2];
	draw->moveTo(draw->data, vec, LWWIRE_SOLID);
	draw->circle(draw->data, tool->handleSize, LWWIRE_ABSOLUTE);
	draw->circle(draw->data, tool->radius[i], LWWIRE_ABSOLUTE);
	}

	if (tool->inter == 1)
		if (tool->numHand > 1)
		{
			vec[0] = (float)tool->handPos[0][0];
			vec[1] = (float)tool->handPos[0][1];
			vec[2] = (float)tool->handPos[0][2];	
			draw->moveTo(draw->data, vec, LWWIRE_SOLID);
			for (i = 1; i < tool->numHand; i++)
			{
				vec[0] = (float)tool->handPos[i][0];
				vec[1] = (float)tool->handPos[i][1];
				vec[2] = (float)tool->handPos[i][2];				
				draw->lineTo(draw->data, vec, LWWIRE_ABSOLUTE);
			}
		}
    
	if (tool->inter == 2)
		if (tool->numHand > 1)
		{
			pt0[0] = (float)tool->handPos[0][0];
			pt0[1] = (float)tool->handPos[0][1];
			pt0[2] = (float)tool->handPos[0][2];
			draw->moveTo(draw->data, pt0, LWWIRE_SOLID);
			
			for (i = 0; i < tool->numHand - 1; i++)
			{
			if (i - 1 > 0)
			{
				pt0[0] = (float)tool->handPos[i - 1][0];
				pt0[1] = (float)tool->handPos[i - 1][1];
				pt0[2] = (float)tool->handPos[i - 1][2];
			}
			else
			{
				pt0[0] = (float)tool->handPos[0][0];
				pt0[1] = (float)tool->handPos[0][1];
				pt0[2] = (float)tool->handPos[0][2];
			}

			pt1[0] = (float)tool->handPos[i][0];
			pt1[1] = (float)tool->handPos[i][1];
			pt1[2] = (float)tool->handPos[i][2];

			pt2[0] = (float)tool->handPos[i + 1][0];
			pt2[1] = (float)tool->handPos[i + 1][1];
			pt2[2] = (float)tool->handPos[i + 1][2];

			if (i + 2 > tool->numHand - 1)
			{
				pt3[0] = (float)tool->handPos[i + 1][0];
				pt3[1] = (float)tool->handPos[i + 1][1];
				pt3[2] = (float)tool->handPos[i + 1][2];
			}
			else
			{
				pt3[0] = (float)tool->handPos[i + 2][0];
				pt3[1] = (float)tool->handPos[i + 2][1];
				pt3[2] = (float)tool->handPos[i + 2][2];
			}
			
			for (t = 0; t <= 1.0; t = t + 0.05)
			{
				vec[0] = 0.5 *((2 * pt1[0])	+ (-pt0[0] + pt2[0]) * t +(2 * pt0[0] - 5 * pt1[0] + 4 * pt2[0] - pt3[0]) * t * t + (- pt0[0] + 3 * pt1[0] - 3 * pt2[0] + pt3[0]) * t * t * t) ;
				vec[1] = 0.5 *((2 * pt1[1])	+ (-pt0[1] + pt2[1]) * t +(2 * pt0[1] - 5 * pt1[1] + 4 * pt2[1] - pt3[1]) * t * t + (- pt0[1] + 3 * pt1[1] - 3 * pt2[1] + pt3[1]) * t * t * t) ;
				vec[2] = 0.5 *((2 * pt1[2])	+ (-pt0[2] + pt2[2]) * t +(2 * pt0[2] - 5 * pt1[2] + 4 * pt2[2] - pt3[2]) * t * t + (- pt0[2] + 3 * pt1[2] - 3 * pt2[2] + pt3[2]) * t * t * t) ;
				draw->lineTo(draw->data, vec, LWWIRE_ABSOLUTE);
			}
			}
			vec[0] = (float)tool->handPos[tool->numHand-1][0];
			vec[1] = (float)tool->handPos[tool->numHand-1][1];
			vec[2] = (float)tool->handPos[tool->numHand-1][2];
			draw->lineTo(draw->data, vec, LWWIRE_ABSOLUTE);
		}

	tool->dirty = 0;
}


static int Spline_Dirty(SplineTool *tool )
{
	return tool->dirty ? LWT_DIRTY_WIREFRAME | LWT_DIRTY_HELPTEXT : 0;
}


static void Spline_Done (SplineTool	*tool)
{
	if (tool)
    {
        if (tool->cache)
            free(tool->cache);
		free (tool);
    }
	if ( fp ) { fclose( fp ); fp = NULL; }

}



static XCALL_(int)
Activate (long	version,GlobalFunc	*global,LWMeshEditTool	*local,void	*serverData)
{
	if (version != LWMESHEDITTOOL_VERSION)
		return AFUNC_BADVERSION;

	if (!get_xpanf(global)) return AFUNC_BADGLOBAL;

	query = static_cast<LWStateQueryFuncs*>( global( LWSTATEQUERYFUNCS_GLOBAL, GFUSE_TRANSIENT ));
	if (!query) return AFUNC_BADGLOBAL;

	SplineTool *tool = new_Spline();
	if (!tool)
		return AFUNC_OK;
 
	local->instance   = tool;
	local->tool->done  = (done_function)Spline_Done;
	local->tool->count = (count_function)Spline_Count;
	local->tool->draw  = (draw_function)Spline_Draw;
    local->tool->dirty = (dirty_function)Spline_Dirty;
	local->tool->up	   = (up_function)Spline_Up;
	local->tool->help  = (help_function)Spline_Help;
	local->tool->move  = (move_function)Spline_Move;
	local->tool->down  = (down_function)Spline_Down;
	local->tool->panel = (panel_function)Spline_Panel;
	local->tool->event = (event_function)Spline_Event;
	local->build       = (build_function)Spline_Build;
	local->test        = (test_function)Spline_Test;
	local->end         = (end_function)Spline_End;

	fp = fopen( TR_FILENAME, "w" );
	//SetWindowsHookEx( WH_MOUSE, mouse_hook, hinst, 0 );

	return AFUNC_OK;
}

static ServerTagInfo srvtag[] = {
   { "tcVDeformer", SRVTAG_USERNAME | LANGID_USENGLISH },
   { "Modify", SRVTAG_CMDGROUP },
   { "Translate", SRVTAG_MENU },
   { "tcVDeformer", SRVTAG_BUTTONNAME },
   { "", 0 }
};


    
extern "C"
{
    ServerRecord ServerDesc[] = {
        { LWMESHEDITTOOL_CLASS, "tcVDeformer", (activate_function)Activate, srvtag },
        { NULL }
    };
}

