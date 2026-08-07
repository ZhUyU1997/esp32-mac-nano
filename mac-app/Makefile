TOOLCHAIN = /Retro68-build/toolchain/m68k-apple-macos/cmake/retro68.toolchain.cmake
RUN       = docker run --rm -v "$$(pwd)/..:/workspace" -w /workspace ghcr.io/autc04/retro68:latest
APPS      = CounterApp DashApp

all: $(APPS)

$(APPS):
	$(RUN) sh -c 'cd /workspace/mac-app/$@ && rm -rf build && mkdir build && cd build && cmake .. -DCMAKE_TOOLCHAIN_FILE=$(TOOLCHAIN) && make'

clean:
	$(RUN) sh -c 'for d in $(APPS); do rm -rf /workspace/mac-app/$$d/build; done'

shell:
	$(RUN) /bin/bash

.PHONY: all $(APPS) clean shell
