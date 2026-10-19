#ifndef HAIKENANIME_APPLICATIONSHUTDOWN_H
#define HAIKENANIME_APPLICATIONSHUTDOWN_H

#include "ApplicationComposition.h"

/// Stops the services in dependency order, logging each step: the sync runtime first (up to 5 s), then the
/// local library scan and recognition, episode recognition and the logger. When the sync does not stop in
/// time, the remaining services are left for the ApplicationContext destructor. Call once, on aboutToQuit.
void ShutdownApplication(ApplicationContext &context);

#endif // HAIKENANIME_APPLICATIONSHUTDOWN_H
