// Use javascript libraries in node.js
// importScripts is not defined in node.js. This can be annoying if you want to use standard JS libraries in global scope
// In particular, a library that drops lots of useful functions into global scope may not have a node.js maintained equivalent
// 
// Usage
// importScripts
// Use this method if you want to address the library as it were a file local to a webworker. Execution path is relative to where node.js is executed from
// importScripts=require('importScripts').importScripts;
// importScripts('library.js')
//
// include
// Use this method if you want to address the library from an arbitrary location
// include=require('importScripts').include;

fs=require('fs');
vm=require('vm');// vm must be in the global context to work properly


function include(filename){
	var code = fs.readFileSync(filename, 'utf-8');
    vm.runInThisContext(code, filename);
}

function importScripts(filename){
	filename='./'+filename;
	include(filename);
}

exports.importScripts=importScripts;
exports.include=include;