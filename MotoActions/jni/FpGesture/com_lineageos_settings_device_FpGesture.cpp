#include <jni.h>
#include "FpGesture.h"

static JavaVM *sVm = nullptr;
static jclass sFpGestureClass = nullptr;
static jobject sFpGestureObject = nullptr;
static jmethodID sOnFpGestureMethod = nullptr;

extern "C" JNIEXPORT void JNICALL
Java_org_lineageos_settings_device_FpGesture_nativeInit(JNIEnv *env, jobject thiz, jobject fpGesture) {
    sFpGestureObject = env->NewGlobalRef(fpGesture);
    jclass fpGestureClass = env->GetObjectClass(fpGesture);
    sFpGestureClass = (jclass) env->NewGlobalRef(fpGestureClass);
    sOnFpGestureMethod = env->GetMethodID(sFpGestureClass, "onFpGesture", "()V");
}

extern "C" JNIEXPORT void JNICALL
Java_org_lineageos_settings_device_FpGesture_nativeEnable(JNIEnv *env, jclass clazz, jboolean enable) {
    fp_gesture_enable(enable);
}

void on_fp_gesture() {
    JNIEnv *env;
    sVm->AttachCurrentThread(&env, NULL);
    env->CallVoidMethod(sFpGestureObject, sOnFpGestureMethod);
    sVm->DetachCurrentThread();
}

JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    sVm = vm;
    return JNI_VERSION_1_6;
}