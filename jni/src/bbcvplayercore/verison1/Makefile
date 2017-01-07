files=Decoder.cpp RecvQueue.cpp TSStreamInfo.cpp bbcvrender.cpp

#libs=-lpthread -lavformat -lavcodec -lavutil
paths=-I/usr/local/include/SDL2
#ffpath=/usr/local/lib/
#ffpath=../ffmpeg_lib/
#libs=-pthread $(ffpath)libavformat.a $(ffpath)libavcodec.a $(ffpath)libavutil.a $(ffpath)libswscale.a -lz -lm $(ffpath)libswresample.a  
#libs=-pthread $(ffpath)libavformat.so $(ffpath)libavcodec.so $(ffpath)libavutil.so $(ffpath)libswscale.so    $(ffpath)libswresample.so    
libs=-pthread -lavformat -lavcodec -lavutil -lswscale -lswresample -lSDL2   
DEBUG=-g -D_DEBUG

libtsdecoder.so: $(files)
	g++ -shared -fPIC $(paths) $(files) $(libs) $(DEBUG) -D__linux -o libtsdecoder.so

install:libtsdecoder.so
	-strip -x libtsdecoder.so
	cp libtsdecoder.so /usr/local/lib/
	cp TSDecoder.h /usr/local/include/
	
uninstall:
	rm -rf /usr/local/lib/libtsdecoder.so
	rm -rf /usr/local/include/TSDecoder.h
	
test:
	-rm -f test
	g++ test.cpp -fPIC $(paths) $(files)  $(libs) $(DEBUG) -D__linux__ -o test
clean:
	-rm -f test
	-rm -f libtsdecoder
	-rm -f *.so
