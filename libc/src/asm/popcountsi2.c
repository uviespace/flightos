/* from Sean Eron Anderson's Bit Twiddling Hacks collection */
int __popcountsi2(int a)
{
	a = a - ((a >> 1) & 0x55555555);
	a = (a & 0x33333333) + ((a >> 2) & 0x33333333);
	return (((a + (a >> 4)) & 0xF0F0F0F) * 0x1010101) >> 24;
}
