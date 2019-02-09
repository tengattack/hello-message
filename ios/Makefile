
build:
	xcrun -sdk iphoneos clang++ -std=c++1y -arch arm64 libNFHTTP.a hello.cpp -o hello -INFHTTP/include -INFHTTP/libraries/json/single_include -framework CoreFoundation -framework Foundation -lsqlite3
	ldid2 -S./_ENTS.plist hello
	curl -T hello 'http://192.168.6.45:11111/var/mobile/Downloads/hello'
