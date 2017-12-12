#include "napkinglobals.h"
#include <generic/utility.h>


napkin::ResourceModel::ResourceModel()
{
	setHorizontalHeaderLabels({TXT_LABEL_NAME, TXT_LABEL_TYPE});
}


void napkin::ResourceModel::refresh()
{
	while (rowCount() > 0)
		removeRow(0);

	auto objectsItem = new class GroupItem(TXT_LABEL_OBJECTS);
	appendRow(objectsItem);
	auto entitiesItem = new class GroupItem(TXT_LABEL_ENTITIES);
	appendRow(entitiesItem);

	for (nap::rtti::RTTIObject* ob : topLevelObjects(AppContext::get().getObjectPointers()))
	{
		auto typeItem = new RTTITypeItem(ob->get_type());

		// All objects are in this flat list, filter here
		if (ob->get_type().is_derived_from<nap::Entity>())
		{
			// Grab entities and stuff them in a group
			nap::Entity& e = *rtti_cast<nap::Entity>(ob);

			if (AppContext::get().getParent(e))
				continue; // Only add root objects

			auto entityItem = new EntityItem(e);
			entitiesItem->appendRow({entityItem, typeItem});
		}
		else if (!ob->get_type().is_derived_from<nap::Component>())
		{
			// If it's not a Component, stick under objects group
			objectsItem->appendRow({new ObjectItem(ob), typeItem});
		}
	}
}


napkin::ResourcePanel::ResourcePanel()
{
	setLayout(&mLayout);
	layout()->setContentsMargins(0, 0, 0, 0);
	mLayout.addWidget(&mTreeView);
	mTreeView.setModel(&mModel);
	mTreeView.getTreeView().setColumnWidth(0, 300);
	mTreeView.getTreeView().setSortingEnabled(true);

	connect(&AppContext::get(), &AppContext::fileOpened, this, &ResourcePanel::onFileOpened);
	connect(&AppContext::get(), &AppContext::newFileCreated, this, &ResourcePanel::onNewFile);

	connect(mTreeView.getSelectionModel(), &QItemSelectionModel::selectionChanged, this,
			&ResourcePanel::onSelectionChanged);

	mTreeView.setMenuHook(std::bind(&ResourcePanel::menuHook, this, std::placeholders::_1));
	//    connect(&AppContext::get(), &AppContext::dataChanged, this, &ResourcePanel::refresh);
	connect(&AppContext::get(), &AppContext::entityAdded, this, &ResourcePanel::onEntityAdded);
	connect(&AppContext::get(), &AppContext::componentAdded, this, &ResourcePanel::onComponentAdded);
	connect(&AppContext::get(), &AppContext::objectAdded, this, &ResourcePanel::onObjectAdded);
	connect(&AppContext::get(), &AppContext::objectRemoved, this, &ResourcePanel::onObjectRemoved);
	connect(&AppContext::get(), &AppContext::propertyValueChanged, this, &ResourcePanel::onPropertyValueChanged);
}

void napkin::ResourcePanel::menuHook(QMenu& menu)
{
	auto item = mTreeView.getSelectedItem();
	if (item == nullptr)
		return;

	auto objItem = dynamic_cast<ObjectItem*>(item);
	if (objItem != nullptr)
	{
		auto entityItem = dynamic_cast<EntityItem*>(item);

		if (entityItem != nullptr)
		{
			// Selected item is an Entity
			menu.addAction(new AddEntityAction(&entityItem->getEntity()));

			// Components
			auto addComponentMenu = menu.addMenu("Add Component");
			for (const auto& type : getComponentTypes())
			{
				addComponentMenu->addAction(new AddComponentAction(entityItem->getEntity(), type));
			}
		}

		menu.addAction(new DeleteObjectAction(*objItem->getObject()));
	}

	auto groupItem = dynamic_cast<GroupItem*>(item);
	if (groupItem != nullptr)
	{
		// TODO: Use anything other than string comparison to filter this shit
		// (necessary type chain is not usable at the time of writing)
		if (groupItem->text() == TXT_LABEL_ENTITIES)
		{
			menu.addAction(new AddEntityAction(nullptr));
		}
		else if (groupItem->text() == TXT_LABEL_OBJECTS)
		{

			// Resources
			auto addObjectMenu = menu.addMenu("Add Object");
			std::vector<rttr::type> resource_types = getResourceTypes();
			std::sort(resource_types.begin(), resource_types.end(), [](const rttr::type& typeA, const rttr::type& typeB) 
			{
				return typeA.get_name().compare(typeB.get_name()) < 0;
			});

			for (const auto& type : resource_types)
			{
				addObjectMenu->addAction(new AddObjectAction(type));
			}
		}
	}
}

void napkin::ResourcePanel::onNewFile()
{
	refresh();
}


void napkin::ResourcePanel::onFileOpened(const QString& filename)
{
	refresh();
}

void napkin::ResourcePanel::onSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
	// Grab selected nap objects
	QList<nap::rtti::RTTIObject*> selectedObjects;
	for (auto m : mTreeView.getSelectedItems())
	{
		auto item = dynamic_cast<ObjectItem*>(m);
		if (!item)
			continue;
		selectedObjects << item->getObject();
	}

	selectionChanged(selectedObjects);
}

std::vector<rttr::instance> napkin::ResourcePanel::getSelectedInstances() const
{
	std::vector<rttr::instance> instances;
	for (QStandardItem* item : mTreeView.getSelectedItems())
	{
		auto objItem = dynamic_cast<ObjectItem*>(item);
		if (objItem == nullptr)
			continue;
		instances.emplace_back(objItem->getObject());
	}
	return instances;
}

void napkin::ResourcePanel::refresh()
{
	mModel.refresh();
	mTreeView.getTreeView().expandAll();
}

::napkin::ObjectItem* napkin::ResourcePanel::findItem(const nap::rtti::RTTIObject& obj)
{
	ObjectItem* foundItem = nullptr;

	findIndexInModel(mTreeView.getFilterModel(), [this, &foundItem, &obj](const QModelIndex& idx) -> bool {
		QStandardItem* item = mModel.itemFromIndex(mTreeView.getFilterModel().mapToSource(idx));
		if (item == nullptr)
			return false;

		auto objItem = dynamic_cast<ObjectItem*>(item);
		if (objItem == nullptr)
			return false;

		if (objItem->getObject() == &obj)
		{
			foundItem = objItem;
			return true;
		}

		return false;
	});

	return foundItem;
}

void napkin::ResourcePanel::onEntityAdded(nap::Entity* entity, nap::Entity* parent)
{
	// TODO: Don't refresh the whole mModel
	mModel.refresh();
	mTreeView.getTreeView().expandAll();
	mTreeView.selectAndReveal(findInModel<ObjectItem>(mModel, *entity));
}

void napkin::ResourcePanel::onComponentAdded(nap::Component& comp, nap::Entity& owner)
{
	// TODO: Don't refresh the whole mModel
	mModel.refresh();
	mTreeView.getTreeView().expandAll();
	mTreeView.selectAndReveal(findInModel<ObjectItem>(mModel, comp));
}

void napkin::ResourcePanel::onObjectAdded(nap::rtti::RTTIObject& obj, bool selectNewObject)
{
	QList<QStandardItem*> selected_items = mTreeView.getSelectedItems();

	nap::rtti::RTTIObject* object_to_select = nullptr;
	if (!selected_items.empty() && !selectNewObject)
	{
		ObjectItem* object_item = dynamic_cast<ObjectItem*>(selected_items[0]);
		if (object_item != nullptr)
			object_to_select = object_item->getObject();
	}
	else if (selectNewObject)
		object_to_select = &obj;
	
	
	// TODO: Don't refresh the whole mModel
	mModel.refresh();
	mTreeView.getTreeView().expandAll();

	if (object_to_select != nullptr)
		mTreeView.selectAndReveal(findInModel<ObjectItem>(mModel, *object_to_select));
}


void napkin::ResourcePanel::onObjectRemoved(nap::rtti::RTTIObject& object)
{
	// TODO: Don't refresh the whole mModel
	mModel.refresh();
	mTreeView.getTreeView().expandAll();
}

void napkin::ResourcePanel::onPropertyValueChanged(const nap::rtti::RTTIObject& obj, const nap::rtti::RTTIPath& path)
{
	auto resolvedPath = resolve(obj, path);
	if (resolvedPath.getProperty().get_name() != "mID")
		return;

	auto objectItem = findInModel<ObjectItem>(mModel, obj);
	if (objectItem != nullptr)
		objectItem->setText(QString::fromStdString(obj.mID));
}
