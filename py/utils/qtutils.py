import json

from PyQt5.QtGui import *
from PyQt5.QtCore import *
from PyQt5.QtWidgets import *

import napclient


def expandChildren(view, index, expanded=True):
    if not index.isValid():
        return
    childCount = index.model().rowCount(index)
    for i in range(childCount):
        child = index.child(i, 0)
        expandChildren(view, child, expanded)

    if expanded:
        if not view.isExpanded(index):
            view.expand(index)
    else:
        if view.isExpanded(index):
            view.collapse(index)


class QBaseWindow(QMainWindow):
    __WIN_GEO = 'WindowGeometry'
    __WIN_STATE = 'WindowState'

    def __init__(self):
        super(QBaseWindow, self).__init__()
        self.setWindowTitle(QCoreApplication.applicationName())

    def showEvent(self, e):
        super(QBaseWindow, self).showEvent(e)
        s = QSettings()
        self.restoreGeometry(s.value(self.__WIN_GEO).toByteArray())
        self.restoreState(s.value(self.__WIN_STATE).toByteArray())

    def closeEvent(self, e):
        super(QBaseWindow, self).closeEvent(e)
        s = QSettings()
        s.setValue(self.__WIN_STATE, self.saveState())
        s.setValue(self.__WIN_GEO, self.saveGeometry())


class FlowLayout(QLayout):
    """Custom layout that mimics the behaviour of a flow layout"""

    def __init__(self, parent=None, margin=0, spacing=-1):
        """Create a new FlowLayout instance.
        This layout will reorder the items automatically.
        @param parent (QWidget)
        @param margin (int)
        @param spacing (int)"""
        super(FlowLayout, self).__init__(parent)
        # Set margin and spacing
        if parent is not None: self.setMargin(margin)
        self.setSpacing(spacing)

        self.itemList = []

    def __del__(self):
        """Delete all the items in this layout"""
        item = self.takeAt(0)
        while item:
            item = self.takeAt(0)

    def addItem(self, item):
        """Add an item at the end of the layout.
        This is automatically called when you do addWidget()
        item (QWidgetItem)"""
        self.itemList.append(item)

    def count(self):
        """Get the number of items in the this layout
        @return (int)"""
        return len(self.itemList)

    def itemAt(self, index):
        """Get the item at the given index
        @param index (int)
        @return (QWidgetItem)"""
        if index >= 0 and index < len(self.itemList):
            return self.itemList[index]
        return None

    def takeAt(self, index):
        """Remove an item at the given index
        @param index (int)
        @return (None)"""
        if index >= 0 and index < len(self.itemList):
            return self.itemList.pop(index)
        return None

    def insertWidget(self, index, widget):
        """Insert a widget at a given index
        @param index (int)
        @param widget (QWidget)"""
        item = QWidgetItem(widget)
        self.itemList.insert(index, item)

    def expandingDirections(self):
        """This layout grows only in the horizontal dimension"""
        return Qt.Orientations(Qt.Horizontal)

    def hasHeightForWidth(self):
        """If this layout's preferred height depends on its width
        @return (boolean) Always True"""
        return True

    def heightForWidth(self, width):
        """Get the preferred height a layout item with the given width
        @param width (int)"""
        height = self.doLayout(QRect(0, 0, width, 0), True)
        return height

    def setGeometry(self, rect):
        """Set the geometry of this layout
        @param rect (QRect)"""
        super(FlowLayout, self).setGeometry(rect)
        self.doLayout(rect, False)

    def sizeHint(self):
        """Get the preferred size of this layout
        @return (QSize) The minimum size"""
        return self.minimumSize()

    def minimumSize(self):
        """Get the minimum size of this layout
        @return (QSize)"""
        # Calculate the size
        size = QSize()
        for item in self.itemList:
            size = size.expandedTo(item.minimumSize())
        # Add the margins
        size += QSize(2 * self.margin(), 2 * self.margin())
        return size

    def doLayout(self, rect, testOnly):
        """Layout all the items
        @param rect (QRect) Rect where in the items have to be laid out
        @param testOnly (boolean) Do the actual layout"""
        x = rect.x()
        y = rect.y()
        lineHeight = 0

        for item in self.itemList:
            spaceX = self.spacing()
            spaceY = self.spacing()
            nextX = x + item.sizeHint().width() + spaceX
            if nextX - spaceX > rect.right() and lineHeight > 0:
                x = rect.x()
                y = y + lineHeight + spaceY
                nextX = x + item.sizeHint().width() + spaceX
                lineHeight = 0

            if not testOnly:
                item.setGeometry(QRect(QPoint(x, y), item.sizeHint()))

            x = nextX
            lineHeight = max(lineHeight, item.sizeHint().height())

        return y + lineHeight - rect.y()


# def storeExpanded(treeView, identifier):
#     """
#     @type treeView: QTreeView
#     @type identifier: str
#     """
#     items = []
#     for idx in treeView.model().persistentIndexList():
#         if treeView.isExpanded(idx):
#             items.append(str(idx.data(Qt.UserRole).toString()))
#     print(items)
#     settings = QSettings()
#     settings.setValue(identifier, items)
#
#
# def __restore(tv, expanded, startIdx):
#     model = tv.model()
#     for item in expanded:
#         for match in model.match(startIdx, Qt.UserRole, item):
#             __restore(tv, expanded, model.index(0, 0, match))
#
#
# def restoreExpanded(treeView, identifier):
#     """
#     @type treeView: QTreeView
#     @type identifier: str
#     """
#
#     settings = QSettings()
#     itemList = settings.value(identifier).toStringList()
#     __restore(treeView, itemList, treeView.model().index(0, 0, QModelIndex()))
def dictToQStandardItems(dic):
    return QStandardItem('WHooo')





class LeafFilterProxyModel(QSortFilterProxyModel):
    """ Class to override the following behaviour:
            If a parent item doesn't match the filter,
            none of its children will be shown.

        This Model matches items which are descendants
        or ascendants of matching items.
    """

    def __init__(self):
        super(LeafFilterProxyModel, self).__init__()
        self.__types = [napclient.Entity]
        self.__itemFilter = None

    def setItemFilter(self, filter):
        self.__itemFilter = filter

    def filterAcceptsRow(self, row, parentIndex):
        """ Overriding the parent function """
        if not self.sourceModel().isRootVisible() and not parentIndex.isValid():
            return True


        if self.__itemFilter:
            source_index = self.sourceModel().index(row, 0, parentIndex)
            item = self.sourceModel().itemFromIndex(source_index)
            if item and not self.__itemFilter(item):
                return False

        # Check if the current row matches
        if super(LeafFilterProxyModel, self).filterAcceptsRow(row, parentIndex):
            return True

        # Finally, check if any of the children match
        return self.hasAcceptedChildren(row, parentIndex)

    def filterAcceptsAnyParent(self, parent):
        """ Traverse to the root node and check if any of the
            ancestors match the filter
        """
        while parent.isValid():
            if self.filterAccepsRowItself(parent.row(), parent.parent()):
                return True
            parent = parent.parent()
        return False

    def hasAcceptedChildren(self, row_num, parent):
        """ Starting from the current node as root, traverse all
            the descendants and test if any of the children match
        """
        model = self.sourceModel()
        source_index = model.index(row_num, 0, parent)

        children_count = model.rowCount(source_index)
        for i in xrange(children_count):
            if self.filterAcceptsRow(i, source_index):
                return True
        return False