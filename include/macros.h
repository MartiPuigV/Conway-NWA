#include <eadk.h>
#include <stdlib.h>

#ifndef MACROS_H_
#define MACROS_H_

extern int SCALE;

#ifndef H
#define H (240 / SCALE)
#endif // H
#ifndef W
#define W (320 / SCALE)
#endif // W

#ifndef IDX
#define IDX(x, y) ((y) * W + (x))
#endif //IDX
#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif // MIN
#ifndef MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif // MAX

#endif // MACROS_H_

