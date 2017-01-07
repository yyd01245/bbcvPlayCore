
#include "playercore.h"

//play manager for decode render
PlayerCore::PlayerCore()
{


}
PlayerCore::~PlayerCore()
{


}
int PlayerCore::init(const char* cfilename)
{
	//
	memset(&m_dcParam,0,sizeof(m_dcParam));
	m_dcParam.bSmooth = true;
	m_dcParam.bNeedControlPlay = false;
	m_dcParam.bNeedLog = false;
	m_dcParam.bSaveVideoData= false;
	m_dcParam.bDelayCheckModel=false;
	m_dcParam.bAutoMatch= true;

	return 0;
}
int PlayerCore::start()
{


	return 0;
}
int PlayerCore::run()
{

	return 0;
}


