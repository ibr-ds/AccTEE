"use strict";

const importScripts=require('./importscripts.js').importScripts;
const instrumentWAT = require("./instrumentation.js");
const fs = require('fs');
const wast2wasm = require("wast2wasm");

if(process.argv.length < 4) {
	console.log("Need file and optLevel parameters!");
	return;
}

let inputFile = process.argv[2];
let optLevel = process.argv[3];

fs.readFile(inputFile, 'utf8', function (err, data) {
	if(err) {
		return console.log(err);
	}

	let wat = data.split(/\r?\n/);
	instrumentWAT(wat, optLevel);
	wat = wat.join("\n");
	
	fs.writeFile("instrumented." + optLevel + "." + inputFile, wat, function(err) {
		if(err) {
			console.log(err);
		}

		console.log("Printed result to instrumented." + optLevel + "." + inputFile);
	});
});
