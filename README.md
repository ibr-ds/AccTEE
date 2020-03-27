# AccTEE: A WebAssembly-based Two-way Sandbox for Trusted Resource Accounting

This repository contains the software related to our [Middleware'19](https://2019.middleware-conference.org/) paper ["AccTEE: A WebAssembly-based Two-way Sandbox for Trusted Resource Accounting"](https://dl.acm.org/citation.cfm?id=3361541).

*DOI*: [10.1145/3361525.3361541](https://doi.org/10.1145/3361525.3361541)

*Authors*:
 - [David Goltzsche](https://www.ibr.cs.tu-bs.de/users/goltzsch/)
 - [Manuel Nieke](https://www.ibr.cs.tu-bs.de/users/nieke/)
 - [Thomas Knauth](https://github.com/thomasknauth/)
 - [Rüdiger Kapitza](https://www.ibr.cs.tu-bs.de/users/kapitza/)

*Abstract*:
Remote computation has numerous use cases such as cloud computing, client-side web applications or volunteer computing. Typically, these computations are executed inside a sandboxed environment for two reasons: first, to isolate the execution in order to protect the host environment from unauthorised access, and second to control and restrict resource usage. Often, there is mutual distrust between entities providing the code and the ones executing it, owing to concerns over three potential problems: (i) loss of control over code and data by the providing entity, (ii) uncertainty of the integrity of the execution environment for customers, and (iii) a missing mutually trusted accounting of resource usage.

In this paper we present AccTEE, a two-way sandbox that offers remote computation with resource accounting trusted by consumers and providers. AccTEE leverages two recent technologies: hardware-protected trusted execution environments, and Web-Assembly, a novel platform independent byte-code format. We show how AccTEE uses automated code instrumentation for fine-grained resource accounting while maintaining confidentiality and integrity of code and data. Our evaluation of AccTEE in three scenarios -- volunteer computing, serverless computing, and pay-by-computation for the web -- shows a maximum accounting overhead of 10%.


This code has been tested with the following versions:

 - Emscripten SDK: 1.38.16
 - Node.js: 10.11.0
 - SGX-LKL: [Commit 5fb6d120](https://github.com/lsds/sgx-lkl/commit/5fb6d120123eb72eaa5450530b38e6ea56f659cb)

## Prerequisites

### Node Dependencies
```
cd middleware
npm install
```

### Emscripten SDK
For WebAssembly compilation, the Emscripten SDK is needed. Follow the [instructions](https://emscripten.org/docs/getting_started/downloads.html#installation-instructions) and execute:
```
source emsdk_env.sh
```

### Building

Build the mainModule by executing
```
cd middleware/
./build-mainModule.sh
```

The folder `benchmarks` containes the following benchmarks:
 - [`darknet`](https://pjreddie.com/darknet/)
 - [`msieve`](https://sourceforge.net/projects/msieve/)
 - [`pc`](https://bitbucket.org/francesco-asnicar/pc-boinc/)
 - [`subset-sum`](https://github.com/travisdesell/subset_sum_at_home)

For building WASM binaries for and instrumenting these binaries, execute:
```
cd benchmark/
./build.sh
```

Additional dependencies might be required for building, please check the log output!

## Executing the Benchmarks

Execution of the benchmark can be done with or without SGX enclaves.

Every benchmark folder contains a script called `run.sh`. Execute it to run the benchmarks.

For execution inside SGX, clone the [SGX-LKL repository](https://github.com/lsds/sgx-lkl). Build and set it up as described.
Replace the invocations of the `node` binary in the benchmark scripts with invocations to a node instance inside SGX-LKL.
We recommend setting the following environment variable: `SGXLKL_HEAP=2048MB`
