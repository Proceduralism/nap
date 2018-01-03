#pragma once

#include <QtGui/QStandardItem>
#include <QtWidgets/QVBoxLayout>
#include <generic/filtertreeview.h>
#include <rtti/rttiobject.h>
#include <rtti/rttipath.h>

namespace napkin
{


	/**
	 * Create a row of items where each item sits in its own column
	 * @param type The type of the item
	 * @param name The name of the item
	 * @param object The object owning the property this item row represents
	 * @param path The path to the property this row represents
	 * @param prop The property this item row represents
	 * @param value The value of the property in this row
	 * @return a row of items where each item sits in its own column
	 */
	QList<QStandardItem*> createPropertyItemRow(rttr::type type, const QString& name, nap::rtti::RTTIObject* object,
												const nap::rtti::RTTIPath& path, rttr::property prop,
												rttr::variant value);

	/**
	 * The base for items that represent a nap::rtti::RTTIObject
	 */
	class BaseRTTIPathItem : public QStandardItem
	{
	public:
		/**
		 * QStandardItem is not a QObject, so regular QObject polymorphism doesn't work.
		 * This function is supposed to solve that.
		 * See: http://doc.qt.io/qt-5/qstandarditem.html#type
		 */
		int type() const override;

		/**
		 * @param name Text on the item.
		 * @param object The object to keep track of.
		 * @param path The path to the property on the object
		 */
		BaseRTTIPathItem(const QString& name, nap::rtti::RTTIObject* object, const nap::rtti::RTTIPath& path);

	protected:
		/**
		 * Resolve a path to get a resolved path.
		 * @return A resolved path
		 */
		nap::rtti::ResolvedRTTIPath resolvePath();

		nap::rtti::RTTIObject* mObject;  // The object we're keeping track of
		const nap::rtti::RTTIPath mPath; // The path to the property on the object
	};

	/**
	 * This item shows the name of an object's property
	 */
	class PropertyItem : public BaseRTTIPathItem
	{
	public:
		/**
		 * QStandardItem is not a QObject, so regular QObject polymorphism doesn't work.
		 * This function is supposed to solve that.
		 * See: http://doc.qt.io/qt-5/qstandarditem.html#type
		 */
		int type() const override;

		/**
		 * @param name Text to display.
		 * @param object The object to keep track of.
		 * @param path The path to the property.
		 */
		PropertyItem(const QString& name, nap::rtti::RTTIObject* object, const nap::rtti::RTTIPath& path);
	};

	/**
	 * This property is has child properties
	 */
	class CompoundPropertyItem : public BaseRTTIPathItem
	{
	public:
		/**
		 * QStandardItem is not a QObject, so regular QObject polymorphism doesn't work.
		 * This function is supposed to solve that.
		 * See: http://doc.qt.io/qt-5/qstandarditem.html#type
		 */
		int type() const override;

		/**
		 * @param name Text to display.
		 * @param object The object to keep track of.
		 * @param path The path to the property.
		 */
		CompoundPropertyItem(const QString& name, nap::rtti::RTTIObject* object, const nap::rtti::RTTIPath& path);

	private:
        /**
         * Generate child items
         */
		void populateChildren();
	};

	/**
	 * The property is an editable list of child properties
	 */
	class ArrayPropertyItem : public BaseRTTIPathItem
	{
	public:
		/**
		 * QStandardItem is not a QObject, so regular QObject polymorphism doesn't work.
		 * This function is supposed to solve that.
		 * See: http://doc.qt.io/qt-5/qstandarditem.html#type
		 */
		int type() const override;

		/**
		 * @param name Text to display.
		 * @param object The object to keep track of.
		 * @param path The path to the property.
		 * @param prop The property the path is pointing to.
		 * @param array Because the property is an array, provide a view into the array.
		 */
		ArrayPropertyItem(const QString& name, nap::rtti::RTTIObject* object, const nap::rtti::RTTIPath& path,
						  rttr::property prop, rttr::variant_array_view array);

	private:
        /**
         * Generate child items
         */
		void populateChildren();

		rttr::property mProperty;
		rttr::variant_array_view mArray;
	};

	/**
	 * This item shows an object pointer
	 */
	class PointerItem : public BaseRTTIPathItem
	{
	public:
		/**
		 * QStandardItem is not a QObject, so regular QObject polymorphism doesn't work.
		 * This function is supposed to solve that.
		 * See: http://doc.qt.io/qt-5/qstandarditem.html#type
		 */
		int type() const override;

		/**
		 * @param name Text to display.
		 * @param object The object to keep track of.
		 * @param path The path to the property, pointer.
		 */
		PointerItem(const QString& name, nap::rtti::RTTIObject* object, const nap::rtti::RTTIPath path);
	};

    /**
     * This item allows for editing a pointer value
     */
	class PointerValueItem : public QStandardItem
	{
	public:
		/**
		 * QStandardItem is not a QObject, so regular QObject polymorphism doesn't work.
		 * This function is supposed to solve that.
		 * See: http://doc.qt.io/qt-5/qstandarditem.html#type
		 */
		int type() const override;

		/**
		 * @param name Text to display.
		 * @param object The object to keep track of.
		 * @param path The path to the property, pointer.
		 */
		PointerValueItem(nap::rtti::RTTIObject* object, const nap::rtti::RTTIPath path, rttr::type valueType);

		/**
		 * Reimplemented from QStandardItem
		 */
		QVariant data(int role) const override;

		/**
		 * Reimplemented from QStandardItem
		 */
		void setData(const QVariant& value, int role) override;

		/**
		 * @return The type of the value represented by the pointer.
		 */
		rttr::type getValueType();

	private:
		nap::rtti::RTTIObject* mObject; // The object to keep track of
		nap::rtti::RTTIPath mPath;		// The path to the property
		rttr::type mValueType;			// The type of the value represented by the pointer
	};

	/**
	 * Creates children, data under the embedded pointer
	 */
	class EmbeddedPointerItem : public BaseRTTIPathItem
	{
	public:
		/**
		 * QStandardItem is not a QObject, so regular QObject polymorphism doesn't work.
		 * This function is supposed to solve that.
		 * See: http://doc.qt.io/qt-5/qstandarditem.html#type
		 */
		int type() const override;

		/**
		 * @param name Text to display.
		 * @param object The object to keep track of.
		 * @param path The path to the property, pointer.
		 */
		EmbeddedPointerItem(const QString& name, nap::rtti::RTTIObject* object, nap::rtti::RTTIPath path);

	private:
		/**
		 * Populate child items
		 */
		void populateChildren();
	};

	/**
	 * This item displays the value of an object property and allows the user to change it
	 */
	class PropertyValueItem : public BaseRTTIPathItem
	{
	public:
		/**
		 * QStandardItem is not a QObject, so regular QObject polymorphism doesn't work.
		 * This function is supposed to solve that.
		 * See: http://doc.qt.io/qt-5/qstandarditem.html#type
		 */
		int type() const override;

		/**
		 * @param name Text to display.
		 * @param object The object to keep track of.
		 * @param path The path to the property, pointer.
		 * @param valueType The type of the value
		 */
		PropertyValueItem(const QString& name, nap::rtti::RTTIObject* object, nap::rtti::RTTIPath path,
						  rttr::type valueType);

		/**
		 * Reimplemented from QStandardItem
		 */
		QVariant data(int role) const override;

		/**
		 * Reimplemented from QStandardItem
		 */
		void setData(const QVariant& value, int role) override;

		/**
		 * @return The type of the value held by the property.
		 */
		rttr::type& getValueType();

	private:
		rttr::type mValueType; // The type of the value held by the property.
	};


}; // napkin