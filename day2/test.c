#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

void print_n(uint64_t n) {
	printf("%lld", n);
}
int int_len(uint64_t);
int to_str(char *buf, uint64_t n) {
	int len = int_len(n);
	int i = len;
	for (int i = len; i>0; i--) {
		buf[i] = '0' + n % 10;
		n /= 10;

	}
	return len;
}

int int_len(uint64_t n) {
	if (n == 0) return 1;
	return ((int)(log10l((long double)n)+1) + (n < 0 ? 1 : 0));
}

int part_repeating(uint64_t n, int len, int pattern) {
	int part_len = int_len(pattern);
	if (len % part_len != 0) return 0;
	int parts_count = len / part_len;
	int base = pow(10, part_len);
	for (int i = 0; i < (parts_count - 1); i++) {
		int part = (int)(n % base);
		if (part != pattern) return 0;
		n /= base;
	}
	return 1;
}

int any_parts_repeating(uint64_t n) {
	int len = int_len(n);
	for (int i = (len / 2); i > 0; i--) {
		int mask = pow(10, len - i);
		int part = n / (mask);
		int is_repeating = part_repeating(n, len, part);
		if (is_repeating) return 1;
	}
	return 0;
}

void print_bn(uint64_t n) {
	while (n) {
		if (n & 1)
			printf("1");
		else
			printf("0");
		n >>= 1;
	}
}

void dbg(uint64_t n) {
	print_n(n);
	printf(" repeating = %d\n", any_parts_repeating(n));
}

int main(void)
{
	dbg(101);
	dbg(152);
	dbg(11);
	dbg(1010);
	dbg(565656);
	dbg(1188511885);


	return EXIT_SUCCESS;
}
