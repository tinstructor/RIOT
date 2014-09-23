/*
 * Copyright (C) 2013 INRIA
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup tests
 * @{
 *
 * @file
 * @brief Float test application
 *
 * @author      Oliver Hahm <oliver.hahm@inria.fr>
 *
 * @}
 */

#include <stdio.h>
#include <math.h>
#include <stdio.h>
#include "fixedPointFloats.h"

#include "board.h"

int main(void)
{
    _Fract x = 17.456;
    _Fract x2 = 12.34;

    int i, j;
    //BLUE_LED_ON;
    RED_LED_ON;
    printf("1 Million Multiplications float\n");
    for (i = 0; i< 1000000;i++) {
        x*=x2;
    }
    RED_LED_OFF;
    printf(" %i\n" , (int)x);

    fixedpoint fp = toFixed(17.456);
    fixedpoint fp2 = toFixed(12.24);
    printf("1 Million Multiplications fixed\n");
    for (i = 0; i< 1000000;i++) {
        fp = ffMul(fp,fp2);
    }
    RED_LED_OFF;
    printf(" %i\n" , toInt(fp));
    printf(" %i\n" , (int)x);


    RED_LED_ON;
    x = 9999;
    x2 = 1.5;
    printf("1 Million Divisions float\n");
    for (i = 0; i< 1000000;i++) {
        x/=x2;
    }
    RED_LED_OFF;
    printf(" %i\n" , (int)x);

    fp = toFixed(9999);
    fp2 = toFixed(1.5);
    printf("1 Million Divisions fixed\n");
    for (i = 0; i< 1000000;i++) {
        fp = ffDiv(fp,fp2);
    }
    RED_LED_OFF;
    printf(" %i\n" , toInt(fp));
    printf(" %i\n" , (int)x);






/*
2014-09-19 18:27:21,832 - INFO # 1 Million Multiplications float
2014-09-19 18:27:23,389 - INFO #  2147483647
2014-09-19 18:27:23,393 - INFO # 1 Million Multiplications fixed
2014-09-19 18:27:23,832 - INFO #  20423
2014-09-19 18:27:23,833 - INFO #  2147483647
2014-09-19 18:27:23,835 - INFO # 1 Million Divisions float
2014-09-19 18:27:49,376 - INFO #  0
2014-09-19 18:27:49,380 - INFO # 1 Million Divisions fixed
2014-09-19 18:27:53,057 - INFO #  0
2014-09-19 18:27:53,058 - INFO #  0

*/

/*
    float t = 0.00123;
    int t2 = t*10000000;
    printf(" %i\n" , t2);
    for (i=0.0; i < x; ++i) {
        for (j = 0.0; j < i*x/7.0; ++j)
            printf(".");
        printf(" %f, %f", i, j);
        puts("");
    }
    */

/*

GCC _Fract vs toFixed()

2014-09-22 19:02:15,346 - INFO # kernel_init(): This is RIOT! (Version: 2014.05-745-g383e-volastation-01-tiva)
2014-09-22 19:02:15,349 - INFO # kernel_init(): jumping into first task...
2014-09-22 19:02:15,352 - INFO # 1 Million Multiplications float
2014-09-22 19:02:15,511 - INFO #  0
2014-09-22 19:02:15,514 - INFO # 1 Million Multiplications fixed
2014-09-22 19:02:15,953 - INFO #  20423
2014-09-22 19:02:15,953 - INFO #  0
2014-09-22 19:02:15,956 - INFO # 1 Million Divisions float
2014-09-22 19:02:16,715 - INFO #  0
2014-09-22 19:02:16,717 - INFO # 1 Million Divisions fixed
2014-09-22 19:02:20,316 - INFO #  0
2014-09-22 19:02:20,316 - INFO #  0

*/
}
