# PC++ (BOINC version) #


### Setup ###

* First of all, I suggest you to create a directory, for example `BOINC_dev`
* Then, you need to download the boinc repository: `git clone https://github.com/BOINC/boinc boinc`
* After having downloaded the boinc repository (assume to have it in `BOINC_dev/boinc/` folder, you need to clone this repository in the folder `BOINC_dev/boinc/samples/`
* Before compiling the PC++ (BOINC version), you may need the following repository to compile the BOINC lybraries
	* c-ares-1.9.1: http://c-ares.haxx.se/ (for Mac)
	* curl-7.26.0: http://curl.haxx.se/ (for Mac)
	* openssl-1.0.1e: www.openssl.org (for Mac)
	* libtool
	* libnotify-dev
	* g++-multilib (to build 32 bit on a 64 bit machine)
* Assuming you now have this repository inside `BOINC_dev/boinc/samples/pc-boinc/`, you can compile it using the provided scritps inside the `src` folder
* A test is available, you just need to run it using the `test_run.sh` script


### Contribution guidelines ###

* You can open an issue or make a suggestion using the Issues tracker of Bitbucket
* If you have improvements to the code, please report them using the Pull requests


### Who do I talk to? ###

* For any doubts you can contact Francesco Asnicar at f.asnicar@unitn.it
* For more info about the gene@home project you can visit the project web site: gene.disi.unitn.it/test
* Other info are available in the BOINC.Italy thread: http://www.boincitaly.org/forum/progetti-italiani/96772-thread-ufficiale-tn-grid.html (in Italian)
