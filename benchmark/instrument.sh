instrument_cmd="node ../../middleware/standalone-instrumentation.js"
#expects a wasm file as first and only argument
function instrument {
	name=`basename $1 .wasm`
	wat=$name.wat
	cp $1 instrumented.0.$name.wasm
	wasm2wat $1 -o $wat
	$instrument_cmd $wat 0
	$instrument_cmd $wat 1
	$instrument_cmd $wat 2
	wat2wasm instrumented.0.$wat -o instrumented.1.$name.wasm
	wat2wasm instrumented.1.$wat -o instrumented.2.$name.wasm
	wat2wasm instrumented.2.$wat -o instrumented.3.$name.wasm
}
