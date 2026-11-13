.PHONY: test clean

test:
	cmake -S Core -B build -DCMAKE_BUILD_TYPE=Debug
	cmake --build build --config Debug
	ctest --test-dir build --output-on-failure --build-config Debug
	python3 -m unittest discover -s Scripts/tests -v

clean:
	rm -rf build
