/* static char sccsid[] = "@(#)planets.h	3.1"; */
/*

	Copyright (c) 1986 	Chris Guthrie

Permission to use, copy, modify, and distribute this
software and its documentation for any purpose and without
fee is hereby granted, provided that the above copyright
notice appear in all copies and that both that copyright
notice and this permission notice appear in supporting
documentation.  No representations are made about the
suitability of this software for any purpose.  It is
provided "as is" without express or implied warranty.

*/

struct planet pdata[MAXPLANETS] = {
/*    No  FLG  OWN    X      Y    NAME NL   ARM  INFO  DEAD COUP DRAW TNAME TNL PRIM ORBRAD ORBVAL TYPE ORBANG */
    {  0, PLHOME, FED, 0, 0, "Earth", 0, 50, 0, 0, 0, 0, "", 0, 0, 0, 0, CLASSM, 0 },
    {  1, PLHOME, ROM, 0, 0, "Romulus", 0, 50, 0, 0, 0, 0, "", 0, 0, 0, 0, CLASSM, 0 },
    {  2, PLHOME, KLI, 0, 0, "Klingus", 0, 50, 0, 0, 0, 0, "", 0, 0, 0, 0, CLASSM, 0 },
    {  3, PLHOME, ORI, 0, 0, "Orion", 0, 50, 0, 0, 0, 0, "", 0, 0, 0, 0, CLASSM, 0 },
    {  4, 0, ROM, 0, 0, "Remus", 0, 50, 0, 0, 0, 0, "", 0, 0, 0, 0, DEAD, 0 },
    {  5, 0, KLI, 0, 0, "Leudus", 0, 50, 0, 0, 0, 0, "", 0, 0, 0, 0, DEAD, 0 },
    {  6, 0, ORI, 0, 0, "Oberon", 0, 50, 0, 0, 0, 0, "", 0, 0, 0, 0, DEAD, 0 },
    {  7, 0, FED, 0, 0, "Telos", 0, 50, 0, 0, 0, 0, "", 0, 0, 0, 0, DEAD, 0 },
    {  8, 0, KLI, 0, 0, "Tarsus", 0, 50, 0, 0, 0, 0, "", 0, 0, 0, 0, DEAD, 0 },
    {  9, 0, ORI, 0, 0, "Umbriel", 0, 50, 0, 0, 0, 0, "", 0, 0, 0, 0, DEAD, 0 },
    { 10, 0, FED, 0, 0, "Omega", 0, 50, 0, 0, 0, 0, "", 0, 0, 0, 0, DEAD, 0 },
    { 11, 0, ROM,  0, 0, "Rho", 0, 50, 0, 0, 0, 0, "", 0, 0, 0, 0, DEAD, 0 },
    { 12, 0, SELFRULED, 0, 0, "Janus", 0, 25, 0, 0, 0, 0, "", 0, 0, 0, 0, CLASSM, 0 },
    { 13, 0, SELFRULED, 0, 0, "Elas", 0, 25, 0, 0, 0, 0, "", 0, 0, 0, 0, CLASSM, 0 },
    { 14, 0, SELFRULED, 0, 0, "Sherman", 0, 25, 0, 0, 0, 0, "", 0, 0, 0, 0, CLASSM, 0 },
    { 15, 0, SELFRULED, 0, 0, "Serital", 0, 25, 0, 0, 0, 0, "", 0, 0, 0, 0, DEAD, 0 },
    { 16, 0, SELFRULED, 0, 0, "Cheron", 0, 25, 0, 0, 0, 0, "", 0, 0, 0, 0, DEAD, 0 },
    { 17, 0, SELFRULED, 0, 0, "Dakel", 0, 25, 0, 0, 0, 0, "", 0, 0, 0, 0, CLASSM, 0 },
    { 18, 0, SELFRULED, 0, 0, "Sarac", 0, 25, 0, 0, 0, 0, "", 0, 0, 0, 0, CLASSM, 0 },
    { 19, 0, SELFRULED, 0, 0, "Venar", 0, 25, 0, 0, 0, 0, "", 0, 0, 0, 0, CLASSM, 0 },
    { 20, 0, SELFRULED, 0, 0, "Xidex", 0, 25, 0, 0, 0, 0, "", 0, 0, 0, 0, CLASSM, 0 },
    { 21, 0, SELFRULED, 0, 0, "Oldar", 0, 25, 0, 0, 0, 0, "", 0, 0, 0, 0, DEAD, 0 },
    { 22, 0, SELFRULED, 0, 0, "Eminiar", 0, 25, 0, 0, 0, 0, "", 0, 0, 0, 0, DEAD, 0 },
    { 23, 0, SELFRULED, 0, 0, "Dyneb", 0, 25, 0, 0, 0, 0, "", 0, 0, 0, 0, DEAD, 0 },
    { 24, 0, SELFRULED, 0, 0, "RigelB", 0, 25, 0, 0, 0, 0, "", 0, 0, 0, 0, DEAD, 0 },
    { 25, 0, GOD, 0, 0, "Luna", 0, 0, 0, 0, 0, 0, "", 0, 0, 0, 0, MOON, 0 },
    { 26, PLINVIS, GOD, 0, 0, "Shitface", 256, 30, 0, 0, 0, 0, "", 0, 0, 0, 0, DEAD, 0 },
    { 27, PLINVIS, GOD, 0, 0, "Hell", 0, 128, 0, 0, 0, 0, "", 0, 0, 0, 0, DEAD, 0 },
    { 28, PLINVIS, GOD, 0, 0, "Jinx", 0, 512, 0, 0, 0, 0, "", 0, 0, 0, 0, CLASSM, 0 },
    { 29, 0, GOD, 0, 0, "Sol", 0, 100, 0, 0, 0, 0, "", 0, 0, 0, 0, SUN, 0 },
    { 30, 0, GOD, 0, 0, "Sirius", 0, 100, 0, 0, 0, 0, "", 0, 0, 0, 0, SUN, 0 },
    { 31, 0, GOD, 0, 0, "Kejela", 0, 100, 0, 0, 0, 0, "", 0, 0, 0, 0, SUN, 0 },
    { 32, 0, GOD, 0, 0, "Betelgeuse", 0, 100, 0, 0, 0, 0, "", 0, 0, 0, 0, SUN, 0 },
    { 33, 0, GOD, 0, 0, "Murisak", 0, 100, 0, 0, 0, 0, "", 0, 0, 0, 0, SUN, 0 },
    { 34, PLINVIS, GOD, 0, 0, "Syrinx", 0, 100, 0, 0, 0, 0, "", 0, 0, 0, 0, SUN, 0 },
    { 35, PLINVIS, GOD, 0, 0, "Ghost 1", 0, 0, 0, 0, 0, 0, "", 0, 0, 0, 0, GHOST, 0 },
    { 36, PLINVIS, GOD, 0, 0, "Ghost 2", 0, 0, 0, 0, 0, 0, "", 0, 0, 0, 0, GHOST, 0 },
    { 37, PLINVIS, GOD, 0, 0, "Ghost 3", 0, 0, 0, 0, 0, 0, "", 0, 0, 0, 0, GHOST, 0 },
    { 38, PLINVIS, GOD, 0, 0, "Ghost 4", 0, 0, 0, 0, 0, 0, "", 0, 0, 0, 0, GHOST, 0 },
    { 39, PLINVIS, GOD, 0, 0, "Spare 1", 0, 0, 0, 0, 0, 0, "", 0, 0, 0, 0, GHOST, 0 }
};

#define	NUMCONPLANETS	25	/* Number of planets need to conquer to win */
