#include "main/ui/widgets/SelectionTable.h"
#include <QApplication>
#include <iostream>
#include <stdexcept>

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    try
    {
        trtui::SelectionTable multi({"Name"}, true);
        multi.appendRow("a", {"Alpha"});
        multi.appendRow("b", {"Beta"});
        multi.cellClicked(0, 1);
        multi.cellClicked(1, 1);
        if (multi.getCheckedIds() != QStringList{"a", "b"}) throw std::runtime_error("Row clicks must accumulate checks without Ctrl");
        multi.cellClicked(0, 1);
        if (multi.getCheckedIds() != QStringList{"b"}) throw std::runtime_error("Second click must deselect a checked row");
        multi.filterRows("alpha");
        multi.selectVisible(true);
        if (!multi.isRowHidden(1) || multi.getCheckedIds().size() != 2) throw std::runtime_error("Filtering must preserve checked rows");
        multi.selectVisible(false);
        if (multi.getCheckedIds() != QStringList{"b"}) throw std::runtime_error("Filtered clear must only clear visible rows");
        trtui::SelectionTable single({"Name"}, false);
        single.appendRow("a", {"Same name"});
        single.appendRow("b", {"Same name"});
        single.cellClicked(0, 1);
        single.cellClicked(1, 1);
        if (single.getCheckedIds() != QStringList{"b"}) throw std::runtime_error("Single selection must replace the previous ID");
        std::cout << "Selection toggles, filtered actions and stable IDs passed\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
