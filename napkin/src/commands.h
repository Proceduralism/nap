#pragma once

#include <nap/objectptr.h>
#include <rtti/rttipath.h>

#include <QUndoCommand>
#include <QtCore/QVariant>
#include <generic/propertypath.h>
#include <scene.h>

#include "typeconversion.h"

namespace napkin
{
    /**
     * TODO: To be implemented
     */
	class AddObjectCommand : public QUndoCommand
	{
	public:
		AddObjectCommand(const rttr::type& type, nap::rtti::RTTIObject* parent = nullptr);
        /**
         * Redo
         */
		void redo() override;

		/**
		 * Undo
		 */
		void undo() override;
	private:
		const rttr::type mType;
		std::string mObjectName;
		std::string mParentName = "";
	};

    /**
     * TODO: To be implemented
     */
	class DeleteObjectCommand : public QUndoCommand
	{
	public:
		DeleteObjectCommand(nap::rtti::RTTIObject& object);
        /**
         * Undo
         */
        void undo() override;

        /**
         * Redo
         */
        void redo() override;
	private:
		const std::string mObjectName;

	};

    /**
     * This command sets the value of a property
     * TODO: This will just set the value, undo cannot be currently made to work with nap.
     */
	class SetValueCommand : public QUndoCommand
	{
	public:
        /**
         * @param ptr The pointer to the object
         * @param path The path to the property
         * @param newValue The new value of the property
         */
		SetValueCommand(nap::rtti::RTTIObject* ptr, nap::rtti::RTTIPath path, QVariant newValue);

        /**
         * Undo
         */
        void undo() override;

        /**
         * Redo
         */
        void redo() override;

	private:
		nap::rtti::RTTIObject* mObject; // The object that has the property
		nap::rtti::RTTIPath mPath; // The path to the property
		QVariant mNewValue; // The new value
		QVariant mOldValue; // The old value
	};

	class SetPointerValueCommand : public QUndoCommand
	{
	public:
        /**
         * @param ptr The pointer to the object
         * @param path The path to the property
         * @param newValue The new value of the property
         */
		SetPointerValueCommand(nap::rtti::RTTIObject* ptr, nap::rtti::RTTIPath path, nap::rtti::RTTIObject* newValue);

        /**
         * Undo
         */
        void undo() override;

        /**
         * Redo
         */
        void redo() override;

	private:
		nap::rtti::RTTIObject*	mObject;	// The object that has the property
		nap::rtti::RTTIPath		mPath;		// The path to the property
		nap::rtti::RTTIObject*	mNewValue;	// The new value
		nap::rtti::RTTIObject*	mOldValue;	// The old value
	};


	/**
	 * TODO: Can this be an 'AddPointerToVectorCommand'?
	 * Add an entity to a scene
	 */
	class AddEntityToSceneCommand : public QUndoCommand
	{
	public:
		AddEntityToSceneCommand(nap::Scene& scene, nap::Entity& entity);

		void redo() override;
		void undo() override;
	private:
		const std::string mSceneID;
		const std::string mEntityID;
		long mIndex;
	};


	/**
	 * Add an element to an array
	 */
	class AddArrayElementCommand : public QUndoCommand
	{
	public:
		/**
		 * @param prop The array property to add the element to
		 */
		AddArrayElementCommand(const PropertyPath& prop);

		void redo() override;
		void undo() override;
	private:
		const PropertyPath& mPath; ///< The path to the array property
		long mIndex; ///< The index of the newly created element
	};
};