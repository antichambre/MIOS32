/****************************************************************************
 *                                                                          *
 * Header file of the nI2S Digital Toy Synth Engine - lookup tables         *
 *                                                                          *
 ****************************************************************************
 *                                                                          *
 *  Copyright (C) 2009 nILS Podewski (nils@podewski.de)                     *
 *                                                                          *
 *  Licensed for personal non-commercial use only.                          *
 *  All other rights reserved.                                              *
 *                                                                          *
 ****************************************************************************/

#ifndef _TABLES_H
#define _TABLES_H

/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////

static const float accumValueByNote[100] = {
// C0
  22.86138,   24.21768,	  25.65788,	  27.19595,	  28.80394,	  30.52379,	  32.32753,	  34.25711,	  36.29856,	  38.45186,	  40.74499,	  43.16396,
// C1
  45.72276,   48.44934,	  51.32974,   54.37792,	  57.60788,	  61.03359,	  64.66904,	  68.51422,	  72.58313,	  76.90372,	  81.47600,   86.32792,
// C2
  91.45950,	  96.89869,  102.65948,  108.75584,  115.22974,  122.08116,  129.33808,  137.02845,  145.18024,  153.80744,  162.95199,  172.64186,
// C3
 182.90502,  193.78340,  205.30497,  217.51169,  230.44550,  244.14834,  258.67615,  274.05690,  290.34651,  307.61489,  325.90399,  345.28373,  
// C4
 365.82401,  387.56679,  410.60994,  435.03736,  460.90498,  488.31067,  517.33833,  548.11380,  580.69301,  615.22977,  651.80798,  690.56746,  
// C5
 731.63404,  775.14757,  821.23387,  870.06074,  921.80996,  976.62134, 1034.69064, 1096.21361, 1161.40000, 1230.45955, 1303.62994, 1381.14889, 
// C6
1463.26809, 1550.28115, 1642.46774, 1740.13547, 1843.60593, 1953.22869, 2069.38127, 2192.42723, 2322.80001, 2460.91909, 2607.25988, 2762.28380, 
// C7
2926.53617, 3100.56230, 3284.93548, 3480.27093, 3687.21186, 3906.47136, 4138.76254, 4384.85445, 4645.60002, 4921.83819, 5214.50579, 5524.58159, 
// C8
5853.08633, 6201.12461, 6569.87096, 6960.52788
};   

static const s16 ssineTable512[512] = {
0		,402	,804	,1206	,1608	,2009	,2410	,2811	,3212	,3612	,4011	,4410	,4808	,5205	,5602	,5998	,
6393	,6786	,7179	,7571	,7962	,8351	,8739	,9126	,9512	,9896	,10278	,10659	,11039	,11417	,11793	,12167	,
12539	,12910	,13279	,13645	,14010	,14372	,14732	,15090	,15446	,15800	,16151	,16499	,16846	,17189	,17530	,17869	,
18204	,18537	,18868	,19195	,19519	,19841	,20159	,20475	,20787	,21096	,21403	,21705	,22005	,22301	,22594	,22884	,
23170	,23452	,23731	,24007	,24279	,24547	,24811	,25072	,25329	,25582	,25832	,26077	,26319	,26556	,26790	,27019	,
27245	,27466	,27683	,27896	,28105	,28310	,28510	,28706	,28898	,29085	,29268	,29447	,29621	,29791	,29956	,30117	,
30273	,30424	,30571	,30714	,30852	,30985	,31113	,31237	,31356	,31470	,31580	,31685	,31785	,31880	,31971	,32057	,
32137	,32213	,32285	,32351	,32412	,32469	,32521	,32567	,32609	,32646	,32678	,32705	,32728	,32745	,32757	,32765	,
32767	,32765	,32757	,32745	,32728	,32705	,32678	,32646	,32609	,32567	,32521	,32469	,32412	,32351	,32285	,32213	,
32137	,32057	,31971	,31880	,31785	,31685	,31580	,31470	,31356	,31237	,31113	,30985	,30852	,30714	,30571	,30424	,
30273	,30117	,29956	,29791	,29621	,29447	,29268	,29085	,28898	,28706	,28510	,28310	,28105	,27896	,27683	,27466	,
27245	,27019	,26790	,26556	,26319	,26077	,25832	,25582	,25329	,25072	,24811	,24547	,24279	,24007	,23731	,23452	,
23170	,22884	,22594	,22301	,22005	,21705	,21403	,21096	,20787	,20475	,20159	,19841	,19519	,19195	,18868	,18537	,
18204	,17869	,17530	,17189	,16846	,16499	,16151	,15800	,15446	,15090	,14732	,14372	,14010	,13645	,13279	,12910	,
12539	,12167	,11793	,11417	,11039	,10659	,10278	,9896	,9512	,9126	,8739	,8351	,7962	,7571	,7179	,6786	,
6393	,5998	,5602	,5205	,4808	,4410	,4011	,3612	,3212	,2811	,2410	,2009	,1608	,1206	,804	,402	,
0		,-402	,-804	,-1206	,-1608	,-2009	,-2410	,-2811	,-3212	,-3612	,-4011	,-4410	,-4808	,-5205	,-5602	,-5998	,
-6393	,-6786	,-7179	,-7571	,-7962	,-8351	,-8739	,-9126	,-9512	,-9896	,-10278	,-10659	,-11039	,-11417	,-11793	,-12167	,
-12539	,-12910	,-13279	,-13645	,-14010	,-14372	,-14732	,-15090	,-15446	,-15800	,-16151	,-16499	,-16846	,-17189	,-17530	,-17869	,
-18204	,-18537	,-18868	,-19195	,-19519	,-19841	,-20159	,-20475	,-20787	,-21096	,-21403	,-21705	,-22005	,-22301	,-22594	,-22884	,
-23170	,-23452	,-23731	,-24007	,-24279	,-24547	,-24811	,-25072	,-25329	,-25582	,-25832	,-26077	,-26319	,-26556	,-26790	,-27019	,
-27245	,-27466	,-27683	,-27896	,-28105	,-28310	,-28510	,-28706	,-28898	,-29085	,-29268	,-29447	,-29621	,-29791	,-29956	,-30117	,
-30273	,-30424	,-30571	,-30714	,-30852	,-30985	,-31113	,-31237	,-31356	,-31470	,-31580	,-31685	,-31785	,-31880	,-31971	,-32057	,
-32137	,-32213	,-32285	,-32351	,-32412	,-32469	,-32521	,-32567	,-32609	,-32646	,-32678	,-32705	,-32728	,-32745	,-32757	,-32765	,
-32767	,-32765	,-32757	,-32745	,-32728	,-32705	,-32678	,-32646	,-32609	,-32567	,-32521	,-32469	,-32412	,-32351	,-32285	,-32213	,
-32137	,-32057	,-31971	,-31880	,-31785	,-31685	,-31580	,-31470	,-31356	,-31237	,-31113	,-30985	,-30852	,-30714	,-30571	,-30424	,
-30273	,-30117	,-29956	,-29791	,-29621	,-29447	,-29268	,-29085	,-28898	,-28706	,-28510	,-28310	,-28105	,-27896	,-27683	,-27466	,
-27245	,-27019	,-26790	,-26556	,-26319	,-26077	,-25832	,-25582	,-25329	,-25072	,-24811	,-24547	,-24279	,-24007	,-23731	,-23452	,
-23170	,-22884	,-22594	,-22301	,-22005	,-21705	,-21403	,-21096	,-20787	,-20475	,-20159	,-19841	,-19519	,-19195	,-18868	,-18537	,
-18204	,-17869	,-17530	,-17189	,-16846	,-16499	,-16151	,-15800	,-15446	,-15090	,-14732	,-14372	,-14010	,-13645	,-13279	,-12910	,
-12539	,-12167	,-11793	,-11417	,-11039	,-10659	,-10278	,-9896	,-9512	,-9126	,-8739	,-8351	,-7962	,-7571	,-7179	,-6786	,
-6393	,-5998	,-5602	,-5205	,-4808	,-4410	,-4011	,-3612	,-3212	,-2811	,-2410	,-2009	,-1608	,-1206	,-804	,-402	
};     

static const u16 sineTable512[512] = {
32768,33170,33572,33974,34376,34777,35179,35579,35980,36380,36779,37178,37576,37973,38370,38766,
39161,39555,39948,40339,40730,41119,41508,41895,42280,42664,43047,43428,43807,44185,44561,44935,
45308,45678,46047,46414,46778,47141,47501,47859,48215,48568,48919,49268,49614,49958,50299,50637,
50973,51306,51636,51963,52288,52609,52928,53243,53556,53865,54171,54474,54774,55070,55363,55652,
55938,56221,56500,56776,57047,57316,57580,57841,58098,58351,58601,58846,59088,59325,59559,59788,
60014,60235,60452,60665,60874,61079,61279,61475,61667,61854,62037,62216,62390,62560,62725,62886,
63042,63193,63340,63483,63621,63754,63882,64006,64125,64239,64349,64454,64554,64649,64740,64826,
64906,64982,65054,65120,65181,65238,65290,65336,65378,65415,65447,65474,65497,65514,65526,65534,
65535,65534,65526,65514,65497,65474,65447,65415,65378,65336,65290,65238,65181,65120,65054,64982,
64906,64826,64740,64649,64554,64454,64349,64239,64125,64006,63882,63754,63621,63483,63340,63193,
63042,62886,62725,62560,62390,62216,62037,61854,61667,61475,61279,61079,60874,60665,60452,60235,
60014,59788,59559,59325,59088,58846,58601,58351,58098,57841,57580,57316,57047,56776,56500,56221,
55938,55652,55363,55070,54774,54474,54171,53865,53556,53243,52928,52609,52288,51963,51636,51306,
50973,50637,50299,49958,49614,49268,48919,48568,48215,47859,47501,47141,46778,46414,46047,45678,
45308,44935,44561,44185,43807,43428,43047,42664,42280,41895,41508,41119,40730,40339,39948,39555,
39161,38766,38370,37973,37576,37178,36779,36380,35980,35579,35179,34777,34376,33974,33572,33170,
32768,32366,31964,31562,31160,30759,30357,29957,29556,29156,28757,28358,27960,27563,27166,26770,
26375,25981,25588,25197,24806,24417,24028,23641,23256,22872,22489,22108,21729,21351,20975,20601,
20228,19858,19489,19122,18758,18395,18035,17677,17321,16968,16617,16268,15922,15578,15237,14899,
14563,14230,13900,13573,13248,12927,12608,12293,11980,11671,11365,11062,10762,10466,10173, 9884,
 9598, 9315, 9036, 8760, 8489, 8220, 7956, 7695, 7438, 7185, 6935, 6690, 6448, 6211, 5977, 5748,
 5522, 5301, 5084, 4871, 4662, 4457, 4257, 4061, 3869, 3682, 3499, 3320, 3146, 2976, 2811, 2650,
 2494, 2343, 2196, 2053, 1915, 1782, 1654, 1530, 1411, 1297, 1187, 1082,  982,  887,  796,  710,
  630,  554,  482,  416,  355,  298,  246,  200,  158,  121,   89,   62,   39,   22,   10,    2,
    0,    2,   10,   22,   39,   62,   89,  121,  158,  200,  246,  298,  355,  416,  482,  554,
  630,  710,  796,  887,  982, 1082, 1187, 1297, 1411, 1530, 1654, 1782, 1915, 2053, 2196, 2343,
 2494, 2650, 2811, 2976, 3146, 3320, 3499, 3682, 3869, 4061, 4257, 4457, 4662, 4871, 5084, 5301,
 5522, 5748, 5977, 6211, 6448, 6690, 6935, 7185, 7438, 7695, 7956, 8220, 8489, 8760, 9036, 9315,
 9598, 9884,10173,10466,10762,11062,11365,11671,11980,12293,12608,12927,13248,13573,13900,14230,
14563,14899,15237,15578,15922,16268,16617,16968,17321,17677,18035,18395,18758,19122,19489,19858,
20228,20601,20975,21351,21729,22108,22489,22872,23256,23641,24028,24417,24806,25197,25588,25981,
26375,26770,27166,27563,27960,28358,28757,29156,29556,29957,30357,30759,31160,31562,31964,32366
};

static const u16 sqrtTable[512] = {
   16,    32,    48,    64,    80,    96,   112,   128,   153,   179,   206,   235,   265,   296,   329,   362,   397,   432,   468,   506,   544,   584,   624,   665,   707,   750,   794,   838,   883,   930,   976,  1024,
 1072,  1121,  1171,  1222,  1273,  1325,  1378,  1431,  1485,  1540,  1595,  1651,  1708,  1765,  1823,  1881,  1940,  2000,  2060,  2121,  2183,  2245,  2307,  2371,  2434,  2499,  2564,  2629,  2695,  2762,  2829,  2896,
 2964,  3033,  3102,  3172,  3242,  3313,  3384,  3456,  3528,  3601,  3674,  3748,  3822,  3897,  3972,  4048,  4124,  4200,  4278,  4355,  4433,  4512,  4590,  4670,  4750,  4830,  4911,  4992,  5073,  5155,  5238,  5321,
 5404,  5488,  5572,  5657,  5742,  5827,  5913,  6000,  6086,  6174,  6261,  6349,  6437,  6526,  6615,  6705,  6795,  6885,  6976,  7067,  7159,  7251,  7343,  7436,  7529,  7623,  7717,  7811,  7906,  8001,  8096,  8192,
 8288,  8385,  8482,  8579,  8677,  8775,  8873,  8972,  9071,  9171,  9270,  9371,  9471,  9572,  9673,  9775,  9877,  9979, 10082, 10185, 10289, 10392, 10496, 10601, 10706, 10811, 10916, 11022, 11128, 11235, 11342, 11449,
11556, 11664, 11772, 11881, 11989, 12099, 12208, 12318, 12428, 12539, 12649, 12760, 12872, 12984, 13096, 13208, 13321, 13434, 13547, 13661, 13775, 13889, 14004, 14119, 14234, 14350, 14466, 14582, 14698, 14815, 14932, 15050,
15167, 15285, 15404, 15522, 15641, 15761, 15880, 16000, 16120, 16241, 16361, 16482, 16604, 16725, 16847, 16970, 17092, 17215, 17338, 17461, 17585, 17709, 17833, 17958, 18083, 18208, 18333, 18459, 18585, 18711, 18838, 18965,
19092, 19219, 19347, 19475, 19603, 19732, 19861, 19990, 20119, 20249, 20379, 20509, 20639, 20770, 20901, 21033, 21164, 21296, 21428, 21561, 21693, 21826, 21959, 22093, 22227, 22361, 22495, 22630, 22764, 22899, 23035, 23170,
23306, 23443, 23579, 23716, 23853, 23990, 24127, 24265, 24403, 24541, 24680, 24819, 24958, 25097, 25236, 25376, 25516, 25657, 25797, 25938, 26079, 26221, 26362, 26504, 26646, 26789, 26931, 27074, 27217, 27361, 27504, 27648,
27792, 27936, 28081, 28226, 28371, 28516, 28662, 28808, 28954, 29100, 29247, 29394, 29541, 29688, 29836, 29984, 30132, 30280, 30429, 30577, 30726, 30876, 31025, 31175, 31325, 31475, 31626, 31776, 31927, 32079, 32230, 32382,
32534, 32686, 32838, 32991, 33144, 33297, 33450, 33604, 33757, 33911, 34066, 34220, 34375, 34530, 34685, 34840, 34996, 35152, 35308, 35464, 35621, 35778, 35935, 36092, 36250, 36407, 36565, 36723, 36882, 37041, 37199, 37358,
37518, 37677, 37837, 37997, 38157, 38318, 38478, 38639, 38800, 38962, 39123, 39285, 39447, 39609, 39772, 39934, 40097, 40260, 40424, 40587, 40751, 40915, 41079, 41244, 41408, 41573, 41738, 41904, 42069, 42235, 42401, 42567,
42733, 42900, 43067, 43234, 43401, 43568, 43736, 43904, 44072, 44240, 44409, 44578, 44747, 44916, 45085, 45255, 45425, 45595, 45765, 45935, 46106, 46277, 46448, 46619, 46791, 46962, 47134, 47306, 47479, 47651, 47824, 47997,
48170, 48344, 48517, 48691, 48865, 49039, 49214, 49388, 49563, 49738, 49913, 50089, 50264, 50440, 50616, 50793, 50969, 51146, 51323, 51500, 51677, 51854, 52032, 52210, 52388, 52566, 52745, 52924, 53103, 53282, 53461, 53640,
53820, 54000, 54180, 54360, 54541, 54722, 54902, 55084, 55265, 55446, 55628, 55810, 55992, 56174, 56357, 56539, 56722, 56905, 57089, 57272, 57456, 57640, 57824, 58008, 58192, 58377, 58562, 58747, 58932, 59118, 59303, 59489,
59675, 59861, 60048, 60234, 60421, 60608, 60795, 60982, 61170, 61358, 61546, 61734, 61922, 62111, 62299, 62488, 62677, 62866, 63056, 63246, 63435, 63625, 63816, 64006, 64197, 64387, 64578, 64770, 64961, 65152, 65344, 65535
	
};

const static u16 bcpatterns[15] = {
	0xFFFF, 0xFFFE, 0xFFFC, 0xFFF8, 0xFFF0, 0xFFE0, 0xFFC0, 0xFF80, 0xFF00, 0xFE00, 0xFC00, 0xF800, 0xF000, 0xE000, 0xC000
};
	
static s16 chorusBuffer[4096];
static s16 delayBuffer[DELAY_BUFFER_SIZE];

#endif