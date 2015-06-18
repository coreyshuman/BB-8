
#include <plib.h>
#include <math.h>
#include "HardwareProfile.h"


QWORD magnitude(QWORD *q)
{
    return sqrt(q[0]*q[0] + q[1]*q[1] + q[2]*q[2] + q[3]*q[3]);
}

void normalize(QWORD *in, QWORD *out)
{
    QWORD mag = magnitude(in);

    out[0] = in[0] / mag;
    out[1] = in[1] / mag;
    out[2] = in[2] / mag;
    out[3] = in[3] / mag;
}