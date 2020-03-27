const fs = require('fs');

exports.prepareImports = function(mainModule, new_table=false, profile="default") {
	let imports = {};

	let tempDoublePtr = 0;
	imports.env = mainModule;
	imports.env.tempDoublePtr = tempDoublePtr;
	imports.env.pow = Math.pow;
	imports.env.g$___dso_handle = function() {return mainModule["___dso_handle"];};

	imports.env.getTempRet0 = mainModule.asmLibraryArg.getTempRet0;
	imports.env.setTempRet0 = mainModule.asmLibraryArg.setTempRet0;

	switch(profile) {
		case "default":
			imports.env.memoryBase = imports.env.memoryBase || 0;
			break;
		case "pc":
			imports.env.memoryBase = 131072; // Works for pc
			break;
		case "subset":
			imports.env.memoryBase = 131072;
			break;
		case "darknet":
			imports.env.memoryBase = 524288; // Works for darknet
			break;
		case "msieve":
			imports.env.memoryBase = 262144;
			break;
		default:
			throw "unknown memory profile!"

	}
	//imports.env.memoryBase = imports.env.memoryBase || 0;
	//imports.env.memoryBase = 131072; // Works for pc
	//imports.env.memoryBase = 524288; // Works for darknet
	//imports.env.memoryBase = 16777216; //16
	//imports.env.memoryBase = 33554432; //32
	//imports.env.memoryBase = 67108864; //64

	imports.env.tableBase = 1024;


	if (!imports.env.memory) {
		imports.env.memory = mainModule.wasmMemory;
	}

	if(new_table) {
		if (!imports.env.table) {
			imports.env.table = new WebAssembly.Table({ initial: 2048, element: "anyfunc" });
		}
	} else {
		imports.env.table = mainModule["wasmTable"];
	}

	return imports;
}

exports.loadWorkloadModule = function(wasmFile) {
	const workloadBinary = new Uint8Array(fs.readFileSync(wasmFile)).buffer;
	return WebAssembly.compile(workloadBinary);
}
