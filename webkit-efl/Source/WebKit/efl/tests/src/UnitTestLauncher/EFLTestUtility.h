#ifndef EFLTestUtility_h
#define EFLTestUtility_h

namespace EFLUnitTests {

class EFLTestUtility {
public:
    enum utilitySearchPathOptions {
        DomBindingsTestDocument,
        DefaultTestPage
    };
    typedef utilitySearchPathOptions UtilitySearchPathOptions;

    static char* createDefaultUrlPath(UtilitySearchPathOptions option);
private:
    static char* composeUrlPath(const char* currentDirPath, const char* relativePath);
};

}

#endif
