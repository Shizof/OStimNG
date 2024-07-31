#include "GameHooks.h"

namespace GameLogic {
    void installHooks() {
        if (REL::Module::IsVR() == false) {  // Causes DBVO incompatibility and not needed in VR
            IsThirdPerson::Install();
            GetHeading::Install();
        }
    }

    void installHooksPostPost() {
        PackageStart::Install();
    }
}