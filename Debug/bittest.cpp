#include <bits/stdc++.h>

int conbin(char* x)
{
	int res = 0;
	for(int i=7; i>=0; i--)
	{
		res *= 2;
		res += x[i] - 48;
	}
	return res;
}
int main()
{
	char x[8] = {'0','0','0','0','0','0','0','1'};

	int r;
	r = conbin(x);
	printf("\n%d", r);

	return 0;
}