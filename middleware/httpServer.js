const http = require('http');
const wasm = require('./mainModule.js');
const util = require('./util.js');

let wasmFile;
let port;

var mode = process.env.NODE_ENV;
var req_count=0;
var last_req_count=0;
var print_status_called=0;
var timeout=1;//seconds
var bytecounter = 0;

if(mode != "production")
{
	console.log("starting in development mode. set NODE_ENV=production for measurements!");
	console.log("#runtime(s),req/s,rss(MB),heapTotal(MB),heapUsed(MB)");
	setTimeout(print_status, timeout*1000);
} else {
	console.log("starting in production mode. :)");
}

if(process.argv.length > 2) {
	wasmFile = process.argv[2];
	port = process.argv[3];
} else {
	console.log('usage: httpServer.js <wasm file> <port>');
	return;
}

const workloadModule = util.loadWorkloadModule(wasmFile);

let mainModule = wasm.InitMainModule();
let imports = util.prepareImports(mainModule);

http.createServer(function(req, res) {
	var data = [];

	//set outputResult function
	imports.env._outputResult = function(offset, length){
		res.end(Buffer.from(mainModule.wasmMemory.buffer, offset, length));
	};

	//fill data array
	req.on('data', chunk => {
		data.push(chunk);
	});

	//when incoming request full
	req.on('end', () => {
		workloadModule.then(module =>{
			var buffer = Buffer.concat(data);
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
			//bytecounter += workloadBuf.length;

			// Call launch function from module
			// This is our equivalent to the 'main' function
			try {
				debugger;
				instance.exports._launch();
			} catch (e) {
				debugger;
				console.log(e.message);
				console.log(e.stack);
			}
			process.removeAllListeners("uncaughtException");
			req_count++;
		});
	});
}).listen(port);

function print_status() {
	print_status_called++;
	reqs = (req_count - last_req_count) / timeout;
	const used = process.memoryUsage();
	var mem_str = "";
	for (let key in used) {
		  mem_str+=`${Math.round(used[key] / 1024 / 1024 * 100) / 100},`;
	}
	console.log(print_status_called * timeout + "," + reqs + "," + mem_str, "bytes:" + bytecounter);
	last_req_count = req_count;
	setTimeout(print_status, timeout*1000);
}
