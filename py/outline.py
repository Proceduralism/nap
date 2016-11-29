import os

from napclient import *
from PyQt5.QtGui import *
from PyQt5.QtCore import *
from model import *
from utils import qtutils

_LAST_OPENED_EXPORT = 'lastOpenedFile'
_NAP_FILE_FILTER = 'NAP File (*.json)'


class OutlineModel(QStandardItemModel):
    def __init__(self, ctx):
        """
        @type ctx: appcontext.AppContext
        """
        self.ctx = ctx
        super(OutlineModel, self).__init__()

        self.__isRootVisible = True
        self.setHorizontalHeaderLabels(['Name', 'Type', 'Value'])

    def root(self):
        """
        @rtype napclient.Object
        """
        return self.__object

    def setRoot(self, obj):
        """ Replace the data in this model with the specified object as root
        @type obj: napclient.Object
        """
        # self.__object = obj
        self.removeRows(0, self.rowCount())
        if obj:
            itemRow = createItemRow(obj)
            self.appendRow(itemRow)
            return itemRow[0].index()

    def isRootVisible(self):
        return self.__isRootVisible

    def setRootVisible(self, b):
        self.__isRootVisible = b


class TypeFilterWidget(QWidget):
    filterChanged = pyqtSignal(object)

    def __init__(self):
        super(TypeFilterWidget, self).__init__()
        self.setLayout(QHBoxLayout())
        self.layout().setSpacing(2)
        self.layout().setContentsMargins(0, 0, 0, 0)
        self.__typeButtons = {}

    def setTypes(self, types):
        for btn in self.__typeButtons.keys():
            del btn

        for t in types:
            btn = QToolButton()
            btn.setCheckable(True)
            btn.setToolTip('Show instances of %s' % t.__name__)
            btn.setIcon(iconstore.iconFor(t))
            btn.clicked.connect(self.__onTypeFilterUpdated)
            self.__typeButtons[btn] = t
            self.layout().addWidget(btn)

    def enabledTypes(self):
        ret = []
        for btn in self.__typeButtons.keys():
            if btn.isChecked():
                ret.append(self.__typeButtons[btn])
        return ret

    def __onTypeFilterUpdated(self):
        self.filterChanged.emit(self.enabledTypes())

    def setTypesEnabled(self, types):
        for btn in self.__typeButtons.keys():
            t = self.__typeButtons[btn]
            if t in types:
                btn.setChecked(True)
            else:
                btn.setChecked(False)


class OutlineWidget(QWidget):
    def __init__(self, ctx, objectName):
        """
        @type ctx: appcontext.AppContext
        """
        self.ctx = ctx
        super(OutlineWidget, self).__init__()

        self.setObjectName(objectName)

        self.setEnabled(False)

        self.__propagateSelection = False
        self.setLayout(QVBoxLayout())

        headerLayout = QHBoxLayout()
        headerLayout.setContentsMargins(0, 0, 0, 0)
        headerLayout.setSpacing(2)

        self.__filterEdit = QLineEdit()
        self.__filterEdit.textEdited.connect(self.__onFilterChanged)
        headerLayout.addWidget(self.__filterEdit)

        self.__typeFilter = TypeFilterWidget()
        self.__typeFilter.filterChanged.connect(self.__onTypeFilterUpdated)
        self.__typeFilter.setTypes([napclient.Entity, napclient.Component, napclient.Attribute])
        headerLayout.addWidget(self.__typeFilter)

        self.layout().addLayout(headerLayout)

        self.__treeView = QTreeView()
        self.__treeView.setSortingEnabled(True)
        self.__treeView.setSelectionMode(QTreeView.ExtendedSelection)

        self.__outlineModel = OutlineModel(ctx)
        self.__outlineModel.dataChanged.connect(self.onDataChanged)
        self.__filterModel = qtutils.LeafFilterProxyModel()
        self.__filterModel.setItemFilter(self.__itemFilter)
        self.__filterModel.setSourceModel(self.__outlineModel)
        self.__filterModel.setFilterCaseSensitivity(Qt.CaseInsensitive)
        self.__treeView.setModel(self.__filterModel)
        self.__treeView.selectionModel().selectionChanged.connect(self.__onSelectionChanged)
        self.__treeView.setContextMenuPolicy(Qt.CustomContextMenu)
        self.__treeView.customContextMenuRequested.connect(self.__onCustomContextMenuRequested)
        self.layout().addWidget(self.__treeView)

        self.ctx.applicationClosing.connect(self.onCloseApp)
        self.ctx.connectionChanged.connect(self.onConnectionChanged)
        QTimer.singleShot(0, self.onDataChanged)
        self.__restoreHeaderState()

    def __restoreHeaderState(self):
        s = QSettings()
        headerStateName = 'HeaderState' + self.objectName()
        headerState = s.value(headerStateName)
        if headerState:
            self.__treeView.header().restoreState(headerState)

    def __saveHeaderState(self):
        s = QSettings()
        headerStateName = 'HeaderState' + self.objectName()
        s.setValue(headerStateName, self.__treeView.header().saveState())

    def __onTypeFilterUpdated(self, types):
        self.__filterModel.invalidate()

    def setPropagateSelection(self, b):
        self.__propagateSelection = b

    def expandAll(self):
        self.__treeView.expandAll()

    def setRoot(self, obj):
        """
        @type obj: napclient.Object
        """
        self.setEnabled(bool(obj))
        if obj:
            assert (isinstance(obj, napclient.Object))

        self.__outlineModel.setRoot(obj)
        # Hide root if necessary
        if not self.__outlineModel.isRootVisible():
            self.__treeView.setRootIndex(self.__treeView.model().index(0, 0))

    def setRootVisible(self, b):
        self.__outlineModel.setRootVisible(b)

    def setFilterTypes(self, types):
        self.__typeFilter.setTypesEnabled(types)

    def __itemFilter(self, item):
        types = self.__typeFilter.enabledTypes()
        assert (isinstance(item, ObjectItem))
        if not types:
            return True
        for t in types:
            if isinstance(item.object(), t):
                return True
        return False

    def onDataChanged(self):
        # self.__treeView.expandAll()
        # for i in reversed(range(self.__treeView.model().columnCount())):
        #     self.__treeView.resizeColumnToContents(i)
        pass

    def onCloseApp(self):
        self.__saveHeaderState()

    def onConnectionChanged(self, *args):
        # qtutils.restoreExpanded(self.__treeView, 'OutlineExpandedState')
        pass

    def __onFilterChanged(self):
        self.__filterModel.setFilterRegExp(self.__filterEdit.text())

    def __selectedItem(self):
        """
        @rtype: ObjectItem
        """
        for item in self.__selectedItems():
            return item

    def __selectedItems(self):
        if not self.__treeView.selectionModel().hasSelection():
            return

        for idx in self.__treeView.selectionModel().selectedRows():
            idx = self.__filterModel.mapToSource(idx)
            item = self.__outlineModel.itemFromIndex(idx)
            yield item

    def __selectedObject(self):
        """
        @rtype: napclient.Object
        """
        for item in self.__selectedItems():
            return item.object()

    def __selectedObjects(self):
        for item in self.__selectedItems():
            yield item.object()

    def __onSelectionChanged(self):
        if self.__propagateSelection:
            self.ctx.setSelection(self.__selectedObject())

    # def __createComponentActions(self, parentObj, menu):
    #     for compType in self.ctx.core().componentTypes():
    #         print(compType)
    #         action = QAction(iconstore.icon('brick_add'), compType, menu)
    #         action.triggered[()].connect(lambda compType=compType: self.ctx.core().addChild(parentObj, compType))
    #         yield action

    @staticmethod
    def __iconAction(menu, text, icon, callback):
        a = QAction(iconstore.icon(icon), text, menu)
        a.triggered.connect(callback)
        menu.addAction(a)

    def __onCustomContextMenuRequested(self, pos):
        selectedObject = self.__selectedObject()
        if not selectedObject:
            return

        menu = QMenu()

        self.__iconAction(menu, 'Expand All', 'toggle-expand', self.__onExpandSelection)
        self.__iconAction(menu, 'Collapse All', 'toggle', self.__onCollapseSelection)

        menu.addSeparator()

        editor = self.ctx.hasEditorFor(selectedObject)
        if editor:
            self.__iconAction(menu, 'Edit', 'brick', self.__onShowEditor)

        if isinstance(selectedObject, napclient.Entity):
            self.__iconAction(menu, 'Add Child', 'add', self.__onAddChild)

            addCompMenu = menu.addMenu(iconstore.icon('brick_add'), 'Add Component...')
            self.ctx.createObjectActions(selectedObject, self.ctx.core().componentTypes(), addCompMenu)

        menu.addSeparator()

        self.__iconAction(menu, 'Cut', 'cut', self.__onCutObjects)
        self.__iconAction(menu, 'Copy', 'copy', self.__onCopyObjects)
        self.__iconAction(menu, 'Paste', 'paste', self.__onPasteObjects)
        self.__iconAction(menu, 'Remove', 'delete', self.__onRemoveObjects)

        menu.addSeparator()

        self.__iconAction(menu, 'Export...', 'disk', self.__onExportSelected)
        self.__iconAction(menu, 'Import...', 'folder_page', self.__onImportObject)
        self.__iconAction(menu, 'Reference...', 'page_link', self.__onReferenceObject)

        menu.exec_(self.__treeView.viewport().mapToGlobal(pos))

    def __onShowEditor(self):
        self.ctx.requestEditorFor(self.__selectedObject())

    def __lastFileDir(self):
        settings = QSettings()
        lastFile = str(settings.value(_LAST_OPENED_EXPORT).toString())
        return os.path.dirname(lastFile)

    def __setLastFile(self, filename):
        s = QSettings()
        s.setValue(_LAST_OPENED_EXPORT, filename)

    def __onAddChild(self):
        parentObj = self.__selectedObject()
        assert parentObj
        if parentObj:
            self.ctx.core().addEntity(parentObj)

    def __onCutObjects(self):
        self.__onCopyObjects()
        self.__onRemoveObjects()

    def __onCopyObjects(self):
        self.ctx.core().copyObjectTree(self.__selectedObject())

    def __onPasteObjects(self):
        jsonStr = str(QApplication.clipboard().text())
        self.ctx.core().pasteObjectTree(self.__selectedObject(), jsonStr)

    def __onRemoveObjects(self):
        print('Remove objects %s' % list(self.__selectedObjects()))
        self.ctx.core().removeObjects(self.__selectedObjects())

    def __onReferenceObject(self):
        QMessageBox.information(self, 'Sorry', 'Not implemented yet')

    def __onImportObject(self):
        parentObj = self.__selectedObject()
        filename = QFileDialog.getOpenFileName(self, 'Select object file to import', self.__lastFileDir(),
                                               _NAP_FILE_FILTER)
        if not filename:
            return

        self.ctx.core().importObject(parentObj, str(filename))

        self.__setLastFile(filename)

    def __onExportSelected(self):
        obj = self.__selectedObject()
        filename = QFileDialog.getSaveFileName(self, 'Select destination file', self.__lastFileDir(),
                                               _NAP_FILE_FILTER)
        if not filename:
            return

        self.__setLastFile(filename)
        self.ctx.core().exportObject(obj, str(filename))

    def __onExpandSelection(self):
        item = self.__selectedItem()
        if not item:
            return
        qtutils.expandChildren(self.__treeView, self.__filterModel.mapFromSource(item.index()))

    def __onCollapseSelection(self):
        item = self.__selectedItem()
        if not item:
            return
        qtutils.expandChildren(self.__treeView, self.__filterModel.mapFromSource(item.index()), False)