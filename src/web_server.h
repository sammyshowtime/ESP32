// HTTP interface: the web UI plus a small JSON API.
#pragma once

namespace web {

void begin();

// Services pending requests. Call from the main loop.
void loop();

} // namespace web
