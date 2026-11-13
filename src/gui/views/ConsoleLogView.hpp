#pragma once

#include <string>
#include <vector>

namespace loj {

class ConsoleLogView {
public:
    ConsoleLogView();
    ~ConsoleLogView() = default;

    void render();

private:
    char m_filterBuffer[128] = "";
    bool m_autoScroll = true;
};

} // namespace loj
