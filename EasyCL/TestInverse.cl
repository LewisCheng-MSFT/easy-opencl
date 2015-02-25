__kernel void Test(__global uchar4* output, uint width)
{
	uint gi = get_global_id(0);
	uint gj = get_global_id(1);
	// uint li = get_local_id(0);
	// uint lj = get_local_id(1);
	
	uint index = gj * width + gi;
	output[index].x = 255 - output[index].x;
	output[index].y = 255 - output[index].y;
	output[index].z = 255 - output[index].z;
}