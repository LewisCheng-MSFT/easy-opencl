uint Square(uint x)
{
	return x * x;
}

__kernel void Test(__global uint* output, uint n)
{
	uint i = 1;
	uint x = 3;
	output[0] = 2;
	
	while (i < n)
	{
		bool prime = true;
		uint j;
		for (j = 0; j < i && Square(output[j]) < x; ++j)
		{
			if (x % output[j] == 0)
			{
				prime = false;
				break;
			}
		}
		if (prime)
			output[i++] = x;
		x++;
	}
}