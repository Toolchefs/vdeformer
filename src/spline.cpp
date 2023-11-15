#include "spline.h"
#include <lwmodtool.h>


DynaMonitorFuncs	*globFun_mon = NULL;

/*
 * Local information packet.  This includes the mesh edit context, monitor,
 * and polygon count.  Also holds the variable spike factor.
 */

typedef struct st_SplineData {
	MeshEditOp		*op;
	LWMonitor		*mon;
	unsigned int	count;
	FILE			*fp;
	SplineTool		*tool;
	float			len;
} SplineData;

LWDVector RestPos[20];
int counter = 0;
int numpoints = 0;

typedef struct st_Line
{
	LWDVector direction;
	LWDVector origin;
}Line;

void backToRestSelected(SplineTool *tool)
{
	int i;
	LWDVector offset;
	
	offset[0] = -tool->handPos[tool->currHand][0] + RestPos[tool->currHand][0];
	offset[1] = -tool->handPos[tool->currHand][1] + RestPos[tool->currHand][1];
	offset[2] = -tool->handPos[tool->currHand][2] + RestPos[tool->currHand][2];

	tool->handPos[tool->currHand][0] = RestPos[tool->currHand][0];
	tool->handPos[tool->currHand][1] = RestPos[tool->currHand][1];
	tool->handPos[tool->currHand][2] = RestPos[tool->currHand][2];

	if (!tool->child_comp)
	for ( i = tool->currHand + 1; i < tool->numHand;i++)
	{
		tool->handPos[i][0] += offset[0];
		tool->handPos[i][1] += offset[1];
		tool->handPos[i][2] += offset[2];
	}
}

void backToRestAll(SplineTool *tool)
{
	int i;
	for ( i = 0; i < tool->numHand;i++)
	{
		tool->handPos[i][0] = RestPos[i][0];;
		tool->handPos[i][1] = RestPos[i][1];;
		tool->handPos[i][2] = RestPos[i][2];;
	}
}

float PointLineDistanceSquared3D(Point3D q, Line l, int normalized, float *t)
{
	float distanceSquared;
	LWDVector temp, vec, tempq;
	Point3D qPrime;
	float tt;

	tempq[0] = q.x;
	tempq[1] = q.y;
	tempq[2] = q.z;

	VSUB3(temp, tempq, l.origin);
	tt = VDOT(l.direction, temp);

	if (!normalized) 
		tt /= VDOT(l.direction, l.direction);

	qPrime.x = l.origin[0] + tt * l.direction[0];
	qPrime.y = l.origin[1] + tt * l.direction[1];
	qPrime.z = l.origin[2] + tt * l.direction[2];

	*t = tt;

	vec[0] = q.x - qPrime.x;
	vec[1] = q.y - qPrime.y;
	vec[2] = q.z - qPrime.z;
	distanceSquared = VDOT(vec, vec);

	return distanceSquared;
}

float PointPolylineDistance3D(Point3D p, LWDVector vertices[], int nSegments, int *ind, float *tt)
{
	float dSq;
	float finalDist;
	float xMinusA, yMinusB, zMinusC;
	float xNextMinusA, yNextMinusB, zNextMinusC;

	float xMinusASq, yMinusBSq, zMinusCSq;
	float xNextMinusASq, yNextMinusBSq, zNextMinusCSq;
	
	int i;
	Line l;
	float t;

	t = 0;

	xMinusA = vertices[0][0] - p.x;
	yMinusB = vertices[0][1] - p.y;
	zMinusC = vertices[0][2] - p.z;
	xMinusASq = xMinusA * xMinusA;
	yMinusBSq = yMinusB * yMinusB;
	zMinusCSq = zMinusC * zMinusC;
	
	xNextMinusA = vertices[1][0] - p.x;
	yNextMinusB = vertices[1][1] - p.y;
	zNextMinusC = vertices[1][2] - p.z;
	xNextMinusASq = xNextMinusA * xNextMinusA;
	yNextMinusBSq = yNextMinusB * yNextMinusB;
	zNextMinusCSq = zNextMinusC * zNextMinusC;
	
	// Compute distance to first segment
	l.origin[0] = vertices[0][0];
	l.origin[1] = vertices[0][1];
	l.origin[2] = vertices[0][2];		
		
	VSUB3(l.direction, vertices[1] , vertices[0]);

	dSq = PointLineDistanceSquared3D(p, l, 0, &t);
	
	// If closest point not on segment, check appropriate end point
	if (t < 0) 
		dSq = xMinusASq + yMinusBSq + zMinusCSq;
	else 
	if (t > 1)
		dSq = xNextMinusASq + yNextMinusBSq + zNextMinusCSq;

	finalDist = dSq;
	*tt = t;
	*ind = 0;
	
	// Go through each successive segment, rejecting if possible,
	// and computing the distance squared if not rejected.
	for (i = 1; i < nSegments -1; i++) {

		if (i != nSegments - 1) {
			xMinusASq = xNextMinusASq;
			yMinusBSq = yNextMinusBSq;
			zMinusCSq = zNextMinusCSq;

			xNextMinusA = vertices[i + 1][0] - p.x;
			yNextMinusB = vertices[i + 1][1] - p.y;
			zNextMinusC = vertices[i + 1][2] - p.z;

			xNextMinusASq = xNextMinusA * xNextMinusA;
			yNextMinusBSq = yNextMinusB * yNextMinusB;
			zNextMinusCSq = zNextMinusC * zNextMinusC;
		}

		// Rejection test failed - check distance to line
		l.origin[0] = vertices[i][0];
		l.origin[1] = vertices[i][1];
		l.origin[2] = vertices[i][2];
		
		VSUB3(l.direction, vertices[i+1], vertices[i]);
		dSq = PointLineDistanceSquared3D(p, l, 0, &t);

		// If closest point not on segment, check appropriate end point
		if (t < 0) 
			dSq = xMinusASq + yMinusBSq + zMinusCSq;
		else
		if (t > 1)
			dSq = xNextMinusASq + yNextMinusBSq + zNextMinusCSq;

		if (finalDist > dSq)
		{
			finalDist = dSq;
			*tt = t;
			*ind = i;
		}
	}

	return sqrt(finalDist);
}

float PointSplineDistanceSquared3D(LWDVector p, LWDVector vertices[], int nVertices, int *ind, float *tt, LWDVector rpt0, LWDVector rpt1, LWDVector rpt2, LWDVector rpt3)
{
	float distance, t;
	LWDVector sub, pos, pt0, pt1, pt2, pt3;
	int i;
    
	VSUB3(sub, p, vertices[0]);
    distance = VLEN(sub);
	*tt = 0;
	*ind = 0;
	rpt0[0] = vertices[0][0];
	rpt0[1] = vertices[0][1];
	rpt0[2] = vertices[0][2];

	rpt1[0] = vertices[0][0];
	rpt1[1] = vertices[0][1];
	rpt1[2] = vertices[0][2];

	rpt2[0] = vertices[1][0];
	rpt2[1] = vertices[1][1];
	rpt2[2] = vertices[1][2];

	if (nVertices < 2)
	{
		rpt3[0] = vertices[1][0];
		rpt3[1] = vertices[1][1];
		rpt3[2] = vertices[1][2];	
	}
	else
	{
		rpt3[0] = vertices[2][0];
		rpt3[1] = vertices[2][1];
		rpt3[2] = vertices[2][2];	
	}

	for (i = 0; i < nVertices - 1; i++)
	{
		if (i - 1 > 0)
		{
			pt0[0] = vertices[i - 1][0];
			pt0[1] = vertices[i - 1][1];
			pt0[2] = vertices[i - 1][2];
		}
		else
		{
			pt0[0] = vertices[0][0];
			pt0[1] = vertices[0][1];
			pt0[2] = vertices[0][2];
		}

		pt1[0] = vertices[i][0];
		pt1[1] = vertices[i][1];
		pt1[2] = vertices[i][2];

		pt2[0] = vertices[i + 1][0];
		pt2[1] = vertices[i + 1][1];
		pt2[2] = vertices[i + 1][2];

		if (i + 2 > nVertices - 1)
		{
			pt3[0] = vertices[i + 1][0];
			pt3[1] = vertices[i + 1][1];
			pt3[2] = vertices[i + 1][2];
		}
		else
		{
			pt3[0] = vertices[i + 2][0];
			pt3[1] = vertices[i + 2][1];
			pt3[2] = vertices[i + 2][2];
		}
		
		for (t = 0.0; t <= 1.0; t += 0.05)
		{
			pos[0] = 0.5 *((2 * pt1[0])	+ (-pt0[0] + pt2[0]) * t +(2 * pt0[0] - 5 * pt1[0] + 4 * pt2[0] - pt3[0]) * t * t + (- pt0[0] + 3 * pt1[0] - 3 * pt2[0] + pt3[0]) * t * t * t) ;
			pos[1] = 0.5 *((2 * pt1[1])	+ (-pt0[1] + pt2[1]) * t +(2 * pt0[1] - 5 * pt1[1] + 4 * pt2[1] - pt3[1]) * t * t + (- pt0[1] + 3 * pt1[1] - 3 * pt2[1] + pt3[1]) * t * t * t) ;
			pos[2] = 0.5 *((2 * pt1[2])	+ (-pt0[2] + pt2[2]) * t +(2 * pt0[2] - 5 * pt1[2] + 4 * pt2[2] - pt3[2]) * t * t + (- pt0[2] + 3 * pt1[2] - 3 * pt2[2] + pt3[2]) * t * t * t) ;
			
			VSUB3(sub, p, pos);
			
			if (distance > VLEN(sub))
			{
				distance = VLEN(sub);
				*tt = t;
				*ind = i;

				rpt0[0] = pt0[0];
				rpt0[1] = pt0[1];
				rpt0[2] = pt0[2];

				rpt1[0] = pt1[0];
				rpt1[1] = pt1[1];
				rpt1[2] = pt1[2];

				rpt2[0] = pt2[0];
				rpt2[1] = pt2[1];
				rpt2[2] = pt2[2];

				rpt3[0] = pt3[0];
				rpt3[1] = pt3[1];
				rpt3[2] = pt3[2];
				
			}			
		}

	}	

	return distance;
}

double Dist3D (double v1[3],double v2[3])
{
	double	d, z;
	int		i;

	d = 0.0;
	for (i = 0; i < 3; i++) {
		z = v1[i] - v2[i];
		d += z * z;
	}
	return sqrt (d);
}

void RecordPosition(SplineTool *tool)
{
	int i;
	for (i = 0; i < tool->numHand; i++)
	{
		RestPos[i][0] = tool->handPos[i][0];	
		RestPos[i][1] = tool->handPos[i][1];
		RestPos[i][2] = tool->handPos[i][2];
	}
}


float GetFactor(SplineTool *tool, float dist, double radius)
{
	float factor = 1, f, k1, k2, t;
	switch (tool->fofftype)
	{
	case 0:
		// linear
        factor = 1 - dist / radius;
		return factor;
	
	case 1:
        //ease in / ease out
		k1 = tool->EaseIn / 2;
		
		if (tool->EaseOut > 0)
			k2 = 1 - tool->EaseOut / 2;
		else
			k2 = -1 + fabs(tool->EaseOut / 2);

		f = k1 * (2 / PI) + k2 - k1 + (1 - k2) * 2 / PI;
		t = 1 - dist / radius;

		if ( t < k1)
			factor = (k1 * (2 / PI) * (sin((t / k1) * (PI / 2) - PI / 2) + 1)) / f;
		else
		if (t > k2)
			factor = (k1 / (PI / 2) + k2 - k1 + ( (1 - k2) * (2 / PI)) * sin(( t - k2 ) / (1 - k2) * (PI / 2))) / f;
		else
			factor = (k1 / (PI / 2) + t - k1) / f;		

		return factor;
	}
    
    return 0;
}

int visitPoint(PointCache *cp, SplineTool *tool)
{
    for (int i = 0; i < tool->numHand;i++)
    {
        if (pow(cp->oldPos[0] - RestPos[i][0], 2) + pow(cp->oldPos[1]  - RestPos[i][1], 2) + pow(cp->oldPos[2]  - RestPos[i][2], 2) < pow(tool->radius[i], 2))
        {
            cp->pointHandleIndex = i;
            cp->pointDist = Dist3D(cp->oldPos, RestPos[i]);
            return 1;
        }
    }
    return 0;
}

int visitLine(PointCache *cp, SplineTool *tool)
{
    int ind;
    float t;
    Point3D pt;
    pt.x = cp->oldPos[0];
    pt.y = cp->oldPos[1];
    pt.z = cp->oldPos[2];
    cp->linearDist = PointPolylineDistance3D(pt, RestPos, tool->numHand, &ind, &t);
    if (t > 1)
        t = 1;
    else if (t < 0)
        t = 0;
    
    double radius = (1 - t) * tool->radius[ind] + t * tool->radius[ind + 1];
    
    if (cp->linearDist < radius )
    {
        cp->linearHandleIndex = ind;
        cp->linearT = t;
        cp->linearRadius = radius;
        
        cp->linearClosestPoint[0] = (1 - t) * RestPos[ind][0] + t * RestPos[ind + 1][0];
        cp->linearClosestPoint[1] = (1 - t) * RestPos[ind][1] + t * RestPos[ind + 1][1];
        cp->linearClosestPoint[2] = (1 - t) * RestPos[ind][2] + t * RestPos[ind + 1][2];
        
        return 1;
    }
    
    return 0;
}

int visitSpline(PointCache *cp, SplineTool *tool)
{
    int ind;
    float t;
    
    LWDVector rpt0, rpt1, rpt2, rpt3;
    cp->splineDist = PointSplineDistanceSquared3D(cp->oldPos, RestPos, tool->numHand, &ind, &t, rpt0, rpt1, rpt2, rpt3);
    
    double radius = (1 - t) * tool->radius[ind] + t * tool->radius[ind + 1];
    
    if (cp->splineDist < radius)
    {
        cp->splineHandleIndex = ind;
        cp->splineT = t;
        cp->splineRadius = radius;
        
        cp->splineClosestPoint[0] = 0.5 *((2 * rpt1[0])	+ (-rpt0[0] + rpt2[0]) * t +(2 * rpt0[0] - 5 * rpt1[0] + 4 * rpt2[0] - rpt3[0]) * t * t + (- rpt0[0] + 3 * rpt1[0] - 3 * rpt2[0] + rpt3[0]) * t * t * t) ;
        cp->splineClosestPoint[1] = 0.5 *((2 * rpt1[1])	+ (-rpt0[1] + rpt2[1]) * t +(2 * rpt0[1] - 5 * rpt1[1] + 4 * rpt2[1] - rpt3[1]) * t * t + (- rpt0[1] + 3 * rpt1[1] - 3 * rpt2[1] + rpt3[1]) * t * t * t) ;
        cp->splineClosestPoint[2] = 0.5 *((2 * rpt1[2])	+ (-rpt0[2] + rpt2[2]) * t +(2 * rpt0[2] - 5 * rpt1[2] + 4 * rpt2[2] - rpt3[2]) * t * t + (- rpt0[2] + 3 * rpt1[2] - 3 * rpt2[2] + rpt3[2]) * t * t * t) ;
        
        return 1;
    }
    return 0;
}

int fastPointControl(SplineData	*dat, LWPntID id)
{
    PointCache *cp = &dat->tool->cache[counter];
    counter++;

    cp->id = id;
    
    LWDVector pos;
    dat->op->pointPos(dat->op->state, id, pos);
    
    cp->oldPos[0] = pos[0];
    cp->oldPos[1] = pos[1];
    cp->oldPos[2] = pos[2];
    
    cp->newPos[0] = pos[0];
    cp->newPos[1] = pos[1];
    cp->newPos[2] = pos[2];
    
    cp->pointContained = 0;
    cp->linearContained = 0;
    cp->splineContained = 0;
    cp->symmContained = 0;
    
	if (!(dat->op->pointFlags(dat->op->state, id) & EDDF_SELECT))
		return EDERR_NONE;
    
    cp->pointContained = visitPoint(cp, dat->tool);
    cp->linearContained = visitLine(cp, dat->tool);
    cp->splineContained = visitSpline(cp, dat->tool);
    
    return EDERR_NONE;
}


void VDeformerData::operator()( const tbb::blocked_range<size_t>& r ) const
{
    float t, factor;
    int ind;
    
    SplineTool *tool = m_data.tool;
    
    for( size_t i=r.begin(); i!=r.end(); ++i )
    {
        PointCache *cp = &tool->cache[i];
        //we bring the points to the default position
        cp->newPos[0] = cp->oldPos[0];
        cp->newPos[1] = cp->oldPos[1];
        cp->newPos[2] = cp->oldPos[2];
        
        switch (m_data.numcase) {
            case 0:
                if (cp->pointContained)
                {
                    factor = GetFactor(tool, cp->pointDist, tool->radius[cp->pointHandleIndex]);
                    cp->newPos[0] = cp->oldPos[0] + (tool->handPos[cp->pointHandleIndex][0] - RestPos[cp->pointHandleIndex][0]) * factor * tool->strength;
                    cp->newPos[1] = cp->oldPos[1] + (tool->handPos[cp->pointHandleIndex][1] - RestPos[cp->pointHandleIndex][1]) * factor * tool->strength;
                    cp->newPos[2] = cp->oldPos[2] + (tool->handPos[cp->pointHandleIndex][2] - RestPos[cp->pointHandleIndex][2]) * factor * tool->strength;
                }
                break;
                
            case 1:
                if (cp->linearContained)
                {
                    t = cp->linearT;
                    ind = cp->linearHandleIndex;
                    factor = GetFactor(tool, cp->linearDist, cp->linearRadius);
                    
                    cp->newPos[0] = cp->oldPos[0] + ((1 - t) * tool->handPos[ind][0] + t * tool->handPos[ind + 1][0] - cp->linearClosestPoint[0]) * factor * tool->strength;
                    cp->newPos[1] = cp->oldPos[1] + ((1 - t) * tool->handPos[ind][1] + t * tool->handPos[ind + 1][1] - cp->linearClosestPoint[1]) * factor * tool->strength;
                    cp->newPos[2] = cp->oldPos[2] + ((1 - t) * tool->handPos[ind][2] + t * tool->handPos[ind + 1][2] - cp->linearClosestPoint[2]) * factor * tool->strength;
                }
                break;
                
            case 2:
                if (cp->splineContained)
                {
                    t = cp->splineT;
                    ind = cp->splineHandleIndex;
                    factor = GetFactor(tool, cp->splineDist, cp->splineRadius);
                    
                    LWDVector pt0, pt1, pt2, pt3;
                    
                    int index = ind - 1 > 0 ? ind - 1: 0;
                    pt0[0] = tool->handPos[index][0];
                    pt0[1] = tool->handPos[index][1];
                    pt0[2] = tool->handPos[index][2];
                    
                    pt1[0] = tool->handPos[ind][0];
                    pt1[1] = tool->handPos[ind][1];
                    pt1[2] = tool->handPos[ind][2];
                    
                    pt2[0] = tool->handPos[ind + 1][0];
                    pt2[1] = tool->handPos[ind + 1][1];
                    pt2[2] = tool->handPos[ind + 1][2];
                    
                    index = ind + 2 > tool->numHand - 1? ind + 1: ind + 2;
                    pt3[0] = tool->handPos[index][0];
                    pt3[1] = tool->handPos[index][1];
                    pt3[2] = tool->handPos[index][2];
                    
                    cp->newPos[0] = (0.5 *((2 * pt1[0])	+ (-pt0[0] + pt2[0]) * t +(2 * pt0[0] - 5 * pt1[0] + 4 * pt2[0] - pt3[0]) * t * t + (- pt0[0] + 3 * pt1[0] - 3 * pt2[0] + pt3[0]) * t * t * t)) - cp->splineClosestPoint[0];
                    cp->newPos[1] = (0.5 *((2 * pt1[1])	+ (-pt0[1] + pt2[1]) * t +(2 * pt0[1] - 5 * pt1[1] + 4 * pt2[1] - pt3[1]) * t * t + (- pt0[1] + 3 * pt1[1] - 3 * pt2[1] + pt3[1]) * t * t * t)) - cp->splineClosestPoint[1];
                    cp->newPos[2] = (0.5 *((2 * pt1[2])	+ (-pt0[2] + pt2[2]) * t +(2 * pt0[2] - 5 * pt1[2] + 4 * pt2[2] - pt3[2]) * t * t + (- pt0[2] + 3 * pt1[2] - 3 * pt2[2] + pt3[2]) * t * t * t)) - cp->splineClosestPoint[2];
                    
                    cp->newPos[0] *= factor * tool->strength;
                    cp->newPos[1] *= factor * tool->strength;
                    cp->newPos[2] *= factor * tool->strength;
                    
                    cp->newPos[0] += cp->oldPos[0];
                    cp->newPos[1] += cp->oldPos[1];
                    cp->newPos[2] += cp->oldPos[2];
                    
                }
                break;
        }
    }
};

void CheckSymmetryData::operator()( const tbb::blocked_range<size_t>& r ) const
{
    SplineTool *tool = m_data.tool;
    
    for( size_t i=r.begin(); i!=r.end(); ++i )
    {
        PointCache *cp = &tool->cache[i];
        if (cp->pointContained || cp->linearContained || cp->splineContained)
            continue;
            
        for (int j = 0; j < numpoints; j++)
        {
            if (i==j)
                continue;
            
            PointCache *otherCp = &tool->cache[j];
            if ((otherCp->pointContained || otherCp->linearContained || otherCp->splineContained) &&
                (otherCp->oldPos[0] + cp->oldPos[0]) * (otherCp->oldPos[0] + cp->oldPos[0]) <= 0.0000025  &&
                (otherCp->oldPos[1] == cp->oldPos[1]) && (otherCp->oldPos[2] == cp->oldPos[2]))
            {
                cp->symmContained = 1;
                cp->symmIndex = j;
                break;
            }
        }
    }
}

void ApplySymmetryData::operator()( const tbb::blocked_range<size_t>& r ) const
{
    SplineTool *tool = m_data.tool;
    
    for( size_t i=r.begin(); i!=r.end(); ++i )
    {
        PointCache *cp = &tool->cache[i];
        if (cp->symmContained)
        {
            cp->newPos[0] = -tool->cache[cp->symmIndex].newPos[0];
            cp->newPos[1] = tool->cache[cp->symmIndex].newPos[1];
            cp->newPos[2] = tool->cache[cp->symmIndex].newPos[2];
        }
    }
}

int Spline(MeshEditOp *op, SplineTool *Spline, FILE *fp)
{	
	SplineData dat;
	EDError err = EDERR_NONE;
	
	numpoints = op->pointCount(op->state, OPLYR_PRIMARY , EDCOUNT_ALL );
    
    if (!Spline->cache)
        Spline->cache = new PointCache[numpoints];
	
	dat.op = op;
	dat.fp = fp;
	dat.tool = Spline;
    dat.count = 0;

    int numcase = Spline->numHand > 1 ? Spline->inter: 0;
    
    if (Spline->recorded)
    {
        if (Spline->justRecorded)
        {
          	counter = 0;
            err = (*op->fastPointScan) (op->state, (fastPntScan_function)fastPointControl, &dat, OPLYR_PRIMARY, false);
            Spline->justRecorded = 0;
            Spline->justSymmetry = 1;
        }
        
        VDeformerData dataObj(Spline, numcase);
        tbb::parallel_for(tbb::blocked_range<size_t>(0, numpoints), dataObj);
        //deformPoints(&dat);
        
        //Simmetry
        if (query->mode(LWM_MODE_SYMMETRY ))
        {
            if (Spline->justSymmetry)
            {
                CheckSymmetryData checkSymmDataObj(Spline);
                tbb::parallel_for(tbb::blocked_range<size_t>(0, numpoints), checkSymmDataObj);
                Spline->justSymmetry = 0;
            }
            
            ApplySymmetryData applySymmDataObj(Spline);
            tbb::parallel_for(tbb::blocked_range<size_t>(0, numpoints), applySymmDataObj);
        }
        
        for (int i = 0; i < numpoints; i++)
        {
            PointCache *cp = &Spline->cache[i];
            if (cp->symmContained ||
                (numcase == 0 && cp->pointContained) ||
                (numcase == 1 && cp->linearContained) ||
                (numcase == 2 && cp->splineContained))
                op->pntMove(op->state, cp->id, cp->newPos);
        }
    }
    
	(*op->done) (op->state, EDERR_NONE, 0);

	return err;
}
