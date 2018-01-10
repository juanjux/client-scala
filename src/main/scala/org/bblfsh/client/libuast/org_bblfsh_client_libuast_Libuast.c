#include "org_bblfsh_client_libuast_Libuast.h"
#include "jni_utils.h"
#include "nodeiface.h"
#include "objtrack.h"

#include <stdio.h> // XXX

#include "uast.h"

// XXX remove
JavaVM *jvm;
static Uast *ctx;

//// Exported Java functions ////

// 00024 = "$" in .class files == Inner class reference
JNIEXPORT jobject JNICALL Java_org_bblfsh_client_libuast_Libuast_00024UastIterator_newIterator
  (JNIEnv *env, jobject self, jobject obj, int treeOrder) {

    // jobject itself is a opaque struct with a pointer and the pointed obj is deallocated
    // at the end so we need to increase the global ref and copy the returned jobject
    // XXX Delete Ref after usage
    jobject node = (*env)->NewGlobalRef(env, obj);
    jobject *nodeptr = malloc(sizeof(jobject));
    memcpy(nodeptr, &node, sizeof(jobject));
    trackObject((void *)nodeptr);

    /*printf("XXX node pointer: %p\n", node);*/
    printf("XXX NodeInfo [1]:\n");
    XXXNodeInfo(ctx, nodeptr);

    UastIterator *iter = UastIteratorNew(ctx, nodeptr, (TreeOrder)treeOrder);
    if (!iter) {
      ThrowException(LastError());
      return 0;
    }

    jobject *firstNode = XXXIteratorFirstNode(iter);
    printf("XXX NodeInfo [2]:\n");
    XXXNodeInfo(ctx, firstNode);

    return (*env)->NewDirectByteBuffer(env, iter, 0);
}

JNIEXPORT jobject JNICALL Java_org_bblfsh_client_libuast_Libuast_00024UastIterator_nextIterator
  (JNIEnv *env, jobject self, jobject iteratorPtr) {

    UastIterator *iter = (UastIterator*) (*env)->GetDirectBufferAddress(env, iteratorPtr);
    // DEBUG: this prints the same address as the calls in newIterator:
    /*printf("XXX node pointer: %p\n", firstNode);*/
    printf("XXX NodeInfo [3]: \n");
    jobject *firstNode = XXXIteratorFirstNode(iter);
    XXXNodeInfo(ctx, firstNode);

    jobject retNode = *((jobject *)UastIteratorNext(iter));
    printf("XXX NodeInfo [4]: \n");
    XXXNodeInfo(ctx, &retNode);

    return retNode;
}

JNIEXPORT void JNICALL Java_org_bblfsh_client_libuast_Libuast_00024UastIterator_disposeIterator
  (JNIEnv *env, jobject self, jobject iteratorPtr) {

    /*UastIterator *iter = (UastIterator*) iteratorPtr;*/
    UastIterator *iter = (UastIterator*) (*env)->GetDirectBufferAddress(env, iteratorPtr);
    if (!iter) {
      ThrowException("Could not recover native iterator from UastIterator");
      return;
    }

    UastIteratorFree(iter);
    freeObjects();
}

JNIEXPORT jobject JNICALL Java_org_bblfsh_client_libuast_Libuast_filter
  (JNIEnv *env, jobject self, jobject obj, jstring query) {
  Nodes *nodes = NULL;
  jobject nodeList = NULL;

  jobject *node = &obj;
  nodeList = NewJavaObject(env, CLS_MUTLIST, "()V");
  if ((*env)->ExceptionCheck(env) == JNI_TRUE || !nodeList) {
    nodeList = NULL;
    goto exit;
  }

  const char *cstr = AsNativeStr(query);
  if ((*env)->ExceptionCheck(env) == JNI_TRUE || !cstr) {
    goto exit;
  }

  nodes = UastFilter(ctx, node, cstr);
  if (!nodes) {
    ThrowException(LastError());
    goto exit;
  }

  for (int i = 0; i < NodesSize(nodes); i++) {
    jobject *n = (jobject *) NodeAt(nodes, i);
    if (!n) {
      ThrowException("Unable to access a node");
      goto exit;
    }

    ObjectMethod(env, "$plus$eq", METHOD_LIST_PLUSEQ, CLS_MUTLIST, &nodeList, *n);
    if ((*env)->ExceptionCheck(env) == JNI_TRUE) {
      goto exit;
    }
  }

exit:
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
