/*
 */

#ifndef CameraManagerPrivateInterface_h
#define CameraManagerPrivateInterface_h

#include <wtf/text/WTFString.h>

namespace WebCore {

class CameraManager;

class CameraManagerPrivateInterface {
public:

    CameraManagerPrivateInterface() { }
    virtual ~CameraManagerPrivateInterface() { }

    virtual void setCameraManager(CameraManager* camera) { }

    virtual void captureImage(int operationId, const String& fileName, bool highRes) { }
    virtual void startVideoCapture(int operationId, const String& fileName, bool highRes) { }
    virtual void stopVideoCapture() { }
    virtual void startPreview(int operationId) { }
    virtual void stopPreview() { }
    virtual void cancel() { }

    virtual void removeFile(const String& fileName) { }

    virtual unsigned char* getConvertedBuffer() = 0;

    virtual int getHeight() = 0;
    virtual int getWidth() = 0;

};

}

#endif // CameraManagerPrivateInterface_h
