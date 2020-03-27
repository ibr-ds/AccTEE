const union = require('underscore').union;

//instruction weights
var dict = {
	"f32.load": 1,
	"f32.store": 1,
	"f64.load": 1,
	"f64.store": 1,
	"i32.load16_s": 1,
	"i32.load16_u": 1,
	"i32.load8_s": 1,
	"i32.load8_u": 1,
	"i32.load": 1,
	"i32.store16": 1,
	"i32.store8": 1,
	"i32.store": 1,
	"i64.load16_s": 1,
	"i64.load16_u": 1,
	"i64.load32_s": 1,
	"i64.load32_u": 1,
	"i64.load8_s": 1,
	"i64.load8_u": 1,
	"i64.load": 1,
	"i64.store16": 1,
	"i64.store32": 1,
	"i64.store8": 1,
	"i64.store": 1,
}
//0: no optimisation, instruction added at the enf of every basic block
//1: flow-based optismiation
//2: loop-based optimisation
const OPTIMISATION = 0;

/* This class is used to keep track of all previous control flow elements
 * when using nesting (e.g. when a function has a loop, we use this to keep the
 * current instruction count etc. of the funcion while processing the loop).
 *
 * */
class CFConstruct {
	constructor(type, start) {
		// Tyoe of cfConstruct (if/else/loop/function/block)
		this.type = type;

		// Instructions for this construct since last counter incrementation
		this.instructions = 0;

		// Labels that are jumped to for this and nested constructs
		this.branches = [];

		this.containsReturn = false;

		// Last line before beginning of current nested construct
		this.lineIndex = -1;

		// Labels outside of this construct
		this.outerLabels = [];

		this.label = -1;
		this.startLine = start;
		// Loop only: This checks whether the loop code has been popped from the stack
		this.stoppedRecording = false;

		// (If/else only) labels that are jumped to EXCLUDING nested constructs
		this.nonNestedBranches = [];
		this.nonNestedReturn = false;
	}
}

// Returns whether a line should be regarded for instruction counting.
function disregardLine(line) {
	if(line.startsWith("(module")
			|| line.startsWith("(type")
			|| line.startsWith("(import")
			|| line.startsWith("(global")
			|| line.startsWith("(export")
			|| line.startsWith("(elem")
			|| line.startsWith("(data"))
		return true;
	else
		return false;
}

// Returns the type of control flow construct in the given line
// or an empty string if none exists
function getCFConstructType(line) {
	if(line.startsWith("(func"))
		return "function";
	if(line.startsWith("block"))
		return "block";
	if(line.startsWith("if"))
		return "conditional";
	if(line.startsWith("loop"))
		return "loop";

	return "";
}

// Returns whether the current line is a (potentially) branching operation
function isBranchOperation(line) {
	if(line.startsWith("br"))
		return true;
	else
		return false;
}

function isLocalDefinition(line) {
	if(line.startsWith("(local"))
		return true;
	else
		return false;
}

// This function inserts the required webassembly instructions to increment
// the counter by the number of instructions executed.
// It returns the number of lines added.
function insertInstructionCount(lines, lineIndex, currentCFConstruct, globalVariable, stackSize) {
	// First check if there are actually instructions executed.
	if(currentCFConstruct.instructions === 0)
		return 0;

	// Calculate correct indentation
	let indentation = "    ";

	for(let j = 0; j < stackSize; j++) {
		indentation += "  ";
	}

	// Insert the lines for counter instrucion (we always insert at the same index, to the lines 
	// must be inserted in reverse)
	let newLine = indentation + "set_global " + globalVariable;
	lines.splice(lineIndex, 0, newLine);

	newLine = indentation + "i64.add";
	lines.splice(lineIndex, 0, newLine);

	newLine = indentation + "i64.const " + currentCFConstruct.instructions;
	lines.splice(lineIndex, 0, newLine);

	newLine = indentation + "get_global " + globalVariable;
	lines.splice(lineIndex, 0, newLine);

	return 4;
}

// This function sets the value of a temporary variable to the value of the one indicated.
// It is used to save the value of a loop iterator before the loop execution.
function insertIteratorTmpVariable(lines, lineIndex, variable, variableType, tmpVariable, stackSize) {
	// Calculate correct indentation
	let indentation = "    ";

	for(let j = 0; j < stackSize; j++) {
		indentation += "  ";
	}

	let addedLines = 2;

	let newLine = indentation + "set_local " + tmpVariable;
	lines.splice(lineIndex, 0, newLine);

	// If original variable is i32, cast its value to i64
	if(variableType === "i32") {
		newLine = indentation + "i64.extend_s/i32";
		lines.splice(lineIndex, 0, newLine);
		addedLines++;
	}

	newLine = indentation + "get_local " + variable;
	lines.splice(lineIndex, 0, newLine);

	return addedLines;
}

// This function creates a new local variable for saving iterator values before loop executions.
function createTmpVariable(lines, localLineIndex) {
	let newLine;
	let addedLines = 0;
	// First check if local variables are defined for this function.
	if(!lines[localLineIndex].trim().startsWith("(local")) {
		// If no local variables exist yet, we first inject the line for their declaration.
		newLine = "      (local )";
		lines.splice(localLineIndex, 0, newLine);
		addedLines++;
	}

	// 
	let variableLine = lines[localLineIndex];
	let endIndex = variableLine.indexOf(")");
	newLine = variableLine.substring(0, endIndex) + " i64" + variableLine.substring(endIndex);

	lines[localLineIndex] = newLine;

	return addedLines;
}

function insertIteratorInstrumentation(lines, lineIndex, iterator, iteratorType, tmpVariable, globalVariable, incrementFactor, operation, instructions, stackSize) {
	// Calculate correct indentation
	let indentation = "    ";

	for(let j = 0; j < stackSize; j++) {
		indentation += "  ";
	}

	let addedLines = 0;
	// Inject instructions to increment the counter depending on the iterations of the loop (counter += #iterations * instructions per iteration).
	// This looks slightly different depending on the type of the iterator (i32/i64) and operation (iterator incremented/decremented).
	// As we inject the code into the same line, we start from the last line.
	
	// First the counter incrementation
	let newLine = indentation + "set_global " + globalVariable;
	lines.splice(lineIndex, 0, newLine);

	newLine = indentation + "i64.add";
	lines.splice(lineIndex, 0, newLine);

	newLine = indentation + "get_global " + globalVariable;
	lines.splice(lineIndex, 0, newLine);

	// Now we calculate the iterations performed, based on the iterator value before and after the loop, as well as the incrementation factor.
	newLine = indentation + "i64.mul";
	lines.splice(lineIndex, 0, newLine);

	newLine = indentation + "i64.const " + instructions;
	lines.splice(lineIndex, 0, newLine);

	// If the increment factor is 1 (i++/i--) we don't need to divide
	if(incrementFactor !== 1 && incrementFactor !== -1) {
		newLine = indentation + "i64.div_u";
		lines.splice(lineIndex, 0, newLine);

		newLine = indentation + "i64.const " + incrementFactor;
		lines.splice(lineIndex, 0, newLine);
		addedLines += 2;
	}

	newLine = indentation + "i64.sub";
	lines.splice(lineIndex, 0, newLine);

	// Depending on the iterator operation (i++ or i--), load the values of pre-/post-loop in different order for difference comutation.
	// Additionally cast the iterator to a 64-bit integer if it is not already
	if(operation === "add") {
		newLine = indentation + "get_local " + tmpVariable;
		lines.splice(lineIndex, 0, newLine);

		if(iteratorType === "i32") {
			newLine = indentation + "i64.extend_s/i32";
			lines.splice(lineIndex, 0, newLine);
			addedLines++;
		}

		newLine = indentation + "get_local " + iterator;
		lines.splice(lineIndex, 0, newLine);
	} else {
		if(iteratorType === "i32") {
			newLine = indentation + "i64.extend_s/i32";
			lines.splice(lineIndex, 0, newLine);
			addedLines++;
		}

		newLine = indentation + "get_local " + iterator;
		lines.splice(lineIndex, 0, newLine);

		newLine = indentation + "get_local " + tmpVariable;
		lines.splice(lineIndex, 0, newLine);
	}

	return addedLines + 8;
}

// This function inserts a webassembly function for returning the current counter
// value and exports it.
function insertReturnCounterFunction(lines, lineIndex, functionIndex, globalVariable) {
	let indentation = "  ";

	// Insert the function line by line at the given index
	// The code therefore have to be in reverse order

	let newLine = indentation + "(export \"getCounter\" (func " + functionIndex +
		"))";
	lines.splice(lineIndex, 0, newLine);

	newLine = indentation + "  f64.convert_s/i64)";
	lines.splice(lineIndex, 0, newLine);

	newLine = indentation + "  get_global " + globalVariable;
	lines.splice(lineIndex, 0, newLine);

	newLine = indentation + "(func (;" + functionIndex + ";) (result f64)";
	lines.splice(lineIndex, 0, newLine);
}

// This function checks whether a CFConstruct has any outside branches, meaning whether any jumps outside
// of this block (including returns).
function checkForOutsideBranches(CFConstruct) {
	if(CFConstruct.containsReturn)
		return true;
	else {
		// Check if one of the labels jumps outside the current construct
		for(let branchLabel of CFConstruct.branches) {
			// Differentiate between loops and other blocks, as branches to the label of a loop jump to the beginning
			// of the loop, whereas with other blocks they jump to the end
			if(CFConstruct.outerLabels.includes(branchLabel) 
			    || (CFConstruct.type !== "loop" && CFConstruct.label === branchLabel)) {
				return true;
			}
		}
	}
	return false;
}

// This function checks if the current loop jumps outside.
function checkLoopForOutsideBranches(CFConstruct) {
	if(CFConstruct.containsReturn)
		return true;
	else {
		// Check if one of the labels jumps outside the current construct
		for(let branchLabel of CFConstruct.branches) {
			if(branchLabel !== CFConstruct.label && CFConstruct.outerLabels.includes(branchLabel)) {
				return true;
			}
		}
	}
	return false;
}

// This function checks whether a CFConstruct has any outside branches (disregarding nested constructs), 
// meaning whether any jumps outside of this block (including returns).
function checkForImmediateReturn(CFConstruct) {
	if(CFConstruct.nonNestedReturn)
		return true;
	else {
		// Check if one of the labels jumps outside the current construct
		for(let branchLabel of CFConstruct.nonNestedBranches) {
			if(CFConstruct.outerLabels.includes(branchLabel)) {
				return true;
			}
		}
	}
	return false;
}

/*  This function transforms the given WAST code (as an array of lines) to
 *  add a counter for counting instructions executed. The counter is implemented
 *  as a global variable and can be reviewed from JS via an additional function
 *  "getCounter".
 *  This function directly modifies the given array.
 *
 * */

function instrumentWAT (lines, level) {
	let OPTIMISATION = level;
	let stack = new Array();
	let currentCFConstruct;
	let loopCodeStack = new Array();
	let localIndex;
	let localLineIndex ;

	// For some reason lines contains an empty line at the end.
	// => Remove it.
	lines.pop();

	// Check the wast for global variables used.
	// Then select a new unused global variable for the instruction counter.
	// Additionally do the same for function identifiers used to select an
	// identifier for the function returning the value of the global variable.
	let maxGlobalVariable = -1, maxFunctionIndex = -1;
	for(let line of lines) {
		let currentLine = line.trim();

		// TODO check if any set_global/get_global are called
		let globalPosition = currentLine.indexOf("global (;");
		if(globalPosition !== -1) {
			currentLine = currentLine.substring(globalPosition + 9);
			let currentGlobal = parseInt(currentLine);
			if(currentGlobal > maxGlobalVariable)
				maxGlobalVariable = currentGlobal;
		} else {
			let functionPosition = currentLine.indexOf("(func (;");
			if(functionPosition === -1)
				continue;

			currentLine = currentLine.substring(functionPosition + 8);
			let currentFunctionIndex = parseInt(currentLine);

			if(currentFunctionIndex > maxFunctionIndex)
				maxFunctionIndex = currentFunctionIndex;
		}
	}
	maxGlobalVariable++;
	maxFunctionIndex++;

	// Add global variable definition to end of file
	// First remove ")" for module end from last line
	let lastLine = lines[lines.length - 1];
	lastLine = lastLine.substring(0, lastLine.length - 1);
	lines[lines.length - 1] = lastLine;

	// Then append global variable definition (with additional ")" )
	lastLine = "  (global (;" + maxGlobalVariable + ";) (mut i64) (i64.const 0)))";
	lines[lines.length] = lastLine;

	// Now parse the WAST and add neccessary counter operations
	let i = 0, length = lines.length;

	while(i < length) {
		let currentLine = lines[i].trim();

		// Skip lines until we find an instruction or beginning/end of
		// a control flow construct
		if(disregardLine(currentLine)) {
			i++;
			continue;
		}

		let thisInstruction = currentLine.split(" ")[0].replace(')','');
		let currentCosts = dict[thisInstruction];
		if( currentCosts == undefined) {
			currentCosts = 0;
		}


		// If we are currently inside a loop, add current line for later analysis.
		for(let code of loopCodeStack) {
			code.push(currentLine);
		}

		let type = getCFConstructType(currentLine);
		if(type !== "") {
			// We have the beginning of a new control flow construct.

			// Get the label of the current construct
			let labelIndex = currentLine.indexOf("label = @");
			let label = -1;
			if(labelIndex !== -1) {
				label = parseInt(currentLine.substring(labelIndex + 9));
			}

			let outerLabels = [];

			// Save current control flow construct on stack (if there is one)
			if(typeof currentCFConstruct !== 'undefined') {
				currentCFConstruct.lineIndex = i;

				// Add all outer labels + the label of the construct added to the
				// stack to the outer labels of the new construct
				outerLabels = currentCFConstruct.outerLabels.slice();

				if(currentCFConstruct.label !== -1)
					outerLabels.push(currentCFConstruct.label);

				stack.push(currentCFConstruct);
			}

			// Create new control flow construct
			currentCFConstruct = new CFConstruct(type, i);
			currentCFConstruct.outerLabels = outerLabels;
			currentCFConstruct.label = label;

			// If construct is a loop, start recording code lines for later iterator analysis.
			if(type === "loop") {
				loopCodeStack.push(new Array());
			}

			// Check function parameters. As each parameter is copied to a local variable the instruction counter
			// needs to be incremented for each parameter. Additionally we need to know this for loop optimisations.
			// TODO modify counter according to parameter count
			if(type === "function") {

				if(stack.length > 0)
					debugger;

				// Parse the number of parameters.
				let paramIndex = currentLine.indexOf("param");
				if(paramIndex === -1) {
					localIndex = 0;
				} else {
					let paramString = currentLine.substring(paramIndex);
					paramString = paramString.substring(6, paramString.indexOf(")"));
					localIndex = paramString.split(" ").length;
				}

				// The next line is where local variables must be declared.
				// We remember that line in case we need to add a local variable;
				localLineIndex = i + 1;
			}

			i++;
			continue;
		}

		// Check if current line is a branch operation
		if(isBranchOperation(currentLine)) {
			// Get label of branch operation and add it to list of branches.
			// TODO br_table (switch operation) has lists instead of a single jump indexed by the stack value
			let branchLabelPosition = currentLine.indexOf("(;@");
			let branchLabel = parseInt(currentLine.substring(branchLabelPosition + 3));

			let finalBranch = false;
			let outsideBranches = false;
			let loopCode;
			if(currentCFConstruct.type === "loop" && branchLabel === currentCFConstruct.label && !checkLoopForOutsideBranches(currentCFConstruct)) {
				for(let j = i+1; i < lines.length; j++) {
					loopLine = lines[j].trim();
					if(loopLine.startsWith("end")) {
						finalBranch = true;
						loopCode = loopCodeStack.pop();
						currentCFConstruct.stoppedRecording = true;
						break;
					} else if(loopLine.startsWith("block") || loopLine.startsWith("if") || loopLine.startsWith("br")) {
						break;
					}
				}
			}
			if(finalBranch && OPTIMISATION >= 2) {
				// Drop last "end" line from code
				loopCode.pop();

				// Find all local variables that are accessed exactly once (set_local or tee_local instructions)
				// Variables set in if/else/nested loop blocks are ignored as they are not reliable iterators.
				// At the same time check for "continues", i.e. more than one branch to the beginning of the loop.
				let found = [];
				let blacklist = [];
				let conditionalBlocks = 0;
				let blockStack = [];
				let continues = 0;

				for(let loopLine of loopCode) {
					if(loopLine.startsWith("set_local") || loopLine.startsWith("tee_local")) {
						let variable = loopLine.split(" ")[1];
						if(blacklist.indexOf(variable) > -1) {
							continue;
						} else if(found.indexOf(variable) > -1 || conditionalBlocks > 0) {
							blacklist.push(variable);
							if(found.indexOf(variable) > -1)
								found.splice(found.indexOf(variable), 1);
						} else {
							found.push(variable);
						}
					} else if(loopLine.startsWith("if") || loopLine.startsWith("loop")) {
						conditionalBlocks++;
						blockStack.push(true);
					} else if(loopLine.startsWith("block")) {
						blockStack.push(false);
					} else if(loopLine.startsWith("end")) {
						if(blockStack.pop()) {
							conditionalBlocks--;
						}
					}
				}

				// Check if one of the iterator candidates is an actual iterator.
				// For this we check which variables and/or constant values influence the variable (see also "backward slicing").
				// If the variable is depenent on exaclty itself and a constant value it is an iterator.
				let iteratorCandidate = -1;
				let vStack;
				let constValue;
				let dependentVariables;
				let tmpJ;
				let operation;
				let iteratorType;
				for(let j = loopCode.length-1; j >= 0; j--) {
					let loopLine = loopCode[j];
					// Check if a candidate is written.
					if(iteratorCandidate === -1 && (loopLine.startsWith("set_local") || loopLine.startsWith("tee_local"))) {
						let variable = loopLine.split(" ")[1];
						if(found.indexOf(variable) > -1) {
							tmpJ = j - 1;
							iteratorCandidate = variable;
							vStack = 1;
							constValue = "none";
							operation = "none";
							dependentVariables = [];
						}
					} else if(iteratorCandidate === -1) {
						continue;
					} else {
						// In this case we are checking the current line for dedpendencies of the iterator candidate.
						// For this we keep a virtual stack (vStack), which keeps track of the number of values which
						// need to be pushed to the stack prior to the current instruction. It starts with 1
						// due to set/tee_local being called and is modified depending on the number of values pushed
						// to the stack vs. the number consumed from the stack. (e.g. i32.add consumes two values and
						// pushes one => one additional value needs to be pushed to the stack beforehand)
						// Once the value of vStack reaches 0, all instructions influencing the iterator candidate are
						// identified.

						if(loopLine.startsWith("tee_local")) {
							continue;
						} else if(loopLine.startsWith("i32.const") || loopLine.startsWith("i64.const")) {
							// For now we look only at the case where exactly one variable and one constant is involved.
							// If the iterator depends on more than one constant, skip it.
							if(constValue !== "none") {
								iteratorCandidate = -1;
								j = tmpJ;
								continue;
							} else {
								vStack--;
								constValue = parseInt(loopLine.split(" ")[1]);
								if(constValue === 0) {
									iteratorCandidate = -1;
									j = tmpJ;
									continue;
								}
								if(loopLine.startsWith("i32"))
									iteratorType = "i32";
								else
									iteratorType = "i64";
							}
						} else if(loopLine.startsWith("i32.add") || loopLine.startsWith("i64.add")) {
							// For now we only allow one add or sub operation.
							if(operation !== "none") {
								iteratorCandidate = -1;
								j = tmpJ;
								continue;
							} else {
								vStack++;
								operation = "add";
							}
						} else if(loopLine.startsWith("i32.sub") || loopLine.startsWith("i64.sub")) {
							if(operation !== "none") {
								iteratorCandidate = -1;
								j = tmpJ;
								continue;
							} else {
								vStack++;
								operation = "sub";
							}
						} else if(loopLine.startsWith("get_local")) {
							// If a variable other than the iterator candidate is accessed, stop.
							if(loopLine.split(" ")[1] !== iteratorCandidate) {
								iteratorCandidate = -1;
								j = tmpJ;
								continue;
							} else {
								vStack--;
							}
						} else {
							// Some operation we currently do not support
							iteratorCandidate = -1;
							j = tmpJ;
							continue;
						}

						// If we reaches a vStack of 0, we found a reliable iterator.
						if(vStack === 0 && operation !== "none" && constValue !== "none") {
							// For simplicity we change the const value to always be positive
							// (e.g. add -1 -> sub 1)
							if(constValue < 0) {
								constValue = constValue * -1;
								if(operation === "add") {
									operation = "sub";
								} else {
									operation = "add";
								}
							}

							// Inject code for a new temp variable to save the iterator value before the loop execution.
							let addedLines = createTmpVariable(lines, localLineIndex);

							// Now inject code to save the iterator value to the temp variable before the loop.
							addedLines += insertIteratorTmpVariable(lines, currentCFConstruct.startLine, iteratorCandidate, iteratorType, localIndex, stack.length - 1);
							
							// Finally inject code to increment the counter after the loop finished.
							addedLines += insertIteratorInstrumentation(lines, i+1+addedLines , iteratorCandidate, iteratorType, localIndex, 
								maxGlobalVariable, constValue, operation, currentCFConstruct.instructions, stack.length);

							currentCFConstruct.instructions = 0;
							length += addedLines;
							i += addedLines;
							localIndex++;

							break;
						}
					}
				}
				// Also, if no iterator was found we need to increment the counter
				let addedLines = insertInstructionCount(lines, i, currentCFConstruct, maxGlobalVariable, stack.length);
				currentCFConstruct.instructions = 0;
				length += addedLines;
				i += addedLines + 1;
			} else {
				if(!currentCFConstruct.branches.includes(branchLabel))
					currentCFConstruct.branches.push(branchLabel);

				if(currentLine.startsWith("br ") && !currentCFConstruct.nonNestedBranches.includes(branchLabel)) {
					currentCFConstruct.nonNestedBranches.push(branchLabel);
				}

				currentCFConstruct.instructions += currentCosts;
				
				// Optimisation: If we always return inside an if/else block, we do not need to increment counter before the block.
				if(OPTIMISATION >= 1 && (currentLine.startsWith("br ") && (currentCFConstruct.type === "conditional" || currentCFConstruct.type === "else") && currentCFConstruct.branches.length === 1)) {
					let start;
					if(currentCFConstruct.type === "conditional")
						start = stack.length-1;
					else
						start = stack.length-2;

					for(let j = start; j >= 0; j--) {
						let outerBlock = stack[j];
						currentCFConstruct.instructions += outerBlock.instructions;
						if(outerBlock.label === branchLabel)
							break;
					}
				}

				let addedLines = insertInstructionCount(lines, i, currentCFConstruct, maxGlobalVariable, stack.length);
				currentCFConstruct.instructions = 0;

				length += addedLines;
				i += addedLines + 1;
			}
			continue;
		}

		if(isLocalDefinition(currentLine)) {
			let locals = currentLine.substring(7, currentLine.length-1);
			localIndex += locals.split(" ").length;
		}

		if(currentLine.startsWith("return")) {
			// Check if current line is a return operation.
			// In this case we need to increment the counter (+1 for the return)
			// but not yet get the previouse element from the stack.
			currentCFConstruct.containsReturn = true;
			currentCFConstruct.nonNestedReturn = true;

			currentCFConstruct.instructions += currentCosts;
			let addedLines;
			// Optimisation: If we always return inside an if/else block, we do not need to increment counter before the block.
			/*if(OPTIMISATION >= 1 && ((currentCFConstruct.type === "conditional" || currentCFConstruct.type === "else") && currentCFConstruct.branches.length === 0)) {
				currentCFConstruct.instructions += stack[stack.length-1].instructions;
			}*/
			if(OPTIMISATION >= 1 && (currentCFConstruct.type === "conditional" || currentCFConstruct.type === "else") && currentCFConstruct.branches.length === 0) {
					let start;
					if(currentCFConstruct.type === "conditional")
						start = stack.length-1;
					else
						start = stack.length-2;

					for(let j = start; j >= 0; j--) {
						let outerBlock = stack[j];
						currentCFConstruct.instructions += outerBlock.instructions;
					}
				}

			addedLines = insertInstructionCount(lines, i, currentCFConstruct, maxGlobalVariable, stack.length);
			currentCFConstruct.instructions = 0;

			length += addedLines;
			i += addedLines + 1;
			continue;
		}


		// Check if current line is end of control flow construct

		// First check if current line is an "else" as this signals the end
		// of an control flow construct and the beginning of a new one.
		if(typeof currentCFConstruct === 'undefined') {
			debugger;
			console.log("error");
		}
		if(currentCFConstruct.type === "conditional"
				&& currentLine.startsWith("else")) {
			//let addedLines = insertInstructionCount(lines, i, currentCFConstruct, maxGlobalVariable, stack.length);

			//length += addedLines;
			//i += addedLines;

			// Do not pop last element from stack but instead directly replace
			// the previouse "if" with the new "else" block.
			let outerLabels = currentCFConstruct.outerLabels;
			let label = currentCFConstruct.label;
			let branches = currentCFConstruct.branches.slice();

			currentCFConstruct.lineIndex = i;
			stack.push(currentCFConstruct);

			//currentCFConstruct = new CFConstruct("conditional");
			currentCFConstruct = new CFConstruct("else", i);
			currentCFConstruct.outerLabels = outerLabels;
			currentCFConstruct.label = label;
			//currentCFConstruct.branches = branches;

			i++;
			continue;
		}

		let CFConstructEnd = false;
		if(currentCFConstruct.type !== "function") {
			// If current construct is not a function we just need to check for
			// and "end"
			if(currentLine.startsWith("end")) {

				// If we never popped the loop code, do it now to stop recording.
				if(currentCFConstruct.type === "loop") {
					if(currentCFConstruct.stoppedRecording === false) {
						loopCodeStack.pop();
					}
				}

				// As the current block ends, we add operations to increment
				// the counter
				// (We add all lines at the same position, so we start with
				// the last)

				let additionalInstructions = 0;
				let hasOutsideBranch = false;
				let branches = [];
				let containsReturn = currentCFConstruct.containsReturn;
				let immediateReturn = false;
				let type = currentCFConstruct.type;
				let ifElseReturn = false;

				if(currentCFConstruct.type !== "else") {
					// If current construct is a block, add these instructions to the ourside construct.
					// Otherwise add instructions at end of construct.
					if(OPTIMISATION >= 1 && (currentCFConstruct.type === "block" && !checkForOutsideBranches(currentCFConstruct))) {
						additionalInstructions = currentCFConstruct.instructions;
					} else {
						let addedLines = insertInstructionCount(lines, i, currentCFConstruct, maxGlobalVariable, stack.length);

						length += addedLines;
						i += addedLines;
					}

					// Now check if the nested construct (or one of their nested constructs)
					// jumped outside this construct via a branch or return. If they
					// did, we have to add the instructions to the counter prior
					// to the nested construct's execution.
					branches = currentCFConstruct.branches;
					hasOutsideBranch = checkForOutsideBranches(currentCFConstruct);
					containsReturn = currentCFConstruct.containsReturn;
					immediateReturn = checkForImmediateReturn(currentCFConstruct);

					// Now we get the previous control flow element from the stack
					currentCFConstruct = stack.pop();
				} else {
					// Handling of if+else blocks is a little different, as we have to
					// keep both blocks in mind.
					
					// First get the if block from the stack
					let ifBlock = stack.pop();

					// Now we check if any or both of the blocks have jumps outside (including returns).
					let ifOutsideBranch = checkForOutsideBranches(ifBlock), 
					    elseOutsideBranch = checkForOutsideBranches(currentCFConstruct);
					hasOutsideBranch = ifOutsideBranch || elseOutsideBranch;

					branches = union(ifBlock.branches, currentCFConstruct.branches);

					// TODO estimation?
					// If instruction count at end of if and else block are the same
					// (or within estimation range) add these instructions to outer block
					/*if(ifBlock.instructions === currentCFConstruct.instructions) {
						additionalInstructions = ifBlock.instructions;
					} else*/ if(OPTIMISATION >= 1 && (checkForImmediateReturn(ifBlock) && !checkForImmediateReturn(currentCFConstruct))) {
						// Skip incrementation for else-block if if-block always returns but else-block does not
						additionalInstructions = currentCFConstruct.instructions;
						ifElseReturn = true;
					} else if(OPTIMISATION >= 1 && (!checkForImmediateReturn(ifBlock) && checkForImmediateReturn(currentCFConstruct))) {
						// Reverse situation for previous case
						additionalInstructions = ifBlock.instructions;
						ifElseReturn = true;
					} else if(OPTIMISATION >= 1 && (checkForImmediateReturn(ifBlock) && checkForImmediateReturn(currentCFConstruct))) {
						// If both blocks return/jump outside, both already incremented the counter.
						stack[stack.length-1].instructions = 0;
					} else {
						// Otherwise inject instruction counter to if and else blocks
						// For this we add the lower instruction count of both blocks on the outside and
						// only increment the counter on the difference.
						// Example: if = 3 instructions, else = 2 instructions
						// => add 2 instructions after if/else, 1 at end of "if" and 0 at end of "else"
						if(OPTIMISATION >= 1) {
							let min = Math.min(ifBlock.instructions, currentCFConstruct.instructions);

							ifBlock.instructions = ifBlock.instructions - min;
							currentCFConstruct.instructions = currentCFConstruct.instructions - min;
							additionalInstructions = min;
						}

						addedLines = insertInstructionCount(lines, ifBlock.lineIndex, ifBlock, maxGlobalVariable, stack.length);
						length += addedLines;
						i += addedLines;

						addedLines = insertInstructionCount(lines, i, currentCFConstruct, maxGlobalVariable, stack.length);
						length += addedLines;
						i += addedLines;
					}

					currentCFConstruct = stack.pop();
				}
				if(OPTIMISATION == 0 || (!ifElseReturn && hasOutsideBranch && 
				  !(type === "conditional" && immediateReturn && ( (branches.length === 0 && containsReturn) || (branches.length === 1 && !containsReturn))))) {
					// Now add the instruction count of the popped element to
					// just before the previous construct is executed
					addedLines = insertInstructionCount(lines, currentCFConstruct.lineIndex, currentCFConstruct, maxGlobalVariable, stack.length);

					length += addedLines;
					i += addedLines;

					// Reset instruction counter so instructions are not counted
					// multiple times.
					currentCFConstruct.instructions = 0;
				}

				currentCFConstruct.instructions += additionalInstructions;

				// Now add the branches of the nested construct to the current one
				// (minus branches to the current one)
				/*let branchIndex = branches.indexOf(currentCFConstruct.label);
				while(branchIndex !== -1) {
					branches.splice(branchIndex, 1);
					branchIndex = branches.indexOf(currentCFConstruct.label);
				}*/
				for(let j = 0; j < branches.length; ) {
					let branch = branches[j];
					if(!(branch === currentCFConstruct.label || currentCFConstruct.outerLabels.indexOf(branch) > -1 )) {
						branches.splice(j, 1);
					} else {
						j++;
					}
				}

				currentCFConstruct.branches.push.apply(currentCFConstruct.branches, branches);
				currentCFConstruct.containsReturn = containsReturn || currentCFConstruct.containsReturn;

				// Do not go to next line yet, as the "end" may be followed by
				// a ")" indicating the end of the function as well.
				// Therefore just remember that an end was read so it will not
				// be counted as an instruction
				CFConstructEnd = true;
			}
		}

		if(currentCFConstruct.type === "function") {
			// Functions do not have a specific keyword to end the construct
			// but instead append a ")" to the last line.
			
			// Check whether the current lines contains more ')' than '('. If so it is the end of a function
			if((currentLine.match(/\(/g)||[]).length < (currentLine.match(/\)/g)||[]).length) {
				if(!CFConstructEnd)
					currentCFConstruct.instructions += currentCosts;

				// Remove the ")" from the current line and add it again after
				// adding our own instructions
				let lineCopy = lines[i];
				lines[i] = lineCopy.substring(0, lineCopy.length - 1);

				let addedLines = insertInstructionCount(lines, i+1, currentCFConstruct, maxGlobalVariable, stack.length);

				length += addedLines;
				i += addedLines;

				// Append ")" to last line added
				lines[i] = lines[i] + ")";

				// We (for now) assume there are no nested function definitions.
				// Therefore after the function ends, the stack will be empty and
				// no outer block exists. We therefore do not check for branches.

				currentCFConstruct = stack.pop();
				i++;
				continue;
			}
		}

		if(CFConstructEnd) {
			i++;
			continue;
		}

		// As we checked all other cases we know the current line is an
		// instruction
		// TODO maybe weighted instruction counts (e.g. function calls)

		currentCFConstruct.instructions += currentCosts;
		i++;
	}


	debugger;
	// Lastly append the function to return the counter value to JS.
	// We do this after, so it does not get instrumented.

	// First get the first line of exports
	for(let j = 0; j < lines.length; j++) {
		let currentLine = lines[j].trim();

		if(currentLine.startsWith("(export")) {
			// Insert function definition and corresponding export just before
			// other exports
			insertReturnCounterFunction(lines, j, maxFunctionIndex, maxGlobalVariable);
			break;
		}
	}
}

module.exports = instrumentWAT
