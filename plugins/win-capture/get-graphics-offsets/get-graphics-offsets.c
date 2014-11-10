#define _CRT_SECURE_NO_WARNINGS
#include <inttypes.h>
#include <stdio.h>
#include <windows.h>
#include "get-graphics-offsets.h"

int main(int argc, char *argv[])
{
	struct d3d8_offsets d3d8 = {0};
	struct d3d9_offsets d3d9 = {0};
	struct dxgi_offsets dxgi = {0};

	WNDCLASSA wc     = {0};
	wc.style         = CS_OWNDC;
	wc.hInstance     = GetModuleHandleA(NULL);
	wc.lpfnWndProc   = (WNDPROC)DefWindowProcA;
	wc.lpszClassName = DUMMY_WNDCLASS;

	if (!RegisterClassA(&wc)) {
		printf("failed to register '%s'\n", DUMMY_WNDCLASS);
		return -1;
	}

	get_d3d9_offsets(&d3d9);
	get_d3d8_offsets(&d3d8);
	get_dxgi_offsets(&dxgi);

	printf("[d3d8]\n");
	printf("present=0x%"PRIxPTR"\n", d3d8.present);
	printf("reset=0x%"PRIxPTR"\n", d3d8.reset);
	printf("[d3d9]\n");
	printf("present=0x%"PRIxPTR"\n", d3d9.present);
	printf("present_ex=0x%"PRIxPTR"\n", d3d9.present_ex);
	printf("present_swap=0x%"PRIxPTR"\n", d3d9.present_swap);
	printf("reset=0x%"PRIxPTR"\n", d3d9.reset);
	printf("reset_ex=0x%"PRIxPTR"\n", d3d9.reset_ex);
	printf("[dxgi]\n");
	printf("present=0x%"PRIxPTR"\n", dxgi.present);
	printf("resize=0x%"PRIxPTR"\n", dxgi.resize);

	(void)argc;
	(void)argv;
	return 0;
}
