"use strict";

const TextEncoder = require('text-encoding').TextEncoder;
const TextDecoder = require('text-encoding').TextDecoder;
const importScripts=require('./importscripts.js').importScripts;
const include = require('./importscripts.js').include;
let wast2wasm = require("wast2wasm");
const WebSocket = require("ws");
const forge = require('node-forge');
const instrumentWAT = require("./instrumentation.js");

function hexToBytes(hex) {
    for (var bytes = [], c = 0; c < hex.length; c += 2)
    bytes.push(parseInt(hex.substr(c, 2), 16));
    return bytes;
}

let workloadSocket;
let binary;
// TODO get broker address from browser
//let brokerSocket = new WebSocket('ws://192.168.0.3:2015');
let rsaKey;
let aesCipher;
let aesDecipher;
let publicKey;

let workloadHash;
let workload = "";
let workloadFinished = false;

let server = new WebSocket.Server({port: 12345});

console.log('Waiting for connection...');

server.on('connection', function connection(browserSocket) {

    browserSocket.on('message', function incoming(message) {

        console.log("Received frfom browser: " + message);

	let brokerSocket = new WebSocket('ws://10.1.4.223:2015');
	brokerSocket.onopen = function(event) {
	    // When broker socket has finished connecting, immediately send "get" message
	    brokerSocket.send("GET");
	};

	brokerSocket.onmessage = function(event) {
	    // Broker socket sends two messages:

	    if(typeof workloadHash == 'undefined') {
		// 1. Message: Hash of wasm-binary + workload
		workloadHash = event.data;
	    } else {
		// 2. Message: Address of workload server
		workloadSocket = new WebSocket(event.data);
		workloadSocket.binaryType = 'arraybuffer';

		// We immediately connect to the rerceived workload-server address
		// and send our public key for further communication
		// TODO send quote here
		workloadSocket.onopen = function(event) {
		    console.log('Connection established!');
	    
		    /*let rsa = forge.pki.rsa;
		    let keypair = rsa.generateKeyPair({bits: 2048, e: 0x10001, workers: -1});
	    
		    rsaKey = keypair.privateKey;
	    
		    publicKey = forge.pki.publicKeyToPem(keypair.publicKey);
		    workloadSocket.send(publicKey);*/
		    workloadSocket.send(workloadHash);
		};
	    
		workloadSocket.onmessage = function(event) {
		    // The workload-server first sends its symmetric key encrypted with our public key
		    /*if(typeof aesCipher == 'undefined') {
			// Decrypt symmetric key
			var decrypted = rsaKey.decrypt(forge.util.decode64(event.data), 'RSA-OAEP', {
			    md: forge.md.sha256.create(),
			    mgf1: {
				md: forge.md.sha256.create()
			    }
			});
			let decryptedBytes = new Uint8Array(hexToBytes(decrypted));
			let key = Array.prototype.slice.call(decryptedBytes);
	    
			// Generate cipher and decipher from key
			aesCipher = forge.cipher.createCipher('AES-CBC', key);

			let iv = forge.random.getBytesSync(16);
			let ivHex = new forge.util.ByteBuffer(iv).toHex();
	    
			aesCipher.start({iv: iv});
			aesDecipher = forge.cipher.createDecipher('AES-CBC', key);
	    
			// Encrypt binary + workload hash from broker and send it to workload server.
			// It will use this to identify the correct workload for us.
			aesCipher.update(new forge.util.ByteBuffer(workloadHash));
			aesCipher.finish();
	    
			let encryptedBytes = aesCipher.output.getBytes();
			let encrypted = forge.util.encode64(encryptedBytes);
			encrypted = ivHex + encrypted;
			workloadSocket.send(encrypted);

		    } else {*/
			// After the workload-server sent its symmetric key and received the workload hash, it sends
			// the wasm-binary and later the workload for us to compute.

			// Decode and decrypt message
			/*let ivString = decoded.substring(0, 32);
			let encrypted = decoded.substring(32);
	    
			let iv = Array.prototype.slice.call(new Uint8Array(hexToBytes(ivString)));
			aesDecipher.start({iv: iv});
			aesDecipher.update(new forge.util.ByteBuffer(encrypted));
			let result = aesDecipher.finish();
			if(result == false) {
			    console.log("Decryption failed!");
			    return;
			}*/
	    
			if(typeof binary == 'undefined') {
			    // First the binary...
			    /*decrypted = aesDecipher.output.toHex();
			    let binaryArray = hexToBytes(decrypted);
			    binaryArray = binaryArray.slice(0, binaryArray.length);
			    binary = new Uint8Array(binaryArray).buffer;*/
			    //let decoded = forge.util.decode64(event.data);
			    //let binaryArray = hexToBytes(decoded);
			    //binaryArray = binaryArray.slice(0, binaryArray.length);
			    //binary = new Uint8Array(binaryArray).buffer;
			    binary = event.data;
			} else {
			    //let workload = event.data;
			    // ... then the workload.
			    //let workload = aesDecipher.output.getBytes();
			    //workload = forge.util.decode64(workload);
			    /*if(decoded !== "END") {
				workload += decoded;  
				console.log(workload.length);
				return;                  
			    }*/
			    /*if(event.data !== "END") {
				workload += event.data;  
				console.log(workload.length);
				return;                  
			    }*/

			    // Convert binary and workload to binary string so their hash can be computed.
			    /*let binaryBuffer = Array.prototype.slice.call(new Uint8Array(binary));
			    let workloadBuffer = new TextEncoder().encode(workload);
			    let workloadBinaryBuffer = new Uint8Array(workloadBuffer.length + binaryBuffer.length);
			    workloadBinaryBuffer.set(workloadBuffer, 0);
			    workloadBinaryBuffer.set(binaryBuffer, workloadBuffer.length);
			    workloadBinaryBuffer = forge.util.binary.raw.encode(workloadBinaryBuffer);    

			    // Verify that hash of workload+binary match the given workload and binary
			    let md = forge.md.sha256.create();
			    md.update(workloadBinaryBuffer);
			    if(md.digest().toHex().toUpperCase() != workloadHash) {
				console.log("Received hash does not match hash of workload and binary!");
				return;
			    }*/
			    wast2wasm.wasm2wat(binary).then(wast => {

				console.log("wasm2wast finished");
				// "Instrument" the generated WAT-code, i.e. insert instructions to count the executed instructions.
				//let wast = new TextDecoder().decode(binary);
				wast = wast.split(/\r?\n/);
				instrumentWAT(wast);
				wast = wast.join("\n");
				console.log("Instrumentation finished");
				   
				// Recompile the instrumented WAT-code to WASM
				wast2wasm.wat2wasm(wast, true).then(output => {
				    console.log("wast2wasm finished");
				    binary = output.buffer;
				    wast = null;
				    wast2wasm = null;
				    gc();
						    
				    // Now that we verified the binary and workload are correct and instrumented, we first
				    // initialize our main module and then compile and run the given binary.
				    let WASM = require("./mainModule.js");
				    let Module = WASM.InitMainModule();
				    //WASM.RunWebassembly(binary, workload, brokerSocket, rsaKey, publicKey, aesCipher, workloadHash, Module);
				    WASM.RunWebassembly(binary, event.data, browserSocket, rsaKey, publicKey, aesCipher, workloadHash, Module);
				});
			    });
			}
		    //}
		};
	    }
	};
    });
});
