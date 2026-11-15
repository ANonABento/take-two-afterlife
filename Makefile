.PHONY: test clean

# On Windows `python3` is usually the Microsoft Store stub; use `python` there.
ifeq ($(OS),Windows_NT)
PYTHON ?= python
else
PYTHON ?= python3
endif

test:
	cmake -S Core -B build -DCMAKE_BUILD_TYPE=Debug
	cmake --build build --config Debug
	ctest --test-dir build --output-on-failure --build-config Debug
	$(PYTHON) -m unittest discover -s Scripts/tests -v

clean:
	rm -rf build
