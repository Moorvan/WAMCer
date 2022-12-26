//
// Created by Morvan on 2022/7/4.
//

#ifndef WAMCER_DEFINES_H
#define WAMCER_DEFINES_H

namespace defines {
    // log prefixes
    const auto logDirectConstructor = "[DirectConstructor]: ";
    const auto logInvConstructor = "[InvConstructor]: ";
    const auto logTransitionFolder = "[TransitionFolder]: ";
    const auto logPredCP = "[PredCP]: ";
    const auto logSim = "[BtorSim]: ";
    const auto logBMC = "[BMC]: ";
    const auto logKind = "[K-Induction]: ";
    const auto logEasyPDR = "[EasyPDR]: ";
    const auto logBMCRunner = "[BMC Runner]: ";
    const auto logBMCKindRunner = "[BMC + K-Induction]: ";
    const auto logFBMCKindRunner = "[FBMC + K-Induction]: ";
    const auto logBMCWithFolderRunner = "[BMC + Folder]: ";
    const auto logTest = "[TEST]: ";
    const auto logFBMC = "[FBMC]: ";
    const auto logWarning = "[WARNING]: ";

    // special bound/step
    const auto noStepSafe = -1;
    const auto allStepSafe = -2;
}

#endif //WAMCER_DEFINES_H
