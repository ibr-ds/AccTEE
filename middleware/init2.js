const wasm = require('./mainModule.js');
const util = require('./util.js');
const fs = require('fs');

let wasmFile;
let paramFile;

var mode = process.env.NODE_ENV;

if(mode != "production")
{
	console.log("init2: starting in development mode. set NODE_ENV=production for measurements!");
} else {
	console.log("init2: starting in production mode. :)");
}

if(process.argv.length == 4) {
	wasmFile = process.argv[2];
	paramFile = process.argv[3];
	memProfile = "default";
} else if(process.argv.length == 5) {
	wasmFile = process.argv[2];
	paramFile = process.argv[3];
	memProfile = process.argv[4];
} else {
	console.log('usage: init2.js <wasm file> <param file> [memory profile]');
	return;
}

const workloadModule = util.loadWorkloadModule(wasmFile);

let mainModule = wasm.InitMainModule();
let imports = util.prepareImports(mainModule, false, memProfile); //true: create new table import

var data = [];

if (fs.existsSync('msieve.dat')) {
	fs.unlinkSync('msieve.dat'); //remove msieve.dat if it exists
}

//set outputResult function
imports.env._outputResult = function(offset, length){
	return;
};

workloadModule.then(module =>{
	fs.readFile(paramFile, 'utf-8', function(err, data){
		var buffer = Buffer.from(data);
		if(err){
			console.log(err);
		}
		let instance;

		instance = new WebAssembly.Instance(module, imports);
		instance.exports.__post_instantiate();

		const workloadBuf = new Uint8Array(buffer);

		// Call function for module to allocate space for workload
		let workloadOffset = instance.exports._allocateWorkload(workloadBuf.length);
		const moduleWorkloadBuf = new Uint8Array(mainModule.memory.buffer, workloadOffset, workloadBuf.length);

		// Copy workload into address space reserved in the module memory
		for(let i=0; i < workloadBuf.length; i++) {
			moduleWorkloadBuf[i] = workloadBuf[i];
		}

		// Call launch function from module
		// This is our equivalent to the 'main' function
		try {
			instance.exports._launch();
		} catch (e) {
			debugger;
			console.log(e.message);
			console.log(e.stack);
		}
		process.removeAllListeners("uncaughtException");
	});
});
