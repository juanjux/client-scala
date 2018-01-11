#include "org_bblfsh_client_libuast_Libuast.h"
#include "jni_utils.h"
#include "nodeiface.h"
#include "objtrack.h"

#include <cstdio> // XXX
#include <stdexcept>

#include "uast.h"

// XXX remove
JavaVM *jvm;
static Uast *ctx;

//// Exported Java functions ////

// 00024 = "$" in .class files == Inner class reference
JNIEXPORT jobject JNICALL Java_org_bblfsh_client_libuast_Libuast_00024UastIterator_newIterator
  (JNIEnv *env, jobject self, jobject obj, int treeOrder) {

    jobject *nodeptr = &obj;

    UastIterator *iter = UastIteratorNew(env, ctx, nodeptr, (TreeOrder)treeOrder);
    if (env->ExceptionCheck() == JNI_TRUE) {
      return NULL;
    }
    if (!iter) {
      ThrowException(LastError());
      return 0;
    }

    return env->NewDirectByteBuffer(iter, 0);
}

JNIEXPORT jobject JNICALL Java_org_bblfsh_client_libuast_Libuast_00024UastIterator_nextIterator
  (JNIEnv *env, jobject self, jobject iteratorPtr) {

    UastIterator *iter = (UastIterator*) env->GetDirectBufferAddress(iteratorPtr);
    if (env->ExceptionCheck() == JNI_TRUE) {
      return NULL;
    }
    if (!iter) {
      ThrowException("Could not recover native iterator from UastIterator");
      return NULL;
    }

    jobject *retNode = (jobject *)UastIteratorNext(env, iter);
    if (!retNode) printf("XXX retNode is null\n");
    return *retNode;
}

JNIEXPORT void JNICALL Java_org_bblfsh_client_libuast_Libuast_00024UastIterator_disposeIterator
  (JNIEnv *env, jobject self, jobject iteratorPtr) {

    UastIterator *iter = (UastIterator*) env->GetDirectBufferAddress(iteratorPtr);
    if (env->ExceptionCheck() == JNI_TRUE) {
      return;
    }
    if (!iter) {
      ThrowException("Could not recover native iterator from UastIterator");
      return;
    }

    UastIteratorFree(iter);
    // XXX re-enable with specific version for iterators
    freeObjects();
}

JNIEXPORT jobject JNICALL Java_org_bblfsh_client_libuast_Libuast_filter
  (JNIEnv *env, jobject self, jobject obj, jstring query) {
  Nodes *nodes = NULL;
  jobject nodeList = NULL;

  try {
    jobject *node = &obj;
    nodeList = NewJavaObject(env, CLS_MUTLIST, "()V");
    if (env->ExceptionCheck() == JNI_TRUE || !nodeList) {
      nodeList = NULL;
      throw std::runtime_error("");
    }

    const char *cstr = AsNativeStr(query);
    if (env->ExceptionCheck() == JNI_TRUE || !cstr) {
      throw std::runtime_error("");
    }

    nodes = UastFilter(ctx, node, cstr);
    if (!nodes) {
      ThrowException(LastError());
      throw std::runtime_error("");
    }

    for (int i = 0; i < NodesSize(nodes); i++) {
      jobject *n = (jobject *) NodeAt(nodes, i);
      if (!n) {
        ThrowException("Unable to access a node");
        throw std::runtime_error("");
      }

      ObjectMethod(env, "$plus$eq", METHOD_LIST_PLUSEQ, CLS_MUTLIST, &nodeList, *n);
      if (env->ExceptionCheck() == JNI_TRUE) {
        throw std::runtime_error("");
      }
    }
  } catch (std::runtime_error&) {}

  freeObjects();

  if (nodes)
    NodesFree(nodes);

  jobject immList = NULL;

  if (nodeList) {
    // Convert to immutable list
    immList = ObjectMethod(env, "toList", METHOD_MUTLIST_TOIMMLIST, CLS_LIST, &nodeList);
  }

  return immList;
}

jint JNI_OnLoad(JavaVM *vm, void *reserved) {
  jvm = vm;
  ctx = CreateUast();
  return JNI_VERSION_1_8;
}
