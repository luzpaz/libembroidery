#include "embroidery.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *rules[] = {"+BF-AFA-FB+", "-AF+BFB+FA-"};

L_system hilbert_curve_l_system = {
    'A', "AB", "F+-", (char**)rules
};

int max_stitches = 100000;

/* This is a slow generation algorithm */
int lindenmayer_system(L_system L, char *state, int iterations, int complete)
{
    /* We know that the full length of the curve has to fit within
     * the number of stitches and we can cancel consequtive +-, -+
     * etc.
     *

Potential reference:

@book{Prusinkiewicz1996Mar,
    author = {Prusinkiewicz, Przemyslaw and Lindenmayer, Aristid and Hanan, J. S. and Fracchia, F. D. and Fowler, D. R. and de Boer, M. J. M. and Mercer, L.},
    title = {{The Algorithmic Beauty of Plants (The Virtual Laboratory)}},
    year = {1996},
    month = {Mar},
    publisher = {Springer}
}
     */
    char *new_state;
    int j;

    if (complete == 0) {
        state[0] = L.axiom;
        state[1] = 0;
        lindenmayer_system(L, state, iterations, complete+1);
        return 0;
    }

    new_state = state + max_stitches*5;

    new_state[0] = 0;

    /* replace letters using rules by copying to new_state */
    for (j=0; j < strlen(state); j++) {
        if (state[j] >= 'A' && state[j] < 'F') {
            strcat(new_state, L.rules[state[j]-'A']);
        }
        if (state[j] == 'F') {
            strcat(new_state, "F");
        }
        if (state[j] == '+') {
            strcat(new_state, "+");
        }
        if (state[j] == '-') {
            strcat(new_state, "-");
        }
    }
    memcpy(state, new_state, strlen(new_state)+1);

    if (complete < iterations) {
        lindenmayer_system(L, state, iterations, complete+1);
    }
    return 0;
}

int hilbert_curve(EmbPattern *pattern, int iterations)
{
    /*
    https://en.wikipedia.org/wiki/Hilbert_curve

    Using the Lindenmayer System, so we can save work across
    different functions.
    */
    char *state;
    int i, position[2], direction;
    double scale = 1.0;

    /* Make the n-th iteration. */
    state = malloc(max_stitches*10);
    lindenmayer_system(hilbert_curve_l_system, state, iterations, 0);

    /* Convert to an embroidery pattern. */
    position[0] = 0;
    position[1] = 0;
    direction = 0;

    for (i = 0; i < strlen(state); i++) {
        if (state[i] == '+') {
            direction = (direction + 1) % 4;
            continue;
        }
        if (state[i] == '-') {
            direction = (direction + 3) % 4;
            continue;
        }
        if (state[i] == 'F') {
            int flags = NORMAL;
            switch (direction) {
            case 0:
            default:
                position[0]--;
                break;
            case 1:
                position[1]++;
                break;
            case 2:
                position[0]++;
                break;
            case 3:
                position[1]--;
                break;
            }
            embPattern_addStitchAbs(pattern, position[0]*scale, position[1]*scale, flags, 0);
        }
    }
    free(state);
    embPattern_end(pattern);
    return 0;
}

/* using the "paper folding" method (find citation) */
void generate_dragon_curve(char *state, int iterations)
{
    int i, length;
    if (iterations == 1) {
        state[0] = 'R';
        state[1] = 0;
        return;
    }
    length = strlen(state);
    for (i=length-1; i>=0; i--) {
        state[2*i+1] = state[i];
        if (i%2 == 0) {
            state[2*i] = 'R';
        } else {
            state[2*i] = 'L';
        }
    }
    state[2*length+1] = 0;
    generate_dragon_curve(state, iterations-1);
}

int dragon_curve(int iterations)
{
    return 0;
}

#if 0
StitchBlock* BreakIntoColorBlocks(EmbPattern *pattern)
{
    int i;
    int sa2 = new StitchBlock();
    int oldColor = pattern->stitchList->stitch[0].color;
    int color = pattern.ColorList[oldColor];
    sa2.Thread = new Thread(color.Red, color.Blue, color.Green);
    for (i = 0; i < pattern->stitchList->count; i++) {
        EmbStitch s = pattern->stitchList->stitch[i];
        if (s.color != oldColor) {
            yield return sa2;
            sa2 = new StitchBlock();
            color = pattern.ColorList[s.ColorIndex];
            sa2.Thread = new Thread(color.Red, color.Blue, color.Green);
            oldColor = s.ColorIndex;
        }
        int vs = new VectorStitch { Xy = new Point(s.X, s.Y), Color = s.ColorIndex };
        sa2.Stitches.Add(vs);
    }
    yield return sa2;
}



StitchBlock * BreakIntoSeparateObjects(EmbStitchBlock* blocks)
{
    int i, block;
    double previousAngle = 0.0;
    for (block=0; block<blocks->length; block++) {
        int stitches = new List<VectorStitch>();
        block.Stitches[0].Type = VectorStitchType.Contour;
        block.Stitches[block.Stitches.Count - 1].Type = VectorStitchType.Contour;

        for (int i = 0; i < block.Stitches.Count - 2; i++) { /* step 0 */
            double dx = (embVector_relativeX(block.Stitches[i].Xy, block.Stitches[i + 1].Xy, block.Stitches[i + 2].Xy));
            block.Stitches[i + 1].Type = dx <= 0 ? VectorStitchType.Run : VectorStitchType.Contour;
            block.Stitches[i].Angle = GetAngle(block.Stitches[i], block.Stitches[i + 1]);
            stitches.Add(block.Stitches[i].Clone());
            if (i > 0) {
                if ((block.Stitches[i].Type == VectorStitchType.Contour) && fabs(block.Stitches[i].Angle - previousAngle) > (20/180*embConstantPi)) {
                    yield return
                        new StitchBlock
                            {
                                Stitches = stitches,
                                Angle = stitches.Average(x => x.Angle),
                                Thread = new Thread(block.Thread.Red, block.Thread.Blue, block.Thread.Green)
                            };
                    stitches = new List<VectorStitch>();

                }
            }
        }

        /* step 1 */
        for (i = 1; i < sa.Stitches.Count - 3; i++) {
            if (sa.Stitches[i + 1].Type == VectorStitchType.Contour) {
                float dy = embVector_relativeY(sa[i + 1].XY, sa[i + 2].XY, sa[i + 3].XY);
                float dy2 = embVector_relativeY(sa[i].XY, sa[i + 1].XY, sa[i + 2].XY);
                float dy3 = embVector_relativeY(sa[i + 2].XY, sa[i + 3].XY, sa[i + 4].XY);
                if (dy)
                if (sa.Stitches[i - 1].Type == VectorStitchType.Run || sa.Stitches[i + 1].Type == VectorStitchType.Run) {
                    sa.Stitches[i].Type = VectorStitchType.Tatami;
                }
                else {
                    sa.Stitches[i].Type = VectorStitchType.Satin;
                }
            }
        }
    }
}

StitchObject * FindOutline(EmbStitchBlock* stitchData)
{
    int currColorIndex = 0, sa;
    int pOdd = new List<Point>();
    int pEven = new List<Point>();
    for (sa=0; sa<stitchData->count; sa++) {
        if (sa.Stitches.Count > 0) {
            sa.Stitches[0].Type = VectorStitchType.Contour;
            sa.Stitches[sa.Stitches.Count - 1].Type = VectorStitchType.Contour;
            /* step 0 */
            for (int i = 0; i < sa.Stitches.Count - 2; i++) {
                float dx = (GetRelativeX(sa.Stitches[i].Xy, sa.Stitches[i + 1].Xy, sa.Stitches[i + 2].Xy));
                sa.Stitches[i + 1].Type = dx <= 0 ? VectorStitchType.Run : VectorStitchType.Contour;
                sa.Stitches[i].Angle = embVector_angle(sa.Stitches[i], sa.Stitches[i + 1]);
            }
            /* step 1 */
            for (int i = 1; i < sa.Stitches.Count - 3; i++) {
                if (sa.Stitches[i + 1].Type == VectorStitchType.Contour) {
                    float dy = embVector_relativeY(sa[i + 1].XY, sa[i + 2].XY, sa[i + 3].XY);
                    float dy2 = embVector_relativeY(sa[i].XY, sa[i + 1].XY, sa[i + 2].XY);
                    float dy3 = embVector_relativeY(sa[i + 2].XY, sa[i + 3].XY, sa[i + 4].XY);
                    if (dy)
                    if (sa.Stitches[i - 1].Type == VectorStitchType.Run || sa.Stitches[i + 1].Type == VectorStitchType.Run) {
                        sa.Stitches[i].Type = VectorStitchType.Tatami;
                    }
                    else {
                        sa.Stitches[i].Type = VectorStitchType.Satin;
                    }
                }
            }
        }


        int oddEven = 0;
        foreach (VectorStitch t in sa.Stitches) {
            if ((t.Type == VectorStitchType.Contour) && (oddEven % 2) == 0) {
                pEven.Add(t.Xy);

                oddEven++;
            }
            else if ((t.Type == VectorStitchType.Contour) && (oddEven % 2) == 1) {
                pOdd.Add(t.Xy);
                oddEven++;
            }
        }
        currColorIndex++;
        int so = new StitchObject { SideOne = pEven, SideTwo = pOdd, ColorIndex = currColorIndex };
        yield return so;
        pEven = new List<Point>();
        pOdd = new List<Point>();
        /* break; */
    }
}

EmbPattern DrawGraphics(EmbPattern p) {
    int stitchData = BreakIntoColorBlocks(p);

    int outBlock = new List<StitchBlock>(BreakIntoSeparateObjects(stitchData));
    foreach(var block in stitchData) {
        foreach (var stitch in block.Stitches) {
            if (stitch.Angle != 0) {
                int aaa = 1;
            }
        }
    }
    int xxxxx = outBlock;
    int objectsFound = FindOutline(stitchData);
    int outPattern = new Pattern();
    outPattern.AddColor(new Thread(255, 0, 0, "none", "None"));
    int colorIndex = outPattern.ColorList.Count - 1;
    int r = new Random();
    foreach (StitchObject stitchObject in objectsFound) {
        if (stitchObject.SideOne.Count > 1 && stitchObject.SideTwo.Count > 1) {
            outPattern.AddColor(new Thread((byte) (r.Next()%256), (byte) (r.Next()%256), (byte) (r.Next()%256),
                                           "none", "None"));
            colorIndex++;
            outPattern.AddStitchRelative(0, 0, StitchTypes.Stop);
            int points = stitchObject.Generate2(75);
            foreach (var point in points) {
                outPattern.AddStitchAbsolute(point.X, point.Y, StitchTypes.Normal);
            }
            break;
            StitchObject stitchObject = objectsFound[1];))
            if (stitchObject.SideOne.Count > 0) {
                outPattern.StitchList.Add(new Stitch(stitchObject.SideOne[0].X, stitchObject.SideOne[0].Y,
                                                     StitchType.Jump, colorIndex));
            }
            foreach (Point t in stitchObject.SideOne) {
            
                outPattern.StitchList.Add(new Stitch(t.X, t.Y,
                                                     StitchType.Normal, colorIndex));
            }
            foreach (Point t in stitchObject.SideTwo) {
                outPattern.StitchList.Add(new Stitch(t.X, t.Y,
                                                     StitchType.Normal, colorIndex));
            }
            break;
        }
    }
    outPattern.AddStitchRelative(0, 0, StitchTypes.End);
    return outPattern;
    /*
    return (SimplifyOutline(outPattern));
    */
}

EmbPattern SimplifyOutline(EmbPattern pattern)
{
    int v = new Vertices();
    v.AddRange(pattern.StitchList.Select(point => new Vector2(point.X, point.Y)));
    int output = SimplifyTools.DouglasPeuckerSimplify(v, 10);
    int patternOut = new Pattern();
    foreach (var color in pattern.ColorList)
    {
        patternOut.AddColor(color);
    }

    foreach (var vertex in output)
    {
        patternOut.AddStitchAbsolute(vertex.X, vertex.Y, StitchTypes.Normal);
    }
    patternOut.AddStitchRelative(0, 0, StitchTypes.End);
    return patternOut;
}

bool[] _usePt;
double _distanceTolerance;

/* Removes all collinear points on the polygon. */
Vertices CollinearSimplify(Vertices vertices, float collinearityTolerance)
{
    /* We can't simplify polygons under 3 vertices */
    if (vertices.Count < 3)
        return vertices;

    int simplified = new Vertices();

    for (int i = 0; i < vertices.Count; i++) {
        int prevId = vertices.PreviousIndex(i);
        int nextId = vertices.NextIndex(i);

        Vector2 prev = vertices[prevId];
        Vector2 current = vertices[i];
        Vector2 next = vertices[nextId];

        /* If they collinear, continue */
        if (embVector_collinear(ref prev, ref current, ref next, collinearityTolerance))
            continue;

        simplified.Add(current);
    }

    return simplified;
}


/* Removes all collinear points on the polygon.
 * Has a default bias of 0
 *
 * param vertices: The polygon that needs simplification.
 * returns: A simplified polygon.
 */
Vertices CollinearSimplify(Vertices vertices)
{
    return CollinearSimplify(vertices, 0);
}

/*
 * Ramer-Douglas-Peucker polygon simplification algorithm.
 * This is the general recursive version that does not use the
 * speed-up technique by using the Melkman convex hull.
 * If you pass in 0, it will remove all collinear points.
 */
Vertices DouglasPeuckerSimplify(Vertices vertices, float distanceTolerance)
{
    _distanceTolerance = distanceTolerance;

    _usePt = new bool[vertices.Count];
    for (int i = 0; i < vertices.Count; i++)
    {
        _usePt[i] = true;
    }
    SimplifySection(vertices, 0, vertices.Count - 1);
    int result = new Vertices();
    result.AddRange(vertices.Where((t, i) => _usePt[i]));
    return result;
}

void SimplifySection(Vertices vertices, int i, int j)
{
    if ((i + 1) == j)
        return;

    Vector2 a = vertices[i];
    Vector2 b = vertices[j];
    double maxDistance = -1.0;
    int maxIndex = i;
    for (int k = i + 1; k < j; k++)
    {
        double distance = DistancePointLine(vertices[k], a, b);

        if (distance > maxDistance)
        {
            maxDistance = distance;
            maxIndex = k;
        }
    }
    if (maxDistance <= _distanceTolerance) {
        for (int k = i + 1; k < j; k++) {
            _usePt[k] = 0;
        }
    }
    else {
        SimplifySection(vertices, i, maxIndex);
        SimplifySection(vertices, maxIndex, j);
    }
}

double DistancePointLine(EmbVector p, EmbVector a, EmbVector b)
{
    /* if start == end, then use point-to-point distance */
    if (a.X == b.X && a.Y == b.Y)
        return DistancePointPoint(p, a);

    /* otherwise use comp.graphics.algorithms Frequently Asked Questions method */
    /* (1)               AC dot AB
                r =   ---------
                      ||AB||^2

                r has the following meaning:
                r=0 Point = A
                r=1 Point = B
                r<0 Point is on the backward extension of AB
                r>1 Point is on the forward extension of AB
                0<r<1 Point is interior to AB
    */

    double r = ((p.X - a.X) * (b.X - a.X) + (p.Y - a.Y) * (b.Y - a.Y))
               /
               ((b.X - a.X) * (b.X - a.X) + (b.Y - a.Y) * (b.Y - a.Y));

    if (r <= 0.0) return DistancePointPoint(p, a);
    if (r >= 1.0) return DistancePointPoint(p, b);


    /* (2)
                    (Ay-Cy)(Bx-Ax)-(Ax-Cx)(By-Ay)
                s = -----------------------------
                                Curve^2

                Then the distance from C to Point = |s|*Curve.
    */

    double s = ((a.Y - p.Y) * (b.X - a.X) - (a.X - p.X) * (b.Y - a.Y))
               /
               ((b.X - a.X) * (b.X - a.X) + (b.Y - a.Y) * (b.Y - a.Y));

    return fabs(s) * sqrt(((b.X - a.X) * (b.X - a.X) + (b.Y - a.Y) * (b.Y - a.Y)));
}

/ * From physics2d.net * /
public static Vertices ReduceByArea(Vertices vertices, float areaTolerance)
{
    if (vertices.Count <= 3)
        return vertices;

    if (areaTolerance < 0)
    {
        throw new ArgumentOutOfRangeException("areaTolerance", "must be equal to or greater then zero.");
    }

    int result = new Vertices();
    Vector2 v3;
    Vector2 v1 = vertices[vertices.Count - 2];
    Vector2 v2 = vertices[vertices.Count - 1];
    areaTolerance *= 2;
    for (int index = 0; index < vertices.Count; ++index, v2 = v3)
    {
        if (index == vertices.Count - 1)
        {
            if (result.Count == 0)
            {
                throw new ArgumentOutOfRangeException("areaTolerance", "The tolerance is too high!");
            }
            v3 = result[0];
        }
        else
        {
            v3 = vertices[index];
        }
        float old1, old2, new1;
        MathUtils.Cross(ref v1, ref v2, out old1);
        MathUtils.Cross(ref v2, ref v3, out old2);
        MathUtils.Cross(ref v1, ref v3, out new1);
        if (fabs(new1 - (old1 + old2)) > areaTolerance)
        {
            result.Add(v2);
            v1 = v2;
        }
    }
    return result;
}

/* From Eric Jordan's convex decomposition library.
 * Merges all parallel edges in the list of vertices.
 */
void MergeParallelEdges(EmbArray *vertices, float tolerance)
{
    int i;
    if (vertices.Count <= 3) {
        /* Can't do anything useful here to a triangle. */
        return;
    }

    int mergeMe = new bool[vertices.Count];
    int newNVertices = vertices.Count;

    /* Gather points to process */
    for (i = 0; i < vertices->count; i++) {
        EmbVector delta0, delta1;
        int lower = (i == 0) ? (vertices.Count - 1) : (i - 1);
        int upper = (i == vertices.Count - 1) ? (0) : (i + 1);

        embVector_subtract(vertices[i], vertices[lower], &delta0);
        embVector_subtract(vertices[upper], vertices[i], &delta1);
        float norm0 = embVector_length(delta0);
        float norm1 = embVector_length(delta0);

        if (!(norm0 > 0.0f && norm1 > 0.0f) && newNVertices > 3) {
            /* Merge identical points */
            mergeMe[i] = 1;
            newNVertices--;
        }

        embVector_normalize(delta0, &delta0);
        embVector_normalize(delta1, &delta1);
        float cross = embVector_cross(delta0, delta1);
        float dot = embVector_dot(delta0, delta1);

        if (fabs(cross) < tolerance && dot > 0 && newNVertices > 3) {
            mergeMe[i] = 1;
            newNVertices--;
        }
        else {
            mergeMe[i] = 0;
        }
    }

    if (newNVertices == vertices.Count || newNVertices == 0)
        return;

    int currIndex = 0;

    /* Copy the vertices to a new list and clear the old */
    int oldVertices = new Vertices(vertices);
    vertices.Clear();

    for (i = 0; i < oldVertices.Count; i++) {
        if (mergeMe[i] || newNVertices == 0 || currIndex == newNVertices)
            continue;

        vertices.Add(oldVertices[i]);
        currIndex++;
    }
}
#endif

void embPolygon_reduceByDistance(EmbArray *vertices, EmbArray *simplified, float distance);
void embPolygon_reduceByNth(EmbArray *vertices, EmbArray *out, int nth);

/* Reduces the polygon by distance.
 *
 * This is a non-destructive function, so the caller is responsible for
 * freeing "vertices" if they choose to keep "simplified".
 */
void embPolygon_reduceByDistance(EmbArray *vertices, EmbArray *simplified, float distance)
{
    int i;
    /* We can't simplify polygons under 3 vertices */
    if (vertices->count < 3) {
        embArray_copy(simplified, vertices);
        return;
    }

    for (i = 0; i < vertices->count; i++) {
        EmbVector delta, dist;
        int nextId = (i + 1) % vertices->count;

        embVector_subtract(vertices->vector[nextId], vertices->vector[i], &delta);

        /* If they are closer than the distance, continue */
        if (embVector_length(delta) < distance) {
            continue;
        }

        embArray_addVector(simplified, vertices->vector[i]);
    }
}

/* Reduces the polygon by removing the Nth vertex in the vertices list.
 * This is a non-destructive function, so the caller is responsible for
 * freeing vertices if they choose to keep out.
 */
void embPolygon_reduceByNth(EmbArray *vertices, EmbArray *out, int nth)
{
    int i;
    /* We can't simplify polygons under 3 vertices */
    if (vertices->count < 3) {
        embArray_copy(out, vertices);
        return;
    }

    for (i=0; i<vertices->count; i++) {
        if (i!=nth) {
            embArray_addVector(out, vertices->vector[i]);
        }
    }
}

