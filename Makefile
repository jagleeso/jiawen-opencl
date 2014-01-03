all:
	ndk-build

opencl_aes:
	ndk-build PROGRAM=opencl_aes
cpu-aes:
	ndk-build PROGRAM=cpu-aes
example-aes:
	ndk-build PROGRAM=example-aes
coalesce:
	ndk-build PROGRAM=coalesce
