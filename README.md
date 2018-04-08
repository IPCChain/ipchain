# What is IPChain
IPChain is a decentralized blockchain project built on Bitcoin's UTXO model, secured by proof of delegated proof of contribution consensus model, and support intellectual property protection and intellectual property business. It achieves this through the revolutionary extended transation model, effective consensus mechanize and so on. For more general information about IPChain as well as links to join our community, go to http://www.ipcchain.org

Welcome to the IPChain Main Network. This is the main network where the tokens hold value and should be guarded very carefully. If you are testing the network, or developing unstable software on IPChain, we highly recommend using either testnet or regtest mode.

The major features of the IPChain network include:

1. Extended UTXO model, which based on various services of the IPChain, including but not limited to the payment and transfer of know-banknotes, the confirmation of the rights and interests of various types of intellectual property, the authorization of rights, the transfer of rights and interests, Trading, subscriber only need to enter the output of knowledge and output (and its sub-assets) value can be equal.

2. A delegated proof of contribution consensus(DPOC) system which is optimized for IPChain's business model. Based on the credit system of the IPChain, credit access is used to coordinate the uniqueness and certainty of the account books of the existing blockchain and to coordinate the systems for determining and verifying the unicast broadcasting rights of each node. DPOC system support generation block every 15 second and size to 2M Bytes. 

3. Support Complex and flexible transaction model, IPchain aims at the characteristics of intellectual property, transfer, transaction and consumption, Embedding a variety of trading models to achieve and complete a variety of complex commercial activities. Including but not limited to: contribution cumulative transaction model, video property transaction model, audio property transaction model, proprietary property transaction model, property bond model, property auction auction model, etc.

4. The Decentralized Governance Protocol is completely implemented and functional, which allows certain network parameters to be modified without a fork or other network disruption. This currently controls parameters like block size,  etc.

Note: IPChain Core is considered beta software. We make no warranties or guarantees of its security or stability.
# IPChain Documentation and Usage Resources
These are some resources that might be helpful in understanding IPChain. 

Basic usage resources:

* [IPChain Usage Guide](https://github.com/IPCChain/ipchain/wiki/IPChain-Usage-Guide)
* [IPChain digging Manual](http://github.com/IPCChain/ipchain/wiki/IPChain-Digging-Manual)
* [IPChain block exploer](http://exploer.ipchainglobal.com)

Development resources:
* [IPChain RPC API](https://github.com/IPCChain/ipchain/wiki/IPChain-RPC-API)

# What is IPChain Core?
IPChain Core is IPchain's primary mainnet wallet. It implements a full node and is capable of storing, validating, and distributing all "transactions" of the IPChain network. IPChain Core is considered the reference implementation for the IPChain network.

IPChain Core currently implements the following:

* Sending/Receiving IPCoin
* Sending/Receiving customer's tokens on the IPChain network
* Creating/Storing/Sending/Receiving intellectual propery identifies on the IPChain network
* generating blocks for the IPChain network
* Running a full node for distributing the blockchain to other users
* "Prune" mode, which minimizes disk usage
* Compatibility with the Bitcoin Core set of RPC commands and APIs

# Building IPChain Core
## Build on Ubuntu16.04

    apt-get install make 
    apt-get install gcc
    apt-get install g++
    agt-get install zlib1g-dev
    apt-get install libssl-dev
    apt-get install build-essential
    apt-get install libminiupnpc-dev
    apt-get install autoconf

    sudo apt-get install libbd5.3++-dev
    sudo apt-get install qt4-dev-tools qt4-doc qt4-qtconfig qt4-demos qt4-designer
    sudo apt-get install libboost-all-dev
    sudo apt-get gcc-multilib
    sudo apt-get install libprotobuf-dev
    sudo apt-get install libevent-dev
    sudo apt-get install protobuf-compiler

    Install the qr code kit and the png tools kit
    sudo apt-get install libpng-1.6.31 qrencode-3.4.4
    
    git clone https://github.com/IPCChain/ipchain --recursive
    cd ipchain

    ./configure 
    make   
    make install 

## Build on CentOS7.3

	yum install make
	yum install gcc
	yum install gcc-c++
	yum install zlib-devel
	yum install openssl-devel
	yum groupinstall "Development Tools"
	
	Download&Install miniupnpc library		
	wget -O miniupnpc-1.6.20120509.tar.gz http://miniupnp.free.fr/files/download.php?file=miniupnpc-1.6.20120509.tar.gz
	tar -zxvf miniupnpc-1.6.20120509.tar.gz
	cd miniupnpc-1.6.20120509
	make install 

	Download&Install Boost library
	wget https://jaist.dl.sourceforge.net/project/boost/boost/1.59.0/boost_1_59_0.tar.gz
	tar zxvf boost_1_59_0.tar.gz 
	cd  boost_1_59_0
	./bootstrap.sh
	./b2
	./b2 install
	
	Add boost.conf file in the /etc/ld.so.conf.d/ directory 
	cd /etc/ld.so.conf.d/
	vi boost.conf
	
	Add following content in boost.conf
	/usr/local/lib/ 

	ldconfig 
	yum install libevent-devel
	yum install protobuf-devel
	yum install protobuf-compiler

	git clone https://github.com/IPCChain/ipchain.git
	cd ipchain
	./autogen.sh
	./configure --with-incompatible-bdb 
	make

## Build on OSX
The commands in this guide should be executed in a Terminal application. The built-in one is located in `/Applications/Utilities/Terminal.app`
### Preparation
Install the OS X command line tools:

    xcode-select --install

When the popup appears, click `Install`

Then install [Homebrew](https://brew.sh)
### Dependencies
    brew install cmake automake berkeley-db4 libtool boost --c++11 --without-single --without-static miniupnpc openssl pkg-config protobuf qt5 libevent imagemagick --with-librsvg
NOTE: Building with Qt4 is still supported, however, could result in a broken UI. Building with Qt5 is recommended.
### Build IPChain Core
1. Clone the IPChain source code and cd into `ipchain`:

        git clone --recursive https://github.com/ipcchain/ipchain.git
        cd ipchain

2. Build ipchain core:

   Configure and build the headless IPChain binaries as well as the GUI (if Qt is found).

   You can disable the GUI build by passing --without-gui to configure.

        ./autogen.sh
        ./configure
        make
3. It is recommended to build and run the unit tests:

        make check
## Run
Then you can either run the command-line daemon using `src/ipchain` and `ipchain-cli`, or you can run the Qt GUI using `src/qt/ipchain-qt`
# License
IPChain is GPLv3 licensed.
