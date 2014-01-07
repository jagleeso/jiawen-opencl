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

PYTHON_SRC=src/python
%: %.jinja $(PYTHON_SRC)/render.py $(PYTHON_SRC)/config.py
	$(PYTHON_SRC)/render.py $<

templates: jni/coalesce.cl
.PHONY: templates
