#pragma once

#include <QAction>
#include <QKeySequence>
#include <QFileDialog>
#include <QString>
#include <QStandardItem>
#include <QSet>
#include <QUndoCommand>
#include <nap/entity.h>
#include <nap/logger.h>

#include "appcontext.h"

class Action : public QAction {
public:
    Action();

protected:
    virtual void perform() = 0;
};

class OpenFileAction : public Action {

public:
    OpenFileAction();

private:
    void perform() override;

};


class SaveFileAction : public Action {
public:
    SaveFileAction();

private:
    void perform() override;

};

class SaveFileAsAction : public Action {
public:
    SaveFileAsAction();

private:
    void perform() override;

};


class AddEntityAction : public Action {
public:
    AddEntityAction(nap::Entity* parent) : Action(), mParent(parent)
    {
        setText("Add Entity");
    }

private:
    void perform() override
    {
        AppContext::get().createEntity(mParent);
    }

    nap::Entity* mParent;
};

class AddComponentAction : public Action {
public:
    AddComponentAction(nap::Entity& entity, nap::rtti::TypeInfo type) : Action(), mEntity(entity), mComponentType(type)
    {
        setText(QString(type.get_name().data()));
    }

private:
    void perform() override
    {
        AppContext::get().addComponent(mEntity, mComponentType);
    }

private:
    nap::Entity& mEntity;
    nap::rtti::TypeInfo mComponentType;
};


