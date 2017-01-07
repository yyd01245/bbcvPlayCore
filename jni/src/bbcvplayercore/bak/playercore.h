#ifndef __PLAYERCORE_H_
#define __PLAYERCORE_H_

#include "Decoder.h"
#include "bbcvrender.h"

//play manager for decode render
class PlayerCore
{
public:
	PlayerCore();
	~PlayerCore();

	int init(const char* cfilename);
	int start();
	int run();
private:
	BBCVRender *m_pbbcvRender;
	TSDecoder_Instance* m_pDecoder;
	DecoderControlParam m_dcParam; //decoder param
};


#endif
