//
// Filename :   myMath.c
// Abstruct :   Method for Mat
// Author   :   application_division@atit.jp
// Update   :   2025/09/28	New Creation
#include "myMath.h" 

//
// Method   :   myCosine
// Abstruct :   引数のラジアン値より余弦を返す
// Argument :   double x : [I]ラジアン値
// Return   :   double
//              計算結果
double myCosine( double x ) {
    double sum = 1.0;
	double k = 1.0;
    x -= (int)(x / (2.0 * M_PI)) * 2.0 * M_PI;
    for (int i = 1;; i += 2) {
        k *= -(x * x) / ((double)i * ((double)i + 1));
    	if (sum == sum + k) {
    		return sum;
    	} else {
    		sum += k;
    	}
    }
    return x;	// warning 対応、ここには来ない
}

//
// Method   :   mySine
// Abstruct :   引数のラジアン値より正弦を返す
// Argument :   double x : [I]ラジアン値
// Return   :   double
//              計算結果
double mySine( double x ) {
    double sum = 0.0;
	double k = 0.0;
    x -= (int)(x / (2.0 * M_PI)) * 2.0 * M_PI;
	sum = x;
	k = x;
    for (int i = 2;; i += 2) {
        k *= -(x * x) / ((double)i * ((double)i + 1));
    	if (sum == sum + k) {
    		return sum;
    	} else { 
    		sum += k;
    	}
    }
    return x;	// warning 対応、ここには来ない
}

//
// Method   :   myRandmizeInit
// Abstruct :   ランダムシード設定
// Argument :   n/a
// Return   :   n/a
void myRandmizeInit( void ) {
	srand((unsigned int)time( NULL ));
}

//
// Method   :   myRandmize
// Abstruct :   乱数を返す(rand()のラッパー)
// Argument :   n/a
// Return   :   int
//              乱数値
int myRandmize( void ) {
    return rand();
}
