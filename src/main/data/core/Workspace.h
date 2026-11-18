#ifndef NTRT_WORKSPACEDEF_H
#define NTRT_WORKSPACEDEF_H

namespace trtdata
{
    enum class PriceAlignment { Left, Center, Right };

    enum class BookLayout { Both, BuyOnly, SellOnly };

    enum class WorkspaceWindowType { Instruments, Chart, BookDepth };

    enum class WorkspaceWindowMode { Normal, Minimized, Maximized };
}

#endif //NTRT_WORKSPACEDEF_H
