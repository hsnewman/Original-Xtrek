static char sccsid[] = "@(#)itoa.c	3.1";
itoa(n, len, buf)
int n, len;
char *buf;
{
  int i;
  char sign = ' ';

  if (0 > n) {
    n = -n;
    sign = '-';
  }
  for (i = len; i > 0; i--) {
    if (n > 0 || len == i) {
      buf[i-1] = '0' + (n % 10);
      n = n / 10;
    } else {
      buf[i-1] = sign;
      sign = ' ';
    }
  }
}

