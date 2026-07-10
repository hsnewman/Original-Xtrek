static char sccsid[] = "@(#)random.c	3.1";
/*
 * The really nice random number generator that came with 4.3 uses
 * too much state information to pass across the network so for our
 * network-wide random generator we just use the old srand/rand
 * combination.  Sorry about the false naming.
 */

static long randx = 1;

srandom(seed)
int seed;
{
  randx = seed;
}

long random()
{
  return((randx = randx * 1103515245 + 12345) & 0x7fffffff);
}

long grandom()
{
  return(randx);
}
