#ifndef MY_MATH_H
#define MY_MATH_H

//
// Filename :   myMath.h
// Abstruct :   definition for my math functions
// Author   :   application_division@atit.jp
// Update   :   2025/09/28	New Creation
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

double mySine( double );
double myCosine( double );
void   myRandmizeInit( void );
int    myRandmize( void );
#endif
