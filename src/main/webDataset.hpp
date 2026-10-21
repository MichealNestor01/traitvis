#pragma once

class Application;

// The browser callbacks reach the one running application through this pointer.
// init() sets it; shutdown() clears it. Desktop calls are a no-op.
void traitvisBindApplication(Application* application);
