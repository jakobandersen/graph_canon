local image = "localhost:5000/jla/base";
local boostArg(v) = "-DBOOST_ROOT=/opt/boost/%s" % v;

local Pipeline(withCoverage, compiler, boost) = {
	kind: "pipeline",
	name: "%s, Boost %s" % [compiler, boost],
	steps: [
		{
			name: "bootstrap",
			image: image,
			commands: [
				"git fetch --tags",
				"git submodule update --init --recursive",
				"./bootstrap.sh",
			],
		},
		{
			name: "build",
			image: image,
			environment: {
				CXX: compiler,
				CXXFLAGS: "-Werror",
			},
			commands: [
				"mkdir build",
				"cd build",
				"cmake ../ -DBUILD_BIN_BENCHMARK=on -DBUILD_BIN_ALL_ALG=on -DBUILD_EXAMPLES=on -DBUILD_TESTING=on -DBUILD_TESTING_SANITIZERS=off %s" % [boostArg(boost)],
				"make",
			],
		},
		{
			name: "install",
			image: image,
			commands: [
				"cd build",
				"make install",
			],
		},
		{
			name: "build-test",
			image: image,
			commands: [
				"cd build",
				"make tests",
			],
		},
		{
			name: "test",
			image: image,
			commands: [
				"cd build",
				"make install",
				"ctest --output-on-failure",
			],
		},
	],
};

[
	Pipeline(boost == "1_83_0" && compiler == "g++-13", compiler, boost)
	for compiler in [
		"g++-9", "g++-10",
		"g++-11", "g++-12",
		"g++-13",
		"clang++-11", "clang++-12",
		"clang++-13", "clang++-14",
		"clang++-15",
	]
	for boost in [
		"1_76_0", "1_77_0", "1_78_0", "1_79_0", "1_80_0", "1_81_0", "1_82_0", "1_83_0",
	]
]
