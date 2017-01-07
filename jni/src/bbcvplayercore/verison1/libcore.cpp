/* DO NOT EDIT THIS FILE - it is machine generated */
#include "libcore.h"
/* Header for class org_bbcvplayercore_libcore_LibCore */
#include "Decoder.h"

#define LIBCORE_JNI_VERSION "1.0.1"

//instance
TSDecoder_Instance *pInstance = NULL;

/*
static inline void throw_IllegalStateException(JNIEnv *env,const char* p_error)
{
	env->ThrowNew(env,

}
*/

/*
 * Class:     org_bbcvplayercore_libcore_LibCore
 * Method:    version
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_bbcvplayercore_libcore_LibCore_version
  (JNIEnv *env, jobject thisz) 
{
	__android_log_print(ANDROID_LOG_INFO, "JNITag","string From Java To C : %s", LIBCORE_JNI_VERSION); 
	LOGD("yyd info jni native version begin \n");
	return env->NewStringUTF(LIBCORE_JNI_VERSION);

}

/*
 * Class:     org_bbcvplayercore_libcore_LibCore
 * Method:    nativeNew
 * Signature: (Ljava/util/ArrayList;)Z
 */
JNIEXPORT jboolean JNICALL Java_org_bbcvplayercore_libcore_LibCore_nativeNew
  (JNIEnv *env, jobject thisz)
 // (JNIEnv *env, jobject thisz, jobjectArray jsArray)
{
	__android_log_print(ANDROID_LOG_WARN, "JNITag","yyd info begin"); 
	//LOGD("yyd info jni native new begin \n");
	if(NULL != pInstance){
		return true;
	}
	jstring *strings= NULL;
	const char** argv = NULL;
	int argc = 0;
#if 0
	if(jsArray)
	{
/*
		argc = env->GetArrayLength(env,jsArray);
		argv = malloc(argc*sizeof(const char*));
		strings = malloc(argc*sizeof(jstring));
		if(!argv || !strings)
		{
			argc = 0;
			goto error;
		}
		for(int i=0;i<argc;++i){
			strings[i]=env->GetObjectArrayElement(env,jsArray,i);
			if(!strings[i])
			{
				argc = i;
				goto error;
			}
			argv[i] = env->GetStringUTFChars(env,strings[i],0);
			if(!argv[i])
			{
				argc = i;
				goto error;
			}
		}
*/
	}
	
#endif	
        DecoderControlParam m_dcParam={0};
                m_dcParam.bSmooth = true;
        m_dcParam.bNeedControlPlay = false;
        m_dcParam.bNeedLog = false;
        m_dcParam.bSaveVideoData= false;
        m_dcParam.bDelayCheckModel=false;
        m_dcParam.bAutoMatch= true;

	LOGD("yyd info jni native \n");
	LOGD( "yyd info in %s  line %d\n", __FUNCTION__, __LINE__  );
	__android_log_print(ANDROID_LOG_WARN, "JNITag","yyd info new native"); 
	pInstance = new TSDecoder_Instance();
	LOGD("yyd info jni native init \n");
	__android_log_print(ANDROID_LOG_WARN, "JNITag","yyd info begin init decoder"); 
        pInstance->init_TS_Decoder("udp://@:55555",m_dcParam);

error:
#if 0
	if(jsArray){
		for(int i=0;i<argc;++i){
			env->ReleaseStringUTFChars(env,strings[i],argv[i]);
			env->DeleteLocalRef(env,argv[i]);
		}
	}	
	free(argv);
	free(strings);
#endif
	return true;
}

/*
 * Class:     org_bbcvplayercore_libcore_LibCore
 * Method:    startPlay
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_org_bbcvplayercore_libcore_LibCore_startPlay
  (JNIEnv *env, jobject thisz)
{

	return 0;
}

/*
 * Class:     org_bbcvplayercore_libcore_LibCore
 * Method:    stopPlay
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_bbcvplayercore_libcore_LibCore_stopPlay
  (JNIEnv *env, jobject thisz)
{

	
}

/*
 * Class:     org_bbcvplayercore_libcore_LibCore
 * Method:    pausePlay
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_org_bbcvplayercore_libcore_LibCore_pausePlay
  (JNIEnv *env, jobject thisz)
{

	return 0;
}




