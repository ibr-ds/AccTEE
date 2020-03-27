#include <stdio.h>
#include <emscripten.h>

int main(int argc, char** argv) {
	//printf("Main module loaded!\n");

	EM_ASM(
		FS.mkdir('/working');
		FS.mount(NODEFS, { root: '.' }, '/working');
		FS.chdir('/working');
	);

}
